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
#include "uv.h"
#include <protobuf-c/protobuf-c.h>
#include <stdbool.h>
#include <stddef.h>

enum FeatureWorkerState {
    FEATURE_WORKER_PENDING, // worker等待中
    FEATURE_WORKER_RUNNING,
    FEATURE_WORKER_INVALID, // worker处于无效状态
    FEATURE_WORKER_RESOLVED,
    FEATURE_WORKER_REJECTED,
    FEATURE_WORKER_FINISHED, // worker已经完成
};

void* FeatureInstanceAllocProtobuf(FeatureInstanceHandle handle, const ProtobufCMessageDescriptor* desc);
void* FeatureInstanceAllocType(FeatureInstanceHandle hInst, size_t size, FeatureType type);
void* FeatureInstanceAlloc(FeatureInstanceHandle handle, size_t size);
void* FeatureInstanceDupValue(void* ptr);
void FeatureInstanceFreeValue(void* ptr);

// memory utils functions
/**
 * @brief Create a new string from a plain C string
 *
 * @param handle
 * @param str
 * @return char*
 */
char* FeatureStrCopy(FeatureInstanceHandle handle, const char* str);

/**
 * @brief Increase the reference count of a Feature string by one
 *
 * @param ptr
 */
#define FeatureStrAddRef(ptr) FeatureInstanceDupValue(ptr)

/**
 * @brief Creates an array with its contents
 *
 * @param handle
 * @param capacity
 * @param element_type
 * @return FtArray*
 */
FtArray* FeatureCreateArray(FeatureInstanceHandle handle, size_t capacity, FeatureType element_type);

/**
 * @brief Create an array, specify the length, and copy the data from data
 *
 * @param handle
 * @param element_type
 * @param data
 * @param count
 * @return FtArray*
 */
FtArray* FeatureArrayCopyRaw(FeatureInstanceHandle handle, FeatureType element_type, const void* data, size_t count);

/**
 * @brief Create an array, specify the length, and copy the data from data
 *
 * @param handle
 * @param element_type
 * @param data
 * @param count
 * @return FtArray*
 */
FtArray* FeatureArrayCopy(FeatureInstanceHandle handle, FeatureType element_type, const void* data, size_t count);

/**
 * @brief Change the size of an array
 *
 * @param arr
 * @param new_size
 * @param data
 * @param count
 * @return FtArray*
 */
FtArray* FeatureArrayResize(FtArray* arr, size_t new_size);

/**
 * @brief Get the size of the array
 *
 * @param arr
 * @return FtArray*
 */
size_t FeatureArrayGetLength(FtArray* arr);

/**
 * @brief Return element object pointer
 *
 * @param arr
 * @param start
 * @return FtArray*
 */
void* FeatureArrayGetData(FtArray* arr, int start);

/**
 * @brief Add data to array
 *
 * @param arr
 * @param data
 * @return FtArray*
 */
FtArray* FeatureArrayAppend(FtArray* arr, const void* data);

/**
 * @brief Add data to array
 *
 * @param arr
 * @param data
 * @return FtArray*
 */
FtArray* FeatureArrayAppendRaw(FtArray* arr, const void* data);

/**
 * @brief Clear the elements in array->element
 *
 * @param arr
 * @return int
 */
int FeatureArrayClear(FtArray* arr);

/**
 * @brief Delete count elements starting from the start position in array->element
 *
 * @param arr
 * @param start
 * @param count
 * @return int
 */
int FeatureArrayRemove(FtArray* arr, int start, size_t count);

/**
 * @brief Insert count elements after the start position of array->element
 *
 * @param arr
 * @param start
 * @param data
 * @param count
 * @return int
 */
int FeatureArrayInsertAfter(FtArray* arr, int start, const void* data, size_t count);

/**
 * @brief Insert count elements after the start position of array->element
 *
 * @param arr
 * @param start
 * @param data
 * @param count
 * @return int
 */
int FeatureArrayInsertRawAfter(FtArray* arr, int start, const void* data, size_t count);

/**
 * @brief Insert count elements before the start position of array->element
 *
 * @param arr
 * @param start
 * @param data
 * @param count
 * @return int
 */
int FeatureArrayInsertBefore(FtArray* arr, int start, const void* data, size_t count);

/**
 * @brief Insert count elements before the start position of array->element
 *
 * @param arr
 * @param start
 * @param data
 * @param count
 * @return int
 */
int FeatureArrayInsertRawBefore(FtArray* arr, int start, const void* data, size_t count);

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
 * @brief get feature value reference count
 *
 * @param[in] ptr the pointer created by FeatureMalloc
 * @return int32_t reference count
 */
int32_t FeatureGetValueRefCount(void* ptr);

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
 * @brief post a task with callback to feature instance.
 * In task_cb_ext, use FeatureInstanceIsDetached function to check if the handle is detached.
 * After the handle is detached, please do not use it.
 *
 * @param[in] handle FeatureInstanceHandle
 * @param[in] task_cb_ext TaskCallbackExt
 * @param[in] data userdata
 * @return bool
 */
bool FeaturePostExt(FeatureInstanceHandle handle, FeatureTaskCallbackExt task_cb_ext,
    uint64_t data);

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
 * @brief set feature userdata to FeatureManagerHandle with free callback
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] name userdata name
 * @param[in] data userdata
 * @param[in] free_cb free callback
 */
void* FeatureSetManagerUserDataWithFreeCallback(FeatureManagerHandle handle,
    const char* name, void* data, ManagerUserdataFreeCallback free_cb);

/**
 * @brief check if userdata is exist in FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] name userdata name
 * @return bool
 */
bool FeatureManagerHasUserData(FeatureManagerHandle handle, const char* name);

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

/**
 * @brief create worker
 *
 * @param handle
 * @param pid
 * @param buf_size
 * @param do_work
 * @param do_after_worker
 * @param free
 * @return FeatureWorkerHandle
 */
FeatureWorkerHandle FeatureCreateWorker(FeatureInstanceHandle handle, FtPromiseId pid, size_t buf_size,
    void (*do_work)(FeatureWorkerHandle), void (*do_after_worker)(FeatureWorkerHandle),
    void (*free)(void*));

/**
 * @brief commit and start worker
 *
 * @param hworker
 * @return bool if commit success
 */
bool FeatureWorkerCommit(FeatureInstanceHandle handle, FeatureWorkerHandle hworker);

/**
 * @brief resolve worker, called by user
 *
 * @param hworker
 * @param result
 */
void FeatureWorkerResolve(FeatureInstanceHandle handle, FeatureWorkerHandle hworker, FeatureWorkerResult result);

/**
 * @brief reject worker, called by user
 *
 * @param hwoerk
 * @param errcode
 * @param err_msg
 */
void FeatureWorkerReject(FeatureInstanceHandle handle, FeatureWorkerHandle hwoerk, int errcode, const char* err_msg);

/**
 * @brief check if worker is valid
 *
 * @param pWorker
 * @return true
 * @return false
 */
bool FeatureWorkerIsValid(FeatureInstanceHandle handle, FeatureWorkerHandle hworker);

/**
 * @brief get worker state
 *
 * @param hworker
 * @return int
 */
int FeatureWorkerGetState(FeatureWorkerHandle hworker);

enum FeatureWorkerCancelResult {
    FeatureWorkerCancelSuccess, // 成功取消
    FeatureWorkerCancelPending, // pending中，未能取消
    FeatureWorkerCancelInvalid, // worker无效
    FeatureWorkerCancelUnknownError, // 未知错误
};

/**
 * @brief free worker which is not pending
 *
 * @param hworker
 * @param cancel
 * @return int
 */
int FeatureWorkerCancel(FeatureInstanceHandle handle, FeatureWorkerHandle hworker);

/**
 * @brief get json string from FtJsonObject
 *
 * @param json_obj
 * @return char*
 */
const char* FeatureGetJsonString(const FtJsonObject json_obj);

/**
 * @brief alloc a json object with string length
 *
 * @param json_str
 * @return FtJsonObject
 */
FtJsonObject FeatureAllocJsonObject(size_t str_len);

/**
 * @brief create a json object with a string
 *
 * @param json_str
 * @return FtJsonObject
 */
FtJsonObject FeatureNewJsonObject(const char* str);

/**
 * @brief throw error in feature
 *
 * @param handle FeatureInstanceHandle
 * @param msg error message
 */
void FeatureThrowError(FeatureInstanceHandle handle, const char* msg);

/** FeatureArrayBufferFreeFunc */
typedef void (*FeatureArrayBufferFreeFunc)(void* opaque, void* buff);

/**
 * @brief create an array buffer from a data pointer
 *
 * @param handle FeatureInstanceHandle
 * @param data
 * @param len
 * @param free_func
 * @param opaque
 * @return FtArrayBuffer
 */
FtArrayBuffer FeatureNewArrayBufferFromData(FeatureInstanceHandle handle, uint8_t* data, size_t len,
    FeatureArrayBufferFreeFunc free_func, void* opaque);

/**
 * @brief create an array buffer and copy data from a data pointer
 *
 * @param handle FeatureInstanceHandle
 * @param data
 * @param len
 * @return FtArrayBuffer
 */
FtArrayBuffer FeatureNewArrayBufferCopyData(FeatureInstanceHandle handle, uint8_t* data, size_t len);

/**
 * @brief get data from an array buffer
 *
 * @param buff
 * @param psize
 * @return data
 */
uint8_t* FeatureArrayBufferGetData(FtArrayBuffer buff, size_t* psize);

/**
 * @brief invoke path operation callback
 *
 * @param handle FeatureInstanceHandle
 * @param uri uri string
 * @return char* path string
 */
char* FeatureGetPathFromUri(FeatureInstanceHandle handle, const char* uri);

// basic promise resolve functions
FtBool FeatureFtVoidPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid);

FtBool FeatureFtStringPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtString val);

FtBool FeatureFtIntPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtInt val);

FtBool FeatureFtUint32PromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtUint32 val);

FtBool FeatureFtInt8PromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtInt8 val);

FtBool FeatureFtUint8PromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtUint8 val);

FtBool FeatureFtInt16PromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtInt16 val);

FtBool FeatureFtUint16PromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtUint16 val);

FtBool FeatureFtInt64PromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtInt64 val);

FtBool FeatureFtUint64PromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtUint64 val);

FtBool FeatureFtFloatPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtFloat val);

FtBool FeatureFtDoublePromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtDouble val);

FtBool FeatureFtBoolPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtBool val);

FtBool FeatureFtAnyPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtAny val);

FtBool FeatureFtArrayPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, FtArray* val);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_EXPORTS_H
