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

#ifndef FEATURE_MAIN_EXPORTS_H
#define FEATURE_MAIN_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "feature_description.h"
#include "feature_types.h"
#include "quickjs/quickjs.h"
#include "uv.h"
#include <stdbool.h>

/**
 * @brief create a FeatureManagerHandle, read package-name from manifest
 * @param manifest
 * @return FeatureManagerHandle
 */
FeatureManagerHandle FeatureCreateManager(const char* package_name);

/**
 * @brief free a FeatureManagerHandle
 * @param handle
 * @return void
 */
void FeatureFreeManager(FeatureManagerHandle handle);

/**
 * @brief set feature uvloop to FeatureManagerHandle
 * @param handle
 * @param loop
 * @return void
 * @note: must be called before FeatureCreateInstance
 */
void FeatureSetUVLoop(FeatureManagerHandle handle, uv_loop_t* loop);

/**
 * @brief unset feature uvloop to FeatureManagerHandle
 * @param handle
 * @return void
 * @note: must be called before FeatureFreeManager
 */
void FeatureUnsetUVLoop(FeatureManagerHandle handle);
/**
 * @brief uninit with FeatureManagerHandle
 *
 * @param handle
 * @return void
 */
void FeatureUninit(FeatureManagerHandle handle);

/**
 * @brief require a feature with feature name
 *
 * @param handle
 * @param ctx
 * @param binding_object
 * @param name
 * @return JSValue
 */
JSValue FeatureRequire(FeatureManagerHandle handle, void* ctx,
    JSValue binding_object, const char* name);

/**
 * @brief find a feature with feature name
 *
 * @param handle
 * @param ctx
 * @param module_name
 * @return JSValue prototype
 */
JSValue FeatureFindFeature(FeatureManagerHandle handle, JSContext* ctx,
    const char* module_name);

/**
 * @brief create a feature with prototype
 *
 * @param handle
 * @param ctx
 * @param prototype
 * @param vm_obj
 * @return JSValue feature_instance
 */
JSValue FeatureCreateFeature(FeatureManagerHandle handle, JSContext* ctx,
    JSValue prototype, JSValue vm_object);

/**
 * @brief register feature to feature registry
 *
 * @param handle
 * @param description
 * @return bool
 */
bool FeatureRegisterFeature(FeatureRegistryHandle handle, const FeatureDescription* description);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_MAIN_EXPORTS_H
