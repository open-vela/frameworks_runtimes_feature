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

/**
 * @file feature_exports.h
 * @brief A series of feature framework related interfaces to help developers operate features to complete their abilities.
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
 * @param[in] size molloc size
 * @param[in] featureType type
 * @return void*
 */
void* FeatureMalloc(size_t size, FeatureType featureType);

/**
 * @brief dup feature value, add ref_count.
 *      ptr must be allocated using FeatureMalloc
 *
 * @param[in] ptr native object pointer
 * @return void*
 */
void* FeatureDupValue(void* ptr);

/**
 * @brief free feature value, decrease ref_count
 *      ptr must be allocated using FeatureMalloc
 *
 * @param[in] ptr native object pointer
 */
void FeatureFreeValue(void* ptr);

/**
 * @brief get feature proto handle from feature instance
 *
 * @param[in] handle FeatureInstanceHandle
 * @return FeatureProtoHandle
 */
FeatureProtoHandle FeatureGetProtoHandle(FeatureInstanceHandle handle);

/**
 * @brief get the native object pointer bind to feature proto(global object for
 * all feature instance)
 *
 * @param[in] handle FeatureProtoHandle
 * @return void*(userdata)
 */
void* FeatureGetProtoData(FeatureProtoHandle handle);

/**
 * @brief Set the Feature Proto Data object
 *
 * @param[in] handle FeatureProtoHandle
 * @param[in] data userdata
 */
void FeatureSetProtoData(FeatureProtoHandle handle, void* data);

/**
 * @brief get feature package name from FeatureProtoHandle
 *
 * @param[in] handle FeatureProtoHandle
 * @return char*(quickapp PackageName)
 */
const char* FeatureGetPackageName(FeatureProtoHandle handle);

/**
 * @brief get feature package version from FeatureProtoHandle
 *
 * @param[in] handle FeatureProtoHandle
 * @return char*(quickapp PackageVersion)
 */
const char* FeatureGetPackageVersion(FeatureProtoHandle handle);

/**
 * @brief get the native object pointer bind to feature instance
 *
 * @param[in] handle FeatureInstanceHandle
 * @return void*(userdata)
 */
void* FeatureGetObjectData(FeatureInstanceHandle handle);

/**
 * @brief Set the Feature Object Data object
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] data userdata
 * @return void
 */
void FeatureSetObjectData(FeatureInstanceHandle handle, void* data);

/**
 * @brief get feature context from FeatureInstanceHandle, feature context is
 * guest context.
 *
 * @param[in] handle FeatureInstanceHandle
 * @return ft_context_ref
 */
ft_context_ref FeatureGetContext(FeatureInstanceHandle handle);

/**
 * @brief get feature binding object value from FeatureInstanceHandle
 *
 * @param[in] handle FeatureInstanceHandle
 * @return JSValue
 */
JSValue FeatureGetBindingObject(FeatureInstanceHandle handle);

/**
 * @brief get feature environment name from FeatureProtoHandle
 *
 * @param[in] handle FeatureProtoHandle
 * @return char*(EnvironmentName)
 */
const char* FeatureGetEnvironmentName(FeatureProtoHandle handle);

/**
 * @brief get user defined data from FeatureManager by name
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] name userdata name
 * @return void*
 */
void* FeatureInstanceGetManagerUserData(FeatureInstanceHandle handle,
    const char* name);

/**
 * @brief invoke callback via cid
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] cid callback id
 * @param[in] ... Variable-length argument
 * @return bool
 */
bool FeatureInvokeCallback(FeatureInstanceHandle handle, FtCallbackId cid, ...);

/**
 * @brief invoke callback via cid with variadic parameter
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] cid callback id
 * @param[in] count arg length
 * @param[in] ... Variable-length argument
 * @return bool
 */
bool FeatureInvokeCallbackCount(FeatureInstanceHandle handle, FtCallbackId cid,
    int count, ...);

/**
 * @brief remove callback from instance via cid.
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] cid callback id
 * @return bool
 */
bool FeatureRemoveCallback(FeatureInstanceHandle handle, FtCallbackId cid);

/**
 * @brief promise resolve, only support one param
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] pid promised id
 * @param[in] ... Variable-length argument
 * @return bool
 */
bool FeaturePromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, ...);

/**
 * @brief promise reject，only support one param
 * @param[in] handle FeatureInstanceHandle
 * @param[in] pid promised id
 * @param[in] code error code
 * @param[in] msg error message
 * @return bool
 */
bool FeaturePromiseReject(FeatureInstanceHandle handle, FtPromiseId pid,
    int code, const char* msg);

/**
 * @brief get real promise type
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] pid promised id
 * @return FeaturePromiseType
 */
enum FeaturePromiseType FeatureGetPromiseType(FeatureInstanceHandle handle, FtPromiseId pid);

/**
 * @brief create a FeatureInterfaceHandle
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] vtable vtable
 * @return FeatureInterfaceHandle
 */
FeatureInterfaceHandle FeatureCreateInterface(FeatureInstanceHandle handle,
    VTable* vtable);

/**
 * @brief post a task with callback to feature instance.
 * In task_cb, use FeatureInstanceIsDetached function to check if the handle is detached.
 * After the handle is detached, please do not use it.
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] task_cb TaskCallback
 * @param[in] data userdata
 * @return bool
 */
bool FeaturePost(FeatureInstanceHandle handle, FeatureTaskCallback task_cb,
    void* data);

/**
 * @brief get feature uvloop from FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @return uv_loop_t*
 */
uv_loop_t* FeatureGetUVLoop(FeatureManagerHandle handle);

/**
 * @brief set feature userdata to FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] name userdata name
 * @param[in] data userdata
 */
void FeatureSetManagerUserData(FeatureManagerHandle handle, const char* name, void* data);

/**
 * @brief get feature userdata from FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] name userdata name
 * @return void*(userdata)
 */
void* FeatureGetManagerUserData(FeatureManagerHandle handle, const char* name);

/**
 * @brief get feature manager handle from feature instance
 *
 * @param[in] handle FeatureInstanceHandle
 * @return FeatureManagerHandle
 */
FeatureManagerHandle FeatureGetManagerHandleFromInstance(FeatureInstanceHandle handle);

/**
 * @brief get feature manager handle from feature prototype
 *
 * @param[in] handle FeatureProtoHandle
 * @return FeatureManagerHandle
 */
FeatureManagerHandle FeatureGetManagerHandleFromProto(FeatureProtoHandle handle);

/**
 * @brief check if cid function is exist in js file
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] cid callback id
 * @return bool
 */
bool FeatureCheckCallbackId(FeatureInstanceHandle handle, FtCallbackId cid);

/**
 * @brief add ref for FeatureInstanceHandle
 *
 * @param[in] handle FeatureInstanceHandle
 * @return FeatureInstanceHandle
 */
FeatureInstanceHandle FeatureDupInstanceHandle(FeatureInstanceHandle handle);

/**
 * @brief dec ref for FeatureInstanceHandle
 *
 * @param[in] handle FeatureInstanceHandle
 */
void FeatureFreeInstanceHandle(FeatureInstanceHandle handle);

/**
 * @brief check feature instance is detached
 *
 * @param[in] handle FeatureInstanceHandle
 * @return bool
 */
bool FeatureInstanceIsDetached(FeatureInstanceHandle handle);

/**
 * @brief get event id from event name
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] name event name
 * @return FtEventId, invalid if FtEventId <= 0
 */
FtEventId FeatureGetEventId(FeatureInstanceHandle handle, const char* name);

/**
 * @brief get event name from event id
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] eid event id
 * @return const char*, NULL if event is not exist
 */
const char* FeatureGetEventName(FeatureInstanceHandle handle, FtEventId eid);

/**
 * @brief emit an event with variadic parameters
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] eid event id
 * @param[in] ... Variable-length argument
 * @return bool
 */
bool FeatureEmitEvent(FeatureInstanceHandle handle, FtEventId eid, ...);

/**
 * @brief emit an event by name and with variadic parameters
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] name event name
 * @param[in] ... Variable-length argument
 * @return bool
 */
bool FeatureEmitEventByName(FeatureInstanceHandle handle, const char* name, ...);

/**
 * @brief set event change listener
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] listener FeatureEventChangeListener
 */
void FeatureSetEventChangeListener(FeatureInstanceHandle handle, FeatureEventChangeListener listener);

/**
 * @brief get event callback count by event id
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] eid event id
 * @return callback count
 */
int FeatureGetEventCallbackCount(FeatureInstanceHandle handle, FtEventId eid);

/**
 * @brief get event callback count by event name
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] name event name
 * @return event callback count
 */
static inline int FeatureGetEventCallbackCountByName(FeatureInstanceHandle handle, const char* name)
{
    return FeatureGetEventCallbackCount(handle, FeatureGetEventId(handle, name));
}

/**
 * @brief registry Features to feature registry
 *
 * @param handle    feature registry handle
 * @param regTable  feature registry table handle
 * @return true
 * @return false
 */
bool FeatureRegisterFeatures(FeatureRegistryHandle handle, const FeatureRegistryTableHandle regTable);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_EXPORTS_H
