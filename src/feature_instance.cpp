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
#include "feature_instance.h"
#include "feature_context.h"
#include "feature_log.h"

#include <string.h>
using namespace FEATURE;

namespace ferry {

/////////////////////////////////////////////////
FeatureInstance::FeatureInstance(FeaturePrototype* proto, VTable* vtable)
    : native(nullptr)
    , proto_(proto)
    , instance_id_(-1)
    , vtable_(vtable)
{
}

FeatureInstance::~FeatureInstance()
{
    if (vtable_ && vtable_->finalizer) {
        dtor_func dtor = (dtor_func)(vtable_->finalizer);
        dtor(this);
    }

    FeatureManager* manager = prototype()->getFeatureManager();
    if (manager) {
        manager->lockAsync();
        runAsyncTasks(FEATURE_TASK_MODE_FREE);
        manager->unlockAsync();
    }
}

void FeatureInstance::addCallback(FeatureTaskCallback task_cb, void* data)
{
    FeatureManager* manager = prototype()->getFeatureManager();
    manager->lockAsync();
    TaskData task_data;
    task_data.task_cb = task_cb;
    task_data.data = data;
    task_queue_.push(task_data);
    manager->unlockAsync();
}

void FeatureInstance::sendAsnyc()
{
    FeatureManager* manager = prototype()->getFeatureManager();
    manager->lockAsync();
    uv_async_send(&manager->async);
    manager->unlockAsync();
}

void FeatureInstance::runAsyncTasks(int task_run_mode)
{

    int task_queue_size = task_queue_.size();
    for (int i = 0; i < task_queue_size; i++) {
        TaskData task_data = task_queue_.front();
        task_data.task_cb(task_run_mode, task_data.data);
        task_queue_.pop();
    }
}

} // namespace ferry
