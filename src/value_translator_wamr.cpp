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
#include "libdyntype.h"
#include "libdyntype_export.h"
#include "feature_ffi_wamr.h"
#include "feature_wamr_utils.h"

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

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, const char** pnative)
{
    if (pnative == NULL) {
        return false;
    }
    void* str = get_wasm_args_by_type(void*, val);
    uint32_t str_len = 0, len = 0;
    if (wasm_obj_is_stringref_obj((wasm_obj_t)str)) {
        str_len = wasm_string_get_length((wasm_stringref_obj_t)str);
    }
    char *buffer = str_len > 0 ? (char *)malloc(str_len + 1) : nullptr;
    if (buffer != nullptr) {
        len = wasm_string_to_cstring((wasm_stringref_obj_t)str, buffer, str_len + 1);
    }
    char* alloc_ptr = (char*)FeatureMalloc(strlen(buffer) + 1, FT_CHAR);
    strcpy(alloc_ptr, buffer);
    *pnative = alloc_ptr;
    return true;
}

bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, ft_value_t* pnative)
{
    void* param = get_wasm_args_by_type(void*, val);
    JSValue* js_value = (JSValue*)wasm_anyref_obj_get_value((wasm_anyref_obj_t)param);
    ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY);
    qjs_val_t* q_val = (qjs_val_t*)f_val;
    q_val->js_val = *js_value;
    pnative = f_val;
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, int32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(int64_t, *((int32_t*)(&native)), ptarget);
    return true;
}
bool toTarget(wasm_exec_env_t exec_env, uint32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(int64_t, *((uint32_t*)(&native)), ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, int64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(int64_t, *((int64_t*)(&native)), ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, uint64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(int64_t, *((int64_t*)(&native)), ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, float native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(float, *((float*)(&native)), ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, double native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(double, *((double*)(&native)), ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, bool native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    set_wasm_var_by_type(uint64_t, *((bool*)(&native)), ptarget);
    return true;
}

bool toTarget(wasm_exec_env_t exec_env, char* native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    const char* str = (char*)native;
    wasm_stringref_obj_t obj = create_wasm_string(exec_env, str);
    set_wasm_var_by_type(void*, obj, ptarget);
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
    return false;
}

bool isString(wasm_exec_env_t exec_env,const uint64_t& value)
{
    return false;
}

void freeString(wasm_exec_env_t exec_env,const char* str)
{

}

bool getObjectField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, uint64_t* pfield)
{
    return 0;
}

void freeValue(wasm_exec_env_t exec_env, uint64_t& target)
{

}

bool isArray(wasm_exec_env_t exec_env, uint64_t& target)
{
    return false;
}

uint32_t arraySize(wasm_exec_env_t exec_env, const uint64_t& array)
{
    return 0;
}

uint64_t arrayGet(wasm_exec_env_t exec_env, const uint64_t& array, uint32_t idx)
{
    return 0;
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
}