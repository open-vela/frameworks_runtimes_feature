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
#include "feature_wamr_utils.h"

namespace value_translator {
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, int32_t* pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(double, num_val, pval);
    *(int32_t*)pnative = (int32_t)num_val;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, uint32_t* pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(double, num_val, pval);
    *(int32_t*)pnative = (int32_t)num_val;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, int64_t* pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(double, num_val, pval);
    *(int64_t*)pnative = (int64_t)num_val;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, uint64_t* pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(double, num_val, pval);
    *(u_int64_t*)pnative = (u_int64_t)num_val;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, float* pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(double, num_val, pval);
    *(float*)pnative = (float)num_val;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, double* pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(double, num_val, pval);
    *(float64*)pnative = (float64)num_val;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, bool* pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(double, num_val, pval);
    *(int32_t*)pnative = (int32_t)num_val;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, const char** pnative)
{
    if (pval == NULL || pnative == NULL) {
        return false;
    }
    native_raw_get_arg(void*, str, pval);
    uint32_t str_len = 0, len = 0;
    if (wasm_obj_is_stringref_obj((wasm_obj_t)str)) {
        str_len = wasm_string_get_length((wasm_stringref_obj_t)str);
    }
    char* buffer = str_len > 0 ? (char*)malloc(str_len + 1) : nullptr;
    if (buffer != nullptr) {
        len = wasm_string_to_cstring((wasm_stringref_obj_t)str, buffer, str_len + 1);
    }
    char* alloc_ptr = (char*)FeatureMalloc(strlen(buffer) + 1, FT_CHAR);
    strcpy(alloc_ptr, buffer);
    *pnative = alloc_ptr;
    return true;
}

static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, ft_value_t* pnative)
{
    native_raw_get_arg(void*, param, pval);
    JSValue* js_value = (JSValue*)wasm_anyref_obj_get_value((wasm_anyref_obj_t)param);
    ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY);
    qjs_val_t* q_val = (qjs_val_t*)f_val;
    q_val->js_val = *js_value;
    pnative = f_val;
    return true;
}

static bool toTarget(wasm_exec_env_t* exec_env, const int32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(int64_t, ptarget);
    native_raw_set_return((int32_t)native);
    return true;
}
static bool toTarget(wasm_exec_env_t* exec_env, const uint32_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(int64_t, ptarget);
    native_raw_set_return((uint32_t)native);
    return true;
}

static bool toTarget(wasm_exec_env_t* exec_env, const int64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(int64_t, ptarget);
    native_raw_set_return((int64_t)native);
    return true;
}

static bool toTarget(wasm_exec_env_t* exec_env, const uint64_t native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(uint64_t, ptarget);
    native_raw_set_return((uint64_t)native);
    return true;
}

static bool toTarget(wasm_exec_env_t* exec_env, const float native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(float, ptarget);
    native_raw_set_return((float)native);
    return true;
}

static bool toTarget(wasm_exec_env_t* exec_env, const double native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(double, ptarget);
    native_raw_set_return((double)native);
    return true;
}

static bool toTarget(wasm_exec_env_t* exec_env, const bool native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(uint64_t, ptarget);
    native_raw_set_return((bool)native);
    return true;
}

static bool toTarget(wasm_exec_env_t* exec_env, const char* native, uint64_t* ptarget)
{
    if (ptarget == NULL) {
        return false;
    }
    native_raw_return_type(void*, ptarget);
    const char* str = (char*)native;
    wasm_stringref_obj_t obj = create_wasm_string(*exec_env, str);
    native_raw_set_return(obj);
    return true;
}

}