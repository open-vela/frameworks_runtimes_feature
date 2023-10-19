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

#ifndef FEATURE_EXPORTS_H
#define FEATURE_EXPORTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "feature_types.h"
#include <uv.h>
#include "quickjs/quickjs.h"

/**
 * @brief malloc a memory by featureType
 *
 * @param size
 * @param featureType
 * @return void*
 */
void* FeatureMalloc(size_t size, FeatureType featureType);

/**
 * @brief dup feature value, add ref_count.
 *      ptr must be allocated using FeatureMalloc
 *
 * @param ptr
 * @return void*
 */
void* FeatureDupValue(void* ptr);

/**
 * @brief free feature value, decrease ref_count
 *      ptr must be allocated using FeatureMalloc
 *
 * @param ptr
 * @return void
 */
void FeatureFreeValue(void* ptr);

/**
 * @brief get feature proto handle from feature instance
 *
 * @param handle
 * @return FeatureProtoHandle
 */
FeatureProtoHandle FeatureGetProtoHandle(FeatureInstanceHandle handle);

/**
 * @brief get the native object pointer bind to feature proto(global object for all feature instance)
 *
 * @param handle
 * @return void*
 */
void* FeatureGetProtoData(FeatureProtoHandle handle);

/**
 * @brief Set the Feature Proto Data object
 *
 * @param handle
 * @param data
 * @return void
 */
void FeatureSetProtoData(FeatureProtoHandle handle, void* data);

/**
 * @brief get feature package name from FeatureProtoHandle
 *
 * @param handle
 * @return char*
 */
char* FeatureGetPackageName(FeatureProtoHandle handle);

/**
 * @brief get the native object pointer bind to feature instance
 *
 * @param handle
 * @return void*
 */
void* FeatureGetObjectData(FeatureInstanceHandle handle);

/**
 * @brief Set the Feature Object Data object
 *
 * @param handle
 * @param data
 * @return void
 */
void FeatureSetObjectData(FeatureInstanceHandle handle, void* data);

/**
 * @brief get feature context from FeatureInstanceHandle, feature context is guest context.
 *
 * @param handle
 * @return context_ref
 */
ft_context_ref FeatureGetContext(FeatureInstanceHandle handle);

/**
 * @brief get feature binding object value from FeatureInstanceHandle
 *
 * @param handle
 * @return feature_value_t
 */
JSValue FeatureGetBindingObject(FeatureInstanceHandle handle);

/**
 * @brief get feature environment name from FeatureProtoHandle
 *
 * @param handle
 * @return char*
 */
char* FeatureGetEnvironmentName(FeatureProtoHandle handle);

/**
 * @brief get feature uvloop from FeatureInstanceHandle
 *
 * @param handle
 * @return uv_loop_t*
 */
uv_loop_t* FeatureGetUvLoop(FeatureInstanceHandle handle);

/**
 * @brief invoke callback via cid
 *
 * @param handle
 * @param cid
 * @param ...
 * @return bool
 */
bool FeatureInvokeCallback(FeatureInstanceHandle handle, FtCallbackId cid, ...);

/**
 * @brief invoke callback via cid with variadic parameter
 *
 * @param handle
 * @param cid
 * @param count
 * @param ...
 * @return bool
 */
bool FeatureInvokeCallbackCount(FeatureInstanceHandle handle, FtCallbackId cid, int count, ...);

/**
 * @brief remove callback from instance via cid.
 *
 * @param handle
 * @param id
 * @return bool
 */
bool FeatureRemoveCallback(FeatureInstanceHandle handle, FtCallbackId cid);

/**
 * @brief promise resolve, only support one param
 *
 * @param handle
 * @param pid
 * @param ...
 * @return bool
 */
bool FeaturePromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, ...);

/**
 * @brief promise reject，only support one param
 *
 * @param handle
 * @param pid
 * @param ...
 * @return bool
 */
bool FeaturePromiseReject(FeatureInstanceHandle handle, FtPromiseId pid, ...);

/**
 * @brief create a FeatureInterfaceHandle
 *
 * @param handle
 * @param vtable
 * @param vtable_size
 * @return FeatureInterfaceHandle
 */
FeatureInterfaceHandle FeatureCreateInterface(FeatureInstanceHandle handle, VTable* vtable);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_EXPORTS_H