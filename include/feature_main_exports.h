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

#include "feature_types.h"
#include "uv.h"
#include <stdbool.h>

typedef struct {
    int argc;
    void* argv;
    int error_code;
    const char* error_msg;
} ArgsErrorInfo;

typedef bool (*ArgsErrorCb)(void* data, ArgsErrorInfo* args_info);

typedef enum FeatureManagerType {
    FEATURE_MANAGER_JS,
    FEATURE_MANAGER_WAMR,
} FeatureManagerType;

typedef void* FeatureRawContextHandle;

typedef void (*ReleaseRawContextCb)(FeatureRawContextHandle);

typedef struct FeatureManagerCreateInfo {
    FeatureRawContextHandle raw_ctx;
    ReleaseRawContextCb release_cb;
    FeatureManagerType manager_type;
    const char* package_name;
} FeatureManagerCreateInfo;

/**
 * @brief create a FeatureManagerHandle, read package-name from pinfo
 * @param pinfo
 * @return FeatureManagerHandle
 */
FeatureManagerHandle FeatureCreateManager(FeatureManagerCreateInfo* pinfo);

/**
 * @brief get a ft_context_ref from a FeatureManagerHandle
 * @param handle
 * @return ft_context_ref
 */
ft_context_ref FeatureManagerGetContext(FeatureManagerHandle handle);

/**
 * @brief set a ArgsError callback to a FeatureManagerHandle
 * @param handle
 * @param client
 * @return void
 */
void FeatureSetArgsErrorCb(FeatureManagerHandle handle, ArgsErrorCb cb, void* data);

/**
 * @brief set packageVersion to a FeatureManagerHandle
 * @param handle
 * @param package_version
 * @return void
 */
void FeatureSetPackageVersion(FeatureManagerHandle handle, const char* package_version);

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
ft_value_t FeatureRequire(FeatureManagerHandle handle,
    ft_value_t binding_obj, const char* name);

/**
 * @brief find a feature with feature name
 *
 * @param handle
 * @param ctx
 * @param name
 * @return JSValue prototype
 */
ft_value_t FeatureFindFeature(FeatureManagerHandle handle, const char* name);

/**
 * @brief create a feature with prototype
 *
 * @param handle
 * @param ctx
 * @param prototype
 * @param vm_obj
 * @return JSValue feature_instance
 */
ft_value_t FeatureCreateFeature(FeatureManagerHandle handle,
    ft_value_t prototype, ft_value_t binding_obj);

/**
 * @brief set feature userdata to FeatureManagerHandle
 *
 * @param handle
 * @param name
 * @param data
 * @return void
 */
void FeatureSetManagerUserData(FeatureManagerHandle handle, const char* name, void* data);

bool FeatureHasFeature(FeatureManagerHandle handle, FtString feature_method);

typedef void (*MemoryDumpCountCB)(unsigned int size, void* userdata);
typedef void (*MemoryDumpCountMetaCB)(const char* name, unsigned int value, void* userdata);
typedef void* (*MemoryDumpSubCB)(const char* name, void* userdata);

typedef struct {
    MemoryDumpCountCB count;
    MemoryDumpCountMetaCB count_meta;
    MemoryDumpSubCB sub;
} FeatureMemoryDump;

void FeatureDumpMemory(FeatureManagerHandle feature_manager, FeatureMemoryDump* dump, void* userdata);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_MAIN_EXPORTS_H
