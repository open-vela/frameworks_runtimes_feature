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

#ifndef __VALUE_TRANSLATOR_WAMR_H__
#define __VALUE_TRANSLATOR_WAMR_H__

#include "feature.h"
#include "feature_context_qjs.h"
#include "feature_ffi_wamr.h"
#include "feature_log.h"

#include <cstdarg>
#include <stdalign.h>

namespace value_translator {
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, int32_t* pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, uint32_t* pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, int64_t* pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, uint64_t* pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, float* pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, double* pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, bool* pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, const char** pnative);
static bool toNative(wasm_exec_env_t* exec_env, const uint64_t* pval, ft_value_t* pnative);

static bool toTarget(wasm_exec_env_t* exec_env, const int32_t native, uint64_t* ptarget);
static bool toTarget(wasm_exec_env_t* exec_env, const uint32_t native, uint64_t* ptarget);
static bool toTarget(wasm_exec_env_t* exec_env, const int64_t native, uint64_t* ptarget);
static bool toTarget(wasm_exec_env_t* exec_env, const uint64_t native, uint64_t* ptarget);
static bool toTarget(wasm_exec_env_t* exec_env, const float native, uint64_t* ptarget);
static bool toTarget(wasm_exec_env_t* exec_env, const double native, uint64_t* ptarget);
static bool toTarget(wasm_exec_env_t* exec_env, const bool native, uint64_t* ptarget);
static bool toTarget(wasm_exec_env_t* exec_env, const char* native, uint64_t* ptarget);
}
#endif // __VALUE_TRANSLATOR_QJS_H__