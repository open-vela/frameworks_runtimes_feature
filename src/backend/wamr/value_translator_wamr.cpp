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

#include "feature_description.h"
#include "feature_log.h"
#include "feature_wamr_utils.h"
#include "libdyntype.h"
#include "libdyntype_export.h"

namespace value_translator {
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, int32_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *pnative = (int32_t)get_wasm_args_by_type(double, val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, uint32_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *pnative = (uint32_t)get_wasm_args_by_type(double, val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, int64_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *pnative = (int64_t)get_wasm_args_by_type(double, val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, uint64_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *pnative = (uint64_t)get_wasm_args_by_type(double, val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, float* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *pnative = (float)get_wasm_args_by_type(double, val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, double* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *pnative = get_wasm_args_by_type(double, val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, bool* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *pnative = (bool)get_wasm_args_by_type(double, val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, char** pnative)
{
    if (val == 0 || pnative == NULL)
        return false;

    void* str = get_wasm_args_by_type(void*, val);
    if (!wasm_obj_is_stringref_obj((wasm_obj_t)str))
        return false;

    uint32_t str_len = wasm_string_get_length((wasm_stringref_obj_t)str);
    char* buffer = str_len > 0 ? (char*)malloc(str_len + 1) : NULL;
    if (buffer) {
        wasm_string_to_cstring((wasm_stringref_obj_t)str, buffer, str_len + 1);
    }
    *pnative = buffer;
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, FtJsonObject* pnative)
{
    return false;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, ft_value_t* pnative)
{
    if (val == 0 || pnative == NULL)
        return false;
    void* param = get_wasm_args_by_type(void*, val);
    JSValue* js_value = (JSValue*)wasm_anyref_obj_get_value((wasm_anyref_obj_t)param);
    ft_value_t* ft_val = (ft_value_t*)pnative;
    memcpy(ft_val, js_value, sizeof(JSValue));
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, int32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(wasm_number, native, *ptarget);
    return true;
}
bool toTarget(wasm_exec_env_t exec_env, uint32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(wasm_number, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, int64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(wasm_number, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, uint64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(wasm_number, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, float native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(wasm_number, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, double native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(wasm_number, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, bool native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(uint64_t, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, const char* native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    const char* str = (char*)native;
    wasm_stringref_obj_t obj = create_wasm_string(exec_env, str);
    push_local_obj_ref(exec_env, obj);
    set_wasm_var_by_type(void*, obj, *ptarget);
    return true;
}

static void extern_obj_finalizer(wasm_anyref_obj_t obj, void* data)
{
    dyn_value_t value = (dyn_value_t)wasm_anyref_obj_get_value(obj);
    free(value);
}

wasm_anyref_obj_t new_anyref_obj(wasm_exec_env_t exec_env, const void* ptr, wasm_obj_finalizer_t finalizer)
{
    wasm_anyref_obj_t any_obj = wasm_anyref_obj_new(exec_env, ptr);
    if (!any_obj) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
            "alloc memory failed");
        return NULL;
    }
    wasm_obj_set_gc_finalizer(exec_env, (wasm_obj_t)any_obj, finalizer, dyntype_get_context());
    return any_obj;
}

bool toTarget(wasm_exec_env_t exec_env, ft_value_t native, uint64_t* ptarget)
{
    JSValue* new_val = (JSValue*)malloc(sizeof(JSValue));
    *new_val = *((JSValue*)&native);
    wasm_anyref_obj_t any_obj = new_anyref_obj(exec_env, new_val, (wasm_obj_finalizer_t)extern_obj_finalizer);
    *ptarget = (uint64_t)any_obj;
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, FtArrayBuffer native, uint64_t* ptarget)
{
    return false;
}

bool isNull(wasm_exec_env_t exec_env, const uint64_t& value)
{
    if (value == 0)
        return true;

    wasm_obj_t obj = (wasm_obj_t)(value);
    if (wasm_obj_is_null_obj(obj))
        return true;
    if (wasm_obj_is_anyref_obj(obj)) {
        dyn_value_t v = (dyn_value_t)wasm_anyref_obj_get_value((wasm_anyref_obj_t)obj);
        return dyntype_is_null(dyntype_get_context(), v);
    }
    return false;
}

bool isUndefined(wasm_exec_env_t exec_env, const uint64_t& value)
{
    return value == 0;
}

bool isString(wasm_exec_env_t exec_env, const uint64_t& value)
{
    if (value == 0)
        return false;

    void* str = get_wasm_args_by_type(void*, value);
    if (!wasm_obj_is_stringref_obj((wasm_obj_t)str))
        return false;

    return true;
}

void freeCString(wasm_exec_env_t exec_env, char* str)
{
    free(str);
}

void freeValue(wasm_exec_env_t exec_env, uint64_t& target)
{
}

bool isArray(wasm_exec_env_t exec_env, uint64_t& target)
{
    wasm_obj_t obj = get_wasm_args_by_type(wasm_obj_t, target);
    if (!wasm_obj_is_struct_obj(obj))
        return false;

    wasm_array_obj_t arr_ref = get_array_ref((wasm_struct_obj_t)obj);
    return wasm_obj_is_array_obj((wasm_obj_t)arr_ref);
}

bool isObject(wasm_exec_env_t exec_env, uint64_t& target)
{
    wasm_obj_t obj = get_wasm_args_by_type(wasm_obj_t, target);
    if (wasm_obj_is_struct_obj(obj) || wasm_obj_is_array_obj(obj) || wasm_obj_is_func_obj(obj))
        return true;

    return false;
}

bool isPlainObject(wasm_exec_env_t exec_env, uint64_t& target)
{
    wasm_obj_t obj = get_wasm_args_by_type(wasm_obj_t, target);
    if (!wasm_obj_is_struct_obj(obj))
        return false;
    return true;
}

bool isFunction(wasm_exec_env_t exec_env, uint64_t& target)
{
    wasm_obj_t obj = get_wasm_args_by_type(wasm_obj_t, target);
    if (!wasm_obj_is_struct_obj(obj)) {
        return false;
    }
    wasm_struct_obj_t struct_obj = (wasm_struct_obj_t)obj;
    wasm_value_t func_val = { 0 };
    wasm_struct_obj_get_field(struct_obj, 2, false, &func_val);
    wasm_obj_t func_obj = (wasm_obj_t)(func_val.gc_obj);
    if (wasm_obj_is_func_obj(func_obj))
        return true;

    return false;
}

bool isArrayBuffer(wasm_exec_env_t exec_env, uint64_t& target)
{
    return false;
}

uint32_t arraySize(wasm_exec_env_t exec_env, const uint64_t& array)
{
    wasm_obj_t obj = get_wasm_args_by_type(wasm_obj_t, array);
    if (!wasm_obj_is_struct_obj(obj))
        return 0;

    return get_array_length((wasm_struct_obj_t)obj);
}

uint64_t arrayGet(wasm_exec_env_t exec_env, const uint64_t& array, uint32_t idx)
{
    wasm_value_t ret = { 0 };
    wasm_obj_t obj = get_wasm_args_by_type(wasm_obj_t, array);
    if (!wasm_obj_is_struct_obj(obj))
        return 0;

    wasm_array_obj_t arr_ref = get_array_ref((wasm_struct_obj_t)obj);
    wasm_array_obj_get_elem(arr_ref, idx, false, &ret);
    return *((uint64_t*)(&ret));
}

bool arraySet(wasm_exec_env_t exec_env, const uint64_t& array, int32_t idx, uint64_t val)
{
    if (array == 0 || !wasm_obj_is_struct_obj((wasm_obj_t)array))
        return false;

    wasm_array_obj_t arr_ref = get_array_ref((wasm_struct_obj_t)array);
    wasm_array_obj_set_elem(arr_ref, idx, (wasm_value_t*)&val);
    return true;
}

uint64_t newObject(wasm_exec_env_t exec_env)
{
    return 0;
}

bool getObjectField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, uint64_t* pfield)
{
    return false;
}

bool setObjectField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, uint64_t field)
{
    return false;
}

uint64_t newArray(wasm_exec_env_t exec_env)
{
    return 0;
}

ft_value_t nullFtVal()
{
    ft_value_t ft_val = { 0 };
    *((JSValue*)&ft_val) = JS_NULL;
    return ft_val;
}

JSValue getVariArg(wasm_exec_env_t exec_env, uint64_t* argv, uint32_t index)
{
    assert(wasm_obj_is_struct_obj((wasm_obj_t)(*argv)));
    wasm_struct_obj_t array_struct = (wasm_struct_obj_t)(*argv);
    wasm_array_obj_t array = get_array_ref(array_struct);

    void* addr = wasm_array_obj_elem_addr(array, index);
    JSValue* ret_ptr = (JSValue*)wasm_anyref_obj_get_value(*((wasm_anyref_obj_t*)addr));
    return *ret_ptr;
}

uint64_t toTargetPromise(wasm_exec_env_t exec_env, JSContext* js_ctx, const JSValue& promise)
{
    JSValue* ppromise = dynamic_dup_value(js_ctx, promise);

    wasm_anyref_obj_t any_obj = wasm_anyref_obj_new(exec_env, ppromise);
    wasm_obj_set_gc_finalizer(exec_env, (wasm_obj_t)any_obj,
        (wasm_obj_finalizer_t)dynamic_object_finalizer, dyntype_get_context());

    return (uint64_t)any_obj;
}

void* interfaceFromTarget(wasm_exec_env_t exec_env, uint64_t& target)
{
    void* param = *((void**)(&target));
    return param;
}

uint64_t targetFromInterface(wasm_exec_env_t exec_env, void* interf)
{
    return (uint64_t)interf;
}

int checkAsyncCallbacks(wasm_exec_env_t exec_env, uint64_t arg)
{
    return 0;
}

int addAsyncCallbacks(wasm_exec_env_t exec_env, void* instance, FeatureType ftype, uint64_t arg)
{
    return -1;
}

static bool get_struct_field_types(ObjectMapType& obj_type, wasm_value_type_t type_arr[], uint32_t count)
{
    if (count <= 0) {
        FEATURE_LOG_ERROR("struct field count is invalid!");
        return false;
    }
    for (uint32_t i = 0; i < count; i++) {
        auto member = obj_type.members[i];
        wasm_value_type_t wsm_type = ft_type_to_wasm_type(member.type);
        if (wsm_type == 0)
            return false;
        type_arr[i] = wsm_type;
    }
    return true;
}

static wasm_struct_type_t get_struct_type(wasm_exec_env_t exec_env, ObjectMapType& obj_map_type, uint32_t member_count)
{
    wasm_struct_type_t struct_type = NULL;
    wasm_value_type_t field_types[member_count];
    if (!get_struct_field_types(obj_map_type, field_types, member_count))
        return struct_type;
    find_struct_type(exec_env, field_types, member_count, &struct_type);
    return struct_type;
}

uint64_t createStruct(wasm_exec_env_t exec_env, ObjectMapType& obj_map_type, uint32_t member_count)
{
    wasm_struct_type_t struct_type = get_struct_type(exec_env, obj_map_type, member_count);
    if (!struct_type) {
        FEATURE_LOG_ERROR("can not find wasm struct type!");
        return 0;
    }
    wasm_struct_obj_t struct_obj = wasm_struct_obj_new_with_type(exec_env, struct_type);
    push_local_obj_ref(exec_env, struct_obj);
    return (uint64_t)struct_obj;
}

bool getStructField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, int idx, uint64_t* field)
{
    if (obj == 0 || !wasm_obj_is_struct_obj((wasm_obj_t)obj))
        return false;

    // the 0 field stores this pointer of the object
    idx++;
    wasm_struct_obj_get_field((wasm_struct_obj_t)obj, idx, false, (wasm_value_t*)field);
    return true;
}

bool setStructField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, int idx, uint64_t field)
{
    if (obj == 0 || !wasm_obj_is_struct_obj((wasm_obj_t)obj))
        return false;
    // the 0 field stores this pointer of the object
    idx++;
    wasm_struct_obj_set_field((wasm_struct_obj_t)obj, idx, ((wasm_value_t*)&field));
    return true;
}

uint64_t createArray(wasm_exec_env_t exec_env, FeatureType& type, uint32_t array_size)
{
    wasm_value_type_t wasm_type = ft_type_to_wasm_type(type);
    if (wasm_type == 0) {
        FEATURE_LOG_ERROR("can not convert feature type to wasm type!");
        return 0;
    }

    wasm_struct_obj_t array_struct = create_array_with_type(exec_env, array_size, wasm_type);
    if (!array_struct) {
        FEATURE_LOG_ERROR("can not create wasm array!");
        return 0;
    }

    push_local_obj_ref(exec_env, array_struct);
    return (uint64_t)array_struct;
}

}
