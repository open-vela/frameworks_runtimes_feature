
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
#include "feature_registry.h"

#include <queue>

namespace ferry {

struct TaskData {
    FeatureInstanceHandle instance;
    FeatureTaskCallback task_cb;
    void* data;
};

class FeatureManager {
public:
    FeatureManager(FeatureRegistry* registry);
    virtual ~FeatureManager();

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

    void setUserData(const char* name, void* data) { user_data_[name] = data; }

    void* getUserData(const char* name)
    {
        auto it = user_data_.find(name);
        return it != user_data_.end() ? it->second : nullptr;
    }

    void addTask(FeatureInstanceHandle handle, FeatureTaskCallback task_cb, void* data);

    void runAllTasks(int mode);

private:
    FeatureRegistry* registry_;
    ft_context_ref ft_ctx_;
    const char* pkg_name_ = nullptr;
    const char* env_name_ = nullptr;
    std::map<std::string, void*> user_data_;
    uv_mutex_t mutex_;
    uv_async_t* async_ = nullptr;
    uv_loop_t* loop_ = nullptr;
    std::queue<TaskData> task_queue_;
};

}
#endif // __FEATURE_MANAGE_H__
