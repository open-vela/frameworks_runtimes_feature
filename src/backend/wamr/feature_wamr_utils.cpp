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
#include "feature_common.h"

#include <cstddef>

int32_t find_struct_type(wasm_exec_env_t exec_env,
    wasm_value_type_t field_types[], uint32_t member_count, wasm_struct_type_t* p_struct_type)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    uint32_t type_count = wasm_get_defined_type_count(module);
    for (uint32_t i = 0; i < type_count; i++) {
        wasm_defined_type_t type = wasm_get_defined_type(module, i);
        if (!wasm_defined_type_is_struct_type(type))
            continue;
        uint32 field_count = wasm_struct_type_get_field_count((wasm_struct_type_t)type);
        if (field_count != (member_count + 1))
            continue;
        bool found = true;
        for (uint32_t j = 0; j < member_count; j++) {
            bool is_mutable = false;
            /* find the ref type of each field from cls struct, starting from index 1*/
            wasm_ref_type_t field_type = wasm_struct_type_get_field_type((wasm_struct_type_t)type, j + 1, &is_mutable);
            // FEATURE_LOG_INFO("field[%d] type: %x, field_type: %x, is_mutable: %d", j, field_types[j], field_type.value_type, is_mutable);
            if (field_types[j] != field_type.value_type || !is_mutable) {
                found = false;
                break;
            }
        }
        if (found && p_struct_type) {
            *p_struct_type = (wasm_struct_type_t)type;
            return i;
        }
    }
    return -1;
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

static int32_t get_array_type(wasm_exec_env_t exec_env, wasm_array_type_t* p_array_type, wasm_value_type_t elem_type)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    uint32_t i, type_count;
    type_count = wasm_get_defined_type_count(module);
    for (i = 0; i < type_count; i++) {
        wasm_defined_type_t type = wasm_get_defined_type(module, i);
        if (!wasm_defined_type_is_array_type(type))
            continue;

        bool is_mutable = false;
        wasm_array_type_t array_type = (wasm_array_type_t)type;
        wasm_ref_type_t element_type = wasm_array_type_get_elem_type(array_type, &is_mutable);
        if (element_type.value_type == elem_type && is_mutable) {
            if (p_array_type) {
                *p_array_type = array_type;
            }
            return i;
        }
    }
    if (p_array_type) {
        *p_array_type = NULL;
    }

    return -1;
}

wasm_struct_obj_t create_array_with_type(wasm_exec_env_t exec_env, uint32_t elem_count, wasm_value_type_t elem_type)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    wasm_local_obj_ref_t local_ref = { 0 };
    wasm_array_type_t array_type = NULL;
    uint32_t type_idx = get_array_type(exec_env, &array_type, elem_type);
    if (type_idx < 0) {
        wasm_runtime_set_exception(module_inst, "can not find array type");
        return NULL;
    }

    /* get result array struct type */
    wasm_struct_type_t array_struct_type = NULL;
    get_array_struct_type(module, type_idx, &array_struct_type);

    wasm_struct_obj_t array_struct = wasm_struct_obj_new_with_type(
        exec_env, array_struct_type);
    if (!array_struct) {
        wasm_runtime_set_exception(module_inst, "alloc array struct memory failed");
        return NULL;
    }

    /* Push object to local ref to avoid being freed at next allocation */
    wasm_runtime_push_local_obj_ref(exec_env, &local_ref);
    local_ref.val = (wasm_obj_t)array_struct;

    wasm_value_t val = { 0 };
    val.gc_obj = NULL;
    wasm_array_obj_t array_obj = wasm_array_obj_new_with_type(
        exec_env, array_type, elem_count, &val);
    if (!array_obj) {
        wasm_runtime_pop_local_obj_ref(exec_env);
        wasm_runtime_set_exception(module_inst, "alloc memory failed");
        return NULL;
    }

    val.gc_obj = (wasm_obj_t)array_obj;
    wasm_struct_obj_set_field(array_struct, 0, &val);
    wasm_value_t array_size = { .i32 = (int32_t)elem_count };
    wasm_struct_obj_set_field(array_struct, 1, &array_size);
    wasm_runtime_pop_local_obj_ref(exec_env);
    return array_struct;
}

wasm_value_type_t ft_type_to_wasm_type(FeatureType ftype)
{
    ftype = FT_GET_REAL_TYPE(ftype);
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (ftype) {
        case FT_BOOLEAN: {
            return VALUE_TYPE_I32;
        } break;
        case FT_INT:
        case FT_INT8:
        case FT_UINT8:
        case FT_INT16:
        case FT_UINT16:
        case FT_INT32:
        case FT_UINT32:
        case FT_INT64:
        case FT_UINT64:
        case FT_FLOAT:
        case FT_DOUBLE: {
            return VALUE_TYPE_F64;
        } break;
        case FT_STRING: {
            return VALUE_TYPE_STRINGREF;
        } break;
        case FT_ANY_REF: {
            return VALUE_TYPE_ANYREF;
        } break;
        default: {
            FEATURE_LOG_ERROR(" invalid primitive struct field type: %d!", ftype);
            return 0;
        } break;
        }
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complex_type->type) {
        case COMPLEX_STRUCT_MAP: {
            return VALUE_TYPE_STRUCTREF;
        } break;
        case COMPLEX_ARRAY: {
            return VALUE_TYPE_ARRAYREF;
        } break;
        default: {
            FEATURE_LOG_ERROR(" invalid complex struct field type: %d!", complex_type->type);
            return 0;
        } break;
        }
    }

    return 0;
}
