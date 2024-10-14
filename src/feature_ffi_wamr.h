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
#ifndef __FEATURE_FFI_WAMR_H__
#define __FEATURE_FFI_WAMR_H__
#include "feature.h"
#include "feature_exports.h"
#include "feature_ffi.h"
#include "gc_object.h"
#include "wasm_export.h"

namespace feature_framework {

class FeatureInstance;

namespace FeatureFFIWamr {

    bool convertConstToGuest(wasm_exec_env_t exec_env, FeatureType ftype, const AppendData& adata, uint64_t& value);

    bool convertValueToHost(FeatureInstance* instance, FeatureType ftype, void*& ptr,
        wasm_exec_env_t exec_env, uint64_t value);

    bool convertValueToGuest(FeatureType ftype, void* ptr, wasm_exec_env_t exec_env, uint64_t& value);
}

}
#endif
