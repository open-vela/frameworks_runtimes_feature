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
#include "feature_manager.h"
#include "feature_context.h"
#include "feature_instance.h"
#include "feature_log.h"
#include "feature_prototype.h"
#include "feature_utils.h"

#include <string.h>

using namespace FEATURE;

namespace ferry {

/////////////////////////////////////////////////
FeatureManager::FeatureManager(FeatureRegistry* registry)
    : registry_(registry)
    , ft_ctx_(nullptr)
{
    uv_mutex_init(&mutex);
}

FeatureManager::~FeatureManager()
{
    ft_ctx_ = nullptr;
}

void FeatureManager::setFeatureContext(ft_context_ref ft_ctx)
{
    ft_ctx_ = ft_ctx;
}

static void feature_async_cb(uv_async_t* handle)
{
    FeatureManager* m = (FeatureManager*)handle->data;
    m->runAllTasks(FEATURE_TASK_MODE_NORMAL);
    return;
}

void FeatureManager::setUVLoop(uv_loop_t* loop)
{
    loop_ = loop;
    uv_async_init(loop_, &async, feature_async_cb);
    async.data = this;
}

void FeatureManager::setPackageName(const char* package_name)
{
    package_name_ = package_name;
}

const char* FeatureManager::getPackageName() const
{
    return package_name_;
}

void FeatureManager::setEnvironmentName(const char* environment_name)
{
    environment_name_ = environment_name;
}

void FeatureManager::lockAsync()
{
    uv_mutex_lock(&mutex);
}

void FeatureManager::unlockAsync()
{
    uv_mutex_unlock(&mutex);
}

void FeatureManager::runAllTasks(int mode)
{
    uv_mutex_lock(&mutex);
    for (const auto& pair : getFeatureRegistry()->getRegisteredFeatures()) {
        FeaturePrototype* prototype = pair.second.second;
        if (prototype) {
            for (const auto& instance : prototype->instances) {
                if (instance) {
                    instance->runAsyncTasks(mode);
                }
            }
        }
    }

    uv_mutex_unlock(&mutex);
}

}