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

#include "value_translator_wamr.h"
#include "feature_log.h"
#include "feature_context_qjs.h"
#include "feature_ffi_wamr.h"
#include "feature_wamr_utils.h"
#include "libdyntype.h"
#include "libdyntype_export.h"

namespace value_translator {
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, int32_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *(int32_t*)pnative = (int32_t)get_wasm_args_by_type(double,val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, uint32_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *(int32_t*)pnative = (int32_t)get_wasm_args_by_type(double,val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, int64_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *(int64_t*)pnative = (int64_t)get_wasm_args_by_type(double,val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, uint64_t* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *(uint64_t*)pnative = (uint64_t)get_wasm_args_by_type(double,val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, float* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *(float*)pnative = (float)get_wasm_args_by_type(double,val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, double* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *(float64*)pnative = (float64)get_wasm_args_by_type(double,val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, bool* pnative)
{
    if (pnative == NULL) {
        return false;
    }
    *(int32_t*)pnative = (int32_t)get_wasm_args_by_type(double,val);
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, char** pnative)
{
    if (pnative == NULL)
        return false;

    void* str = get_wasm_args_by_type(void*, val);
    if (!wasm_obj_is_stringref_obj((wasm_obj_t)str))
        return false;

    uint32_t str_len = wasm_string_get_length((wasm_stringref_obj_t)str);
    char *buffer = str_len > 0 ? (char *)malloc(str_len + 1) : nullptr;
    if (buffer != nullptr) {
        wasm_string_to_cstring((wasm_stringref_obj_t)str, buffer, str_len + 1);
    }
    *pnative = buffer;
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, ft_value_t* pnative)
{
    void* param = get_wasm_args_by_type(void*, val);
    JSValue* js_value = (JSValue*)wasm_anyref_obj_get_value((wasm_anyref_obj_t)param);
    qjs_val_t* q_val = (qjs_val_t*)pnative;
    q_val->js_val = *js_value;
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, int32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(int64_t, native, *ptarget);
    return true;
}
bool toTarget(wasm_exec_env_t exec_env, uint32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(int64_t, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, int64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(int64_t, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, uint64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(uint64_t, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, float native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(float, native, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, double native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(double, native, *ptarget);
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

bool toTarget(wasm_exec_env_t exec_env, char* native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    const char* str = (char*)native;
    wasm_stringref_obj_t obj = create_wasm_string(exec_env, str);
    set_wasm_var_by_type(void*, obj, *ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, ft_value_t native, uint64_t* ptarget)
{
    return true;
}

bool isNull(wasm_exec_env_t exec_env,const uint64_t& value)
{
    wasm_anyref_obj_t any_obj =
            (wasm_anyref_obj_t)wasm_anyref_obj_new(exec_env, (void*)value);
    if (!any_obj) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "alloc memory failed");
        return false;
    }
    dyn_value_t v = (dyn_value_t)wasm_anyref_obj_get_value(any_obj);
    return dyntype_is_null(dyntype_get_context(),v);
}

bool isUndefined(wasm_exec_env_t exec_env,const uint64_t& value)
{
    return value == 0;
}

bool isString(wasm_exec_env_t exec_env,const uint64_t& value)
{
    void* str = get_wasm_args_by_type(void*, value);
    if (!wasm_obj_is_stringref_obj((wasm_obj_t)str))
        return false;

    return true;
}

void freeCString(wasm_exec_env_t exec_env, char* str)
{
    free(str);
}

bool getObjectField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, int idx, uint64_t* pfield)
{
    wasm_struct_obj_t wasm_obj = get_wasm_args_by_type(wasm_struct_obj_t, obj);
    WASMValue feild = { 0 };
    wasm_struct_obj_get_field(wasm_obj, idx + 1, false, &feild);
    if (feild.u64 == 0)
        return false;

    *pfield = *((uint64_t*)(&feild));
    return true;
}

void freeValue(wasm_exec_env_t exec_env, uint64_t& target)
{

}

bool isArray(wasm_exec_env_t exec_env, uint64_t& target)
{
    wasm_struct_obj_t struct_obj = get_wasm_args_by_type(wasm_struct_obj_t, target);
    if (!wasm_obj_is_struct_obj((wasm_obj_t)struct_obj))
        return false;

    wasm_array_obj_t arr_ref = get_array_ref(struct_obj);
    return wasm_obj_is_array_obj((wasm_obj_t)arr_ref);
}

uint32_t arraySize(wasm_exec_env_t exec_env, const uint64_t& array)
{
    wasm_struct_obj_t struct_obj = get_wasm_args_by_type(wasm_struct_obj_t, array);
    if (!wasm_obj_is_struct_obj((wasm_obj_t)struct_obj))
        return 0;

    return get_array_length(struct_obj);
}

uint64_t arrayGet(wasm_exec_env_t exec_env, const uint64_t& array, uint32_t idx)
{
    WASMValue ret = { 0 };
    wasm_struct_obj_t struct_obj = get_wasm_args_by_type(wasm_struct_obj_t, array);
    if (!wasm_obj_is_struct_obj((wasm_obj_t)struct_obj))
        return 0;

    wasm_array_obj_t arr_ref = get_array_ref(struct_obj);
    wasm_array_obj_get_elem(arr_ref, idx, false, &ret);
    return *((uint64_t*)(&ret));
}

uint64_t newObject (wasm_exec_env_t exec_env)
{
    return 0;
}

bool setObjectField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, uint64_t field)
{
    return false;
}

uint64_t newArray (wasm_exec_env_t exec_env)
{
    return 0;
}

bool arraySet(wasm_exec_env_t exec_env, const uint64_t& array, int32_t idx, uint64_t val)
{
    return false;
}

ft_value_t nullFtVal()
{
    ft_value_t ft_val;
    return ft_val;
}

uint64_t getVariArg(wasm_exec_env_t exec_env, uint64_t& arg)
{
    return arg;
}

void toTargetPromise(wasm_exec_env_t exec_env, const uint64_t& promise, uint64_t& ret_val)
{

}

void* interfaceFromTarget(uint64_t& target)
{
    void* param = *((void **)(&target));
    return param;
}

uint64_t targetFromInterface(void* interf)
{
    return (uint64_t)interf;
}
}
