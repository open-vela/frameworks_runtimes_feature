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

#include "feature_main_exports.h"
#include "feature_common.h"
#include "feature_log.h"
#include "feature_manager.h"

#define FEATURE_CHECK_PTR(__ptr__, __ret__, __log__) \
    do {                                             \
        if (!__ptr__) {                              \
            FEATURE_LOG_ERROR(__log__);              \
            return __ret__;                          \
        }                                            \
    } while (0);

using namespace feature_framework;

FeatureManagerHandle FeatureCreateManager(FeatureManagerCreateInfo* pinfo)
{
    return (FeatureManagerHandle)FeatureManager::CreateFeatureManager(pinfo);
}

ft_context_ref FeatureManagerGetContext(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->getFeatureContext();
}

void FeatureSetArgsErrorCb(FeatureManagerHandle handle, ArgsErrorCb cb, void* data)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    if (manager) {
        manager->setArgsErrorCb(cb, data);
    }
}

void FeatureSetPackageVersion(FeatureManagerHandle handle, const char* package_version)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    if (manager) {
        manager->setPackageVesion(package_version);
    }
}

void FeatureFreeManager(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    delete manager;
}

void FeatureSetUVLoop(FeatureManagerHandle handle, uv_loop_t* loop)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FEATURE_CHECK_PTR(loop, ;, "loop is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->setUVLoop(loop);
}

void FeatureUnsetUVLoop(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->unsetUVLoop();
}

void FeatureUninit(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->uninit();
}

ft_value_t FeatureRequire(FeatureManagerHandle handle, ft_value_t binding_obj, const char* name)
{
    ft_value_t ret = { 0 };
    FEATURE_CHECK_PTR(handle, ret, "handle is null !")
    FEATURE_CHECK_PTR(name, ret, "name is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->featureRequire(binding_obj, name);
}

ft_value_t FeatureFindFeature(FeatureManagerHandle handle, const char* name)
{
    ft_value_t ret = { 0 };
    FEATURE_CHECK_PTR(handle, ret, "handle is null !")
    FEATURE_CHECK_PTR(name, ret, "name is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->findFeature(name);
}

ft_value_t FeatureCreateFeature(FeatureManagerHandle handle, ft_value_t prototype, ft_value_t binding_obj)
{
    ft_value_t ret = { 0 };
    FEATURE_CHECK_PTR(handle, ret, "handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->createFeature(prototype, binding_obj);
}

void FeatureSetManagerUserData(FeatureManagerHandle handle, const char* name, void* data)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->setUserData(name, data);
}

void FeatureSetUriConvertCb(FeatureManagerHandle handle, UriConvertCb cb)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->setUriConvertCb(cb);
}

bool FeatureHasFeature(FeatureManagerHandle handle, FtString feature_method)
{
    FEATURE_CHECK_PTR(handle, false, "manager handle is null !")
    FEATURE_CHECK_PTR(feature_method, false, "feature_method is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->hasFeature(feature_method);
}

void FeatureDumpMemory(FeatureManagerHandle handle, FeatureMemoryDump* dump, void* userdata)
{
    if (handle) {
        FeatureManager* manager = static_cast<FeatureManager*>(handle);
        manager->onDumpMemory(dump, userdata);
    }
}