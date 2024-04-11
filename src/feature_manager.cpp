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
#include "feature_exports.h"
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
    uv_mutex_init(&mutex_);
}

FeatureManager::~FeatureManager()
{
    ft_ctx_ = nullptr;
    runAllTasks(FEATURE_TASK_MODE_FREE);
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
    async_ = (uv_async_t*)calloc(1, sizeof(uv_async_t));
    uv_async_init(loop_, async_, feature_async_cb);
    async_->data = this;
}

void FeatureManager::unsetUVLoop()
{
    if (async_ && !uv_is_closing((uv_handle_t*)async_)) {
        uv_close((uv_handle_t*)async_, [](uv_handle_t* handler) {
            free(reinterpret_cast<uv_async_t*>(handler));
        });
        async_ = nullptr;
        loop_ = nullptr;
    }
}

void FeatureManager::addTask(FeatureInstanceHandle handle, FeatureTaskCallback task_cb, void* data)
{
    TaskData task_data;
    task_data.instance = FeatureDupInstanceHandle(handle);
    task_data.task_cb = task_cb;
    task_data.data = data;
    uv_mutex_lock(&mutex_);
    task_queue_.push(task_data);
    uv_mutex_unlock(&mutex_);
    if (async_)
        uv_async_send(async_);
}

void FeatureManager::runAllTasks(int mode)
{
    uv_mutex_lock(&mutex_);
    int task_queue_size = task_queue_.size();
    for (int i = 0; i < task_queue_size; i++) {
        TaskData task_data = task_queue_.front();
        task_data.task_cb(mode, task_data.data);
        FeatureFreeInstanceHandle(task_data.instance);
        task_queue_.pop();
    }
    uv_mutex_unlock(&mutex_);
}

}
