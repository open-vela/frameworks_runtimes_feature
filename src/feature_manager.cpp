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
#include "feature_context_private.h"
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
    feature_list_initialize(&feature_node_list_);
}

FeatureManager::~FeatureManager()
{
    ft_ctx_ = nullptr;
    runAllTasks(FEATURE_TASK_MODE_FREE);
    checkFeatureInstances();
    feature_list_delete(&feature_node_list_);
}

void FeatureManager::setFeatureContext(ft_context_ref ft_ctx)
{
    ft_ctx_ = ft_ctx;
#ifdef CONFIG_FEATURE_ENABLE_THREAD_CHECKER
    ft_ctx->thread_checker = &thread_checker_;
#endif
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
    std::queue<TaskData> tasks;
    uv_mutex_lock(&mutex_);
    std::swap(tasks, task_queue_);
    uv_mutex_unlock(&mutex_);

    int tasks_size = tasks.size();
    for (int i = 0; i < tasks_size; i++) {
        TaskData task_data = tasks.front();
        task_data.task_cb(mode, task_data.data);
        FeatureFreeInstanceHandle(task_data.instance);
        tasks.pop();
    }
}

void FeatureManager::detachFeatureInstances()
{
    feature_list_node *node, *temp;
    feature_list_for_every_safe(&feature_node_list_, node, temp)
    {
        if (node) {
            FeatureInstance* instance = (FeatureInstance*)(node);
            FEATURE_LOG_DEBUG("feature lazy free, base:%p name:%s", node, instance->description()->name);
            instance->onDetached();
        }
    }
}

void FeatureManager::checkFeatureInstances()
{
    feature_list_node *node, *temp;
    feature_list_for_every_safe(&feature_node_list_, node, temp)
    {
        if (node) {
            FeatureInstance* instance = (FeatureInstance*)(node);
            FEATURE_LOG_WARN("feature manager has been released, feature(base:%p, name:%s) may have memory leaks", node, instance->description()->name);
        }
    }
}

}
