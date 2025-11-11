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
 * @brief A series of feature framework related interfaces to help feature managers manage features
 */
#ifndef FEATURE_MAIN_EXPORTS_H
#define FEATURE_MAIN_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "feature_permission.h"
#include "feature_types.h"
#include "uv.h"
#include <stdbool.h>

/** args error message */
typedef struct {
    int argc; /**< parameters count */
    void* argv; /**< parameters list */
    int error_code; /**< error code */
    const char* error_msg; /**< error message */
} ArgsErrorInfo;

/** ArgsErrorCb ptr */
typedef bool (*ArgsErrorCb)(void* data, ArgsErrorInfo* args_info);

typedef char* (*UriConvertCb)(const char* package_name, const char* uri);

/** FeatureManagerType */
typedef enum FeatureManagerType {
    FEATURE_MANAGER_JS, /**< js feature manager */
    FEATURE_MANAGER_WAMR, /**< wamr feature manager */
} FeatureManagerType;

/** FeatureRawContextHandle */
typedef void* FeatureRawContextHandle;

/** ReleaseRawContextCb ptr */
typedef void (*ReleaseRawContextCb)(FeatureRawContextHandle);

/** A structure declaration that describes the information needed when creating a feature manager. */
typedef struct FeatureManagerCreateInfo {
    FeatureRawContextHandle raw_ctx; /**< raw context handle */
    ReleaseRawContextCb release_cb; /**< release raw context cb */
    FeatureManagerType manager_type; /**< JS or Warm */
    const char* package_name; /**< package name */
} FeatureManagerCreateInfo;

/**
 * @brief create a FeatureManagerHandle, read package-name from pinfo
 *
 * @param[in] pinfo info for createFeatureManager @see FeatureManagerCreateInfo
 * @return FeatureManagerHandle
 */
FeatureManagerHandle FeatureCreateManager(FeatureManagerCreateInfo* pinfo);

/**
 * @brief get a ft_context_ref from a FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @return ft_context_ref
 */
ft_context_ref FeatureManagerGetContext(FeatureManagerHandle handle);

/**
 * @brief set a ArgsError callback to a FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] cb ArgsErrorCb
 * @param[in] data userdata
 */
void FeatureSetArgsErrorCb(FeatureManagerHandle handle, ArgsErrorCb cb, void* data);

/**
 * @brief set packageVersion to a FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] package_version package_version
 */
void FeatureSetPackageVersion(FeatureManagerHandle handle, const char* package_version);

/**
 * @brief free a FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 */
void FeatureFreeManager(FeatureManagerHandle handle);

/**
 * @brief set feature uvloop to FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] loop uv_loop
 * @note: must be called before FeatureCreateInstance
 */
void FeatureSetUVLoop(FeatureManagerHandle handle, uv_loop_t* loop);

/**
 * @brief unset feature uvloop to FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @note: must be called before FeatureFreeManager
 */
void FeatureUnsetUVLoop(FeatureManagerHandle handle);

/**
 * @brief uninit with FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 */
void FeatureUninit(FeatureManagerHandle handle);

/**
 * @brief require a feature with feature name
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] binding_object binding_object
 * @param[in] name feature name
 * @return ft_value_t
 */
ft_value_t FeatureRequire(FeatureManagerHandle handle,
    ft_value_t binding_obj, const char* name);

/**
 * @brief find a feature with feature name
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] name feature name
 * @return ft_value_t
 */
ft_value_t FeatureFindFeature(FeatureManagerHandle handle, const char* name);

/**
 * @brief create a feature with prototype
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] prototype feature prototype
 * @param[in] binding_obj binding_obj
 * @return ft_value_t
 */
ft_value_t FeatureCreateFeature(FeatureManagerHandle handle,
    ft_value_t prototype, ft_value_t binding_obj);

/**
 * @brief set feature userdata to FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] name userdata name
 * @param[in] data userdata
 */
void FeatureSetManagerUserData(FeatureManagerHandle handle, const char* name, void* data);

/**
 * @brief set path operation callback to FeatureManagerHandle
 *
 * @param[in] handle FeatureManagerHandle
 * @param[in] cb UriConvertCb
 */
void FeatureSetUriConvertCb(FeatureManagerHandle handle, UriConvertCb cb);

/**
 * @brief Determine whether the feature exists in the registration list
 * @param[in] handle FeatureManagerHandle
 * @param[in] feature_method feature_method
 * @return bool
 */
bool FeatureHasFeature(FeatureManagerHandle handle, FtString feature_method);

typedef void (*MemoryDumpCountCB)(unsigned int size, void* userdata);
typedef void (*MemoryDumpCountMetaCB)(const char* name, unsigned int value, void* userdata);
typedef void* (*MemoryDumpSubCB)(const char* name, void* userdata);

/** FeatureMemoryDump */
typedef struct {
    MemoryDumpCountCB count; /**< count callback */
    MemoryDumpCountMetaCB count_meta; /**< count meta callback */
    MemoryDumpSubCB sub; /**< sub callback */
} FeatureMemoryDump;

/**
 * @brief FeatureDumpMemory
 * @param[in] feature_manager FeatureManagerHandle
 * @param[in] dump FeatureMemoryDump @see FeatureMemoryDump
 * @param[in] userdata userdata
 */
void FeatureDumpMemory(FeatureManagerHandle feature_manager, FeatureMemoryDump* dump, void* userdata);

/** FeaturePermissionsHandle */
typedef void* FeaturePermissionsHandle;

/** FeaturePermissionsInfo */
typedef struct FeaturePermissionsInfo {
    const FeaturePermissions* permissions; /**< permissions */
    const char* api_name; /**< api_name */
    bool has_async_cbs; /**< has_async_cbs */
} FeaturePermissionsInfo;

/** FeaturePermissionsCb ptr */
typedef void (*FeaturePermissionsCb)(FeaturePermissionsHandle handle, const FeaturePermissionsInfo* permissions, void* data);

/**
 * @brief FeatureSetPermissionsCallback
 * @param[in] hmanager FeatureManagerHandle
 * @param[in] cb FeaturePermissionsCb @see FeaturePermissionsCb
 * @param[in] data data
 */
void FeatureSetPermissionsCallback(FeatureManagerHandle hmanager, FeaturePermissionsCb cb, void* data);

/**
 * @brief FeatureGrantPermission
 * @param[in] hmanager FeatureManagerHandle
 * @param[in] handle FeaturePermissionsHandle @see FeaturePermissionsHandle
 */
void FeatureGrantPermissions(FeatureManagerHandle hmanager, FeaturePermissionsHandle handle);

/** FeaturePermsRejectReason */
typedef enum FeaturePermsRejectReason {
    FEATURE_PERMS_DENIED = 400, /**< feature permissions denied */
    FEATURE_PERMS_ERROR, /**< feature permissions error */
    FEATURE_PERMS_NO_BG, /**< feature permissions no background */
} FeaturePermsRejectReason;

/**
 * @brief FeatureRejectPermission
 * @param[in] hmanager FeatureManagerHandle
 * @param[in] handle FeaturePermissionsHandle @see FeaturePermissionsHandle
 * @param[in] reason int
 */
void FeatureRejectPermissions(FeatureManagerHandle hmanager, FeaturePermissionsHandle handle, FeaturePermsRejectReason reason);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_MAIN_EXPORTS_H
