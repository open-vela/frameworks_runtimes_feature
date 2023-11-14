
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
#include "feature_utils.h"

namespace ferry {

// class FeatureRegistry;

class FeatureManager {
public:
    FeatureManager(FeatureRegistry* registry);
    virtual ~FeatureManager();

    FeatureRegistry* getFeatureRegistry() { return registry_; }
    ft_context_ref getFeatureContext() { return ft_ctx_; }
    void setFeatureContext(ft_context_ref ft_ctx);
    uv_loop_t* getUVLoop() const { return loop_; }

    void setUVLoop(uv_loop_t* loop);

    void setPackageName(const char* package_name);

    const char* getPackageName() const;

    void setEnvironmentName(const char* environment_name);

    const char* getEnvironmentName() const { return environment_name_; }

    void setUserData(const char* name, void* data) { user_data_[name] = data; }

    void* getUserData(const char* name)
    {
        auto it = user_data_.find(name);
        return it != user_data_.end() ? it->second : nullptr;
    }

    void runAllTasks(int mode);

    void lockAsync();

    void unlockAsync();

    uv_async_t async;
    uv_mutex_t mutex;

private:
    FeatureRegistry* registry_;
    ft_context_ref ft_ctx_;
    uv_loop_t* loop_ = nullptr;
    const char* package_name_ = nullptr;
    const char* environment_name_ = nullptr;
    std::map<std::string, void*> user_data_;
};

}
#endif // __FEATURE_MANAGE_H__
