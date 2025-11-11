
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

#ifndef __FEATURE_MANAGE_H__
#define __FEATURE_MANAGE_H__

#include "feature_description.h"
#include "feature_list.h"
#include "feature_main_exports.h"
#include "feature_prototype.h"
#include "feature_registry.h"
#include "permissions_manager.h"
#include "thread_checker.h"

#include <list>
#include <map>
#include <queue>
#include <string>

namespace feature_framework {

class ArrayBuffer;
class ArrayBufferCreateParams;

struct TaskData {
    FeatureInstanceHandle instance;
    union {
        FeatureTaskCallback task_cb;
        FeatureTaskCallbackExt task_cb_ext;
    };
    uint64_t data;
    bool is_ext;
};

struct UserDataEntry {
    void* data;
    ManagerUserdataFreeCallback free_cb;
};

class FeatureManager {
public:
    FeatureManager(FeatureRegistry* registry);
    virtual ~FeatureManager();

    static FeatureManager* CreateFeatureManager(FeatureManagerCreateInfo* pinfo);

    virtual ft_value_t featureRequire(ft_value_t binding_obj, const char* name) { return ft_undefined(ft_ctx_); }

    virtual ft_value_t findFeature(const char* name) { return ft_undefined(ft_ctx_); }

    virtual ft_value_t createFeature(ft_value_t prototype, ft_value_t binding_obj) { return ft_undefined(ft_ctx_); }

    virtual bool init() { return true; }

    void registrySetFeatureRegisteredCB(std::function<bool(const FeatureDescription* description)>&& onFeatureRegistered)
    {
        registry_->setOnFeatureRegisteredCB(std::move(onFeatureRegistered));
    }

    virtual void uninit() { }

    virtual ArrayBuffer* createArrayBuffer(ArrayBufferCreateParams& params) { return nullptr; }

    FeatureRegistry* getFeatureRegistry() { return registry_; }

    ft_context_ref getFeatureContext() { return ft_ctx_; }

    void setFeatureContext(ft_context_ref ft_ctx);

    uv_loop_t* getUVLoop() const { return loop_; }

    void setUVLoop(uv_loop_t* loop);

    void unsetUVLoop();

    void setPackageName(const char* pkg_name) { pkg_name_ = pkg_name; }

    const char* packageName() const { return pkg_name_; };

    void setEnvName(const char* env_name) { env_name_ = env_name; }

    const char* envName() const { return env_name_; }

    void setPackageVesion(const char* pkg_version) { pkg_version_ = pkg_version; }

    const char* packageVesion() const { return pkg_version_; };

    void setUserData(const char* name, void* data, ManagerUserdataFreeCallback free_cb = nullptr)
    {
        user_data_[name] = UserDataEntry { data, free_cb };
    }

    void* getUserData(const char* name)
    {
        auto it = user_data_.find(name);
        return it != user_data_.end() ? it->second.data : nullptr;
    }

    bool hasUserData(const char* name)
    {
        auto it = user_data_.find(name);
        return it != user_data_.end();
    }

    void addTask(FeatureInstanceHandle handle, FeatureTaskCallback task_cb, void* data);

    void addTaskExt(FeatureInstanceHandle handle, FeatureTaskCallbackExt task_cb_ext, uint64_t data);

    void runAllTasks(int mode);

    void detachFeatureInstances();

    void checkFeatureInstances();

    feature_list_node* getFeatureNodeList() { return &feature_node_list_; }

    void setArgsErrorCb(ArgsErrorCb cb, void* data)
    {
        args_error_cb_ = cb;
        args_error_data_ = data;
    }
    ArgsErrorCb argsErrorCb() { return args_error_cb_; }
    void* argsErrorData() { return args_error_data_; }
    void setUriConvertCb(UriConvertCb cb) { uri_convert_cb_ = cb; }
    UriConvertCb uriConvertCb() { return uri_convert_cb_; }

    bool hasFeature(const std::string& feature_method);

    void onDumpMemory(FeatureMemoryDump* dump, void* userdata);

    PermissionsManager& permissionsManager() { return perms_manager_; }

    std::map<std::string, FeaturePrototype*>& getFeaturePrototypes() { return prototypes_; }

private:
    void enqueueTask(TaskData& task_data);
    void clearUserData();

    FeatureRegistry* registry_;
    std::map<std::string, FeaturePrototype*> prototypes_;
    ft_context_ref ft_ctx_;
    const char* pkg_name_ = nullptr;
    const char* env_name_ = nullptr;
    const char* pkg_version_ = nullptr;
    feature_list_node feature_node_list_;
    uv_mutex_t mutex_;
    uv_async_t* async_ = nullptr;
    uv_loop_t* loop_ = nullptr;
    std::map<std::string, UserDataEntry> user_data_;
    std::queue<TaskData> task_queue_;
    ArgsErrorCb args_error_cb_ = nullptr;
    void* args_error_data_ = nullptr;
    UriConvertCb uri_convert_cb_ = nullptr;
    ThreadChecker thread_checker_;
    PermissionsManager perms_manager_;
};

} // namespace feature_framework
#endif // __FEATURE_MANAGE_H__
