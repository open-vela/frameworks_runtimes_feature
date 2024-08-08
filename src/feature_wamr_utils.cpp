/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "feature_wamr_utils.h"

#include <cstddef>

static wasm_value_type_t get_wasm_type_from_ts_val(wasm_exec_env_t exec_env, ts_value_t ts_val)
{
    switch (ts_val.type) {
    case TS_STRING: {
        return VALUE_TYPE_STRINGREF;
    }
    case TS_NUMBER: {
        return VALUE_TYPE_F64;
    }
    case TS_BOOLEAN: {
        return VALUE_TYPE_I32;
    }
    default:
        break;
    }
    return 0;
}

static uint32_t get_wasm_struct_type(wasm_exec_env_t exec_env,
    wasm_value_type_t field_types[], uint32_t member_count, wasm_struct_type_t* p_struct_type)
{
    bool mut;
    uint32_t type_count;
    wasm_defined_type_t type;
    wasm_ref_type_t field_type;
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    type_count = wasm_get_defined_type_count(module);

    for (uint32_t i = 0; i < member_count; i++) {
        for (uint32_t j = 0; j < type_count; j++) {
            type = wasm_get_defined_type(module, j);
            if (!wasm_defined_type_is_struct_type(type) || wasm_struct_type_get_field_count((wasm_struct_type_t)type) != (member_count + 1))
                continue;

            /* find the ref type of each field from cls struct, starting from index 1*/
            field_type = wasm_struct_type_get_field_type((wasm_struct_type_t)type, i + 1, &mut);
            if (field_types[i] != field_type.value_type || !mut)
                continue;

            if (p_struct_type) {
                *p_struct_type = (wasm_struct_type_t)type;
                return j;
            }
        }
    }

    if (p_struct_type) {
        *p_struct_type = NULL;
    }
    return -1;
}

/*
    utilities for class object
    * class struct (WasmGC struct)
    +----------+      +---------------------------+
    |  0:vtable| ---> |      struct (field i32)   |
    +----------+      +---------------------------+
    |  1: type |      ^      content data 1       ^
    +----------+      |---------------------------|
    |  2: type |      ^      content data 2       ^
    +----------+      |---------------------------|
    |  ...     |      ^      content ...          ^
    +----------+      |---------------------------|
*/
wasm_struct_obj_t create_wasm_struct(wasm_exec_env_t exec_env, ts_value_t obj_arr[],
    uint32_t member_count)
{
    if (member_count < 1)
        return NULL;

    wasm_value_t ele = { .gc_obj = NULL };
    wasm_struct_type_t cls_struct_type = NULL;
    wasm_struct_obj_t cls_struct_obj;
    wasm_stringref_obj_t str_obj;
    wasm_value_type_t field_types[member_count];

    /* traverse the array elements and find the ref type value through the
     * element type */
    for (uint32_t i = 0; i < member_count; i++) {
        field_types[i] = get_wasm_type_from_ts_val(exec_env, obj_arr[i]);
    }
    /* through ref type and member_count, find cls struct type */
    get_wasm_struct_type(exec_env, field_types, member_count,
        &cls_struct_type);

    /* create new cls struct obj through cls_struct_type */
    cls_struct_obj = wasm_struct_obj_new_with_type(exec_env, cls_struct_type);

    /* traverse the array element value and fill it in cls_struct_obj */
    for (uint32_t k = 0; k < member_count; k++) {
        switch (obj_arr[k].type) {
        case TS_STRING: {
            const char* str = (const char*)obj_arr[k].of.ref;
            str_obj = create_wasm_string(exec_env, str);
            ele.gc_obj = (wasm_obj_t)str_obj;
        } break;
        case TS_NUMBER: {
            ele.f64 = obj_arr[k].of.f64;
        } break;
        case TS_BOOLEAN: {
            ele.i32 = obj_arr[k].of.i32;
        } break;
        default:
            break;
        }
        wasm_struct_obj_set_field(cls_struct_obj, k + 1, &ele);
    }
    return cls_struct_obj;
}

wasm_anyref_obj_t create_anyref_obj(wasm_exec_env_t exec_env, const void* ptr)
{
    wasm_anyref_obj_t any_obj = wasm_anyref_obj_new(exec_env, ptr);
    if (!any_obj) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
            "alloc memory failed");
        return NULL;
    }
    wasm_obj_set_gc_finalizer(exec_env, (wasm_obj_t)any_obj,
        (wasm_obj_finalizer_t)dynamic_object_finalizer, dyntype_get_context());
    return any_obj;
}

static uint32_t get_any_array_type(wasm_module_t module, wasm_array_type_t* p_array_type)
{
    uint32_t i, type_count;
    type_count = wasm_get_defined_type_count(module);
    for (i = 0; i < type_count; i++) {
        wasm_defined_type_t type = wasm_get_defined_type(module, i);
        if (!wasm_defined_type_is_array_type(type))
            continue;

        bool is_mutable = false;
        wasm_array_type_t array_type = (wasm_array_type_t)type;
        wasm_ref_type_t elem_type = wasm_array_type_get_elem_type(array_type, &is_mutable);
        if (elem_type.value_type == VALUE_TYPE_ANYREF && is_mutable) {
            if (p_array_type) {
                *p_array_type = array_type;
            }
            return i;
        }
    }
    if (p_array_type) {
        *p_array_type = nullptr;
    }

    return -1;
}

wasm_struct_obj_t create_any_array_struct(wasm_exec_env_t exec_env, uint32_t elem_count)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    wasm_local_obj_ref_t local_ref = { 0 };
    wasm_array_type_t any_array_type = nullptr;
    uint32_t type_idx = get_any_array_type(module, &any_array_type);
    if (type_idx < 0) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
            "can not find any array type");
        return nullptr;
    }

    /* get result array struct type */
    wasm_struct_type_t array_struct_type = nullptr;
    get_array_struct_type(module, type_idx, &array_struct_type);

    wasm_struct_obj_t array_struct = wasm_struct_obj_new_with_type(
        exec_env, array_struct_type);
    if (!array_struct) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
            "alloc memory failed");
        return nullptr;
    }

    /* Push object to local ref to avoid being freed at next allocation */
    wasm_runtime_push_local_obj_ref(exec_env, &local_ref);
    local_ref.val = (wasm_obj_t)array_struct;

    wasm_value_t val = { 0 };
    val.gc_obj = nullptr;
    wasm_array_obj_t array_obj = wasm_array_obj_new_with_type(
        exec_env, any_array_type, elem_count, &val);
    if (!array_obj) {
        wasm_runtime_pop_local_obj_ref(exec_env);
        wasm_runtime_set_exception(module_inst, "alloc memory failed");
        return nullptr;
    }

    val.gc_obj = (wasm_obj_t)array_obj;
    wasm_struct_obj_set_field(array_struct, 0, &val);
    wasm_value_t array_size = { .i32 = (int32_t)elem_count };
    wasm_struct_obj_set_field(array_struct, 1, &array_size);
    wasm_runtime_pop_local_obj_ref(exec_env);
    return array_struct;
}
