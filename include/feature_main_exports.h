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
 * @brief 这里定义了一系列feature框架相关接口，帮助feature管理者管理feature
 */
#ifndef FEATURE_MAIN_EXPORTS_H
#define FEATURE_MAIN_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "feature_types.h"
#include "uv.h"
#include <stdbool.h>

/** 参数错误信息 */
typedef struct {
    int argc; /**< 参数个数 */
    void* argv; /**< 参数列表 */
    int error_code; /**< 错误码 */
    const char* error_msg; /**< 错误信息 */
} ArgsErrorInfo;

/** 参数错误回调函数指针 */
typedef bool (*ArgsErrorCb)(void* data, ArgsErrorInfo* args_info);

/** feature管理类型 */
typedef enum FeatureManagerType {
    FEATURE_MANAGER_JS, /**< js feature manager */
    FEATURE_MANAGER_WAMR, /**< wamr feature manager */
} FeatureManagerType;

/** feature原始上下文句柄 */
typedef void* FeatureRawContextHandle;

/** ReleaseRawContextCb ptr */
typedef void (*ReleaseRawContextCb)(FeatureRawContextHandle);

/** 一个结构体声明，用来描述feature管理者创建时需要的信息 */
typedef struct FeatureManagerCreateInfo {
    FeatureRawContextHandle raw_ctx; /**< 原始feature上下文句柄 */
    ReleaseRawContextCb release_cb; /**< 释放原始feature上下文句柄的回调函数 */
    FeatureManagerType manager_type; /**< feature管理类型 */
    const char* package_name; /**< 包名 */
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

#ifdef __cplusplus
}
#endif

#endif // FEATURE_MAIN_EXPORTS_H
