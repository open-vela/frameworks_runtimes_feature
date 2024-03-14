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
extern "C" {
#endif

#include "feature_types.h"
#include "quickjs/quickjs.h"
#include "uv.h"
#include <stdbool.h>

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
 * @brief get the native object pointer bind to feature proto(global object for
 * all feature instance)
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
const char* FeatureGetPackageName(FeatureProtoHandle handle);

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
 * @brief get feature context from FeatureInstanceHandle, feature context is
 * guest context.
 *
 * @param handle
 * @return context_ref
 */
ft_context_ref FeatureGetContext(FeatureInstanceHandle handle);

/**
 * @brief get feature binding object value from FeatureInstanceHandle
 *
 * @param handle
 * @return JSValue
 */
JSValue FeatureGetBindingObject(FeatureInstanceHandle handle);

/**
 * @brief get feature environment name from FeatureProtoHandle
 *
 * @param handle
 * @return char*
 */
const char* FeatureGetEnvironmentName(FeatureProtoHandle handle);

/**
 * @brief get user defined data from FeatureManager by name
 *
 * @param handle
 * @param name
 * @return void*
 */
void* FeatureInstanceGetManagerUserData(FeatureInstanceHandle handle,
    const char* name);

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
bool FeatureInvokeCallbackCount(FeatureInstanceHandle handle, FtCallbackId cid,
    int count, ...);

/**
 * @brief remove callback from instance via cid.
 *
 * @param handle
 * @param id
 * @return bool
 */
bool FeatureRemoveCallback(FeatureInstanceHandle handle, FtCallbackId cid);

/**
 * @brief cancel same callback from instance via cid.
 *
 * @param handle
 * @param id
 * @return bool
 */
int FeatureGetSameCallback(FeatureInstanceHandle handle, FtCallbackId cid);

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
FeatureInterfaceHandle FeatureCreateInterface(FeatureInstanceHandle handle,
    VTable* vtable);

/**
 * @brief post a task with callback to feature instance
 *
 * @param handle
 * @param task_cb
 * @param data
 * @return void
 */
void FeaturePost(FeatureInstanceHandle handle, FeatureTaskCallback task_cb,
    void* data);

/**
 * @brief get feature uvloop from FeatureManagerHandle
 *
 * @param handle
 * @return uv_loop_t*
 */
uv_loop_t* FeatureGetUVLoop(FeatureManagerHandle handle);

/**
 * @brief set feature userdata to FeatureManagerHandle
 *
 * @param handle
 * @param name
 * @param data
 * @return void
 */
void FeatureSetManagerUserData(FeatureManagerHandle handle, const char* name, void* data);

/**
 * @brief get feature userdata from FeatureManagerHandle
 *
 * @param handle
 * @param name
 * @return void*
 */
void* FeatureGetManagerUserData(FeatureManagerHandle handle, const char* name);

/**
 * @brief get feature manager handle from feature instance
 *
 * @param handle
 * @return FeatureManagerHandle
 */
FeatureManagerHandle FeatureGetManagerHandleFromInstance(FeatureInstanceHandle handle);

/**
 * @brief get feature manager handle from feature prototype
 *
 * @param handle
 * @return FeatureManagerHandle
 */
FeatureManagerHandle FeatureGetManagerHandleFromProto(FeatureProtoHandle handle);

/**
 * @brief check if cid function is exist in js file
 *
 * @param handle
 * @param cid
 * @return bool
 */
bool FeatureCheckCallbackId(FeatureInstanceHandle handle, FtCallbackId cid);

/**
 * @brief add ref for FeatureInstanceHandle
 *
 * @param handle
 * @return FeatureInstanceHandle
 */
FeatureInstanceHandle FeatureDupInstanceHandle(FeatureInstanceHandle handle);

/**
 * @brief dec ref for FeatureInstanceHandle
 *
 * @param handle
 * @return void
 */
void FeatureFreeInstanceHandle(FeatureInstanceHandle handle);

/**
 * @brief check feature instance is detached
 *
 * @param handle
 * @return bool
 */
bool FeatureInstanceIsDetached(FeatureInstanceHandle handle);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_EXPORTS_H
