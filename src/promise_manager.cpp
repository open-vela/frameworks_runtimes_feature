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

#include "promise_manager.h"
#include "feature_common.h"
#include "feature_log.h"

using namespace FEATURE;

namespace ferry {

PromiseManager::PromiseManager(JSContext* js_ctx)
 : js_ctx_(js_ctx)
{
}

PromiseManager::~PromiseManager()
{
    releasePromises();
}

FtPromiseId PromiseManager::addPromise(FeatureType resolve_type, FeatureType reject_type)
{
    PromiseData* data = (PromiseData*)malloc(sizeof(PromiseData));
    data->promise = FEATURE_VALUE_UNDEFINED;
    data->resolve_funcs[0] = FEATURE_VALUE_UNDEFINED;
    data->resolve_funcs[1] = FEATURE_VALUE_UNDEFINED;
    data->resolve_types[0] = resolve_type;
    data->resolve_types[1] = reject_type;

    feature_value_t promise = feature_promise_capability(js_ctx_, data->resolve_funcs);
    if (feature_is_exception(promise)) {
        feature_free_value(js_ctx_, data->resolve_funcs[0]);
        feature_free_value(js_ctx_, data->resolve_funcs[1]);
        feature_free_value(js_ctx_, promise);
        free(data);
        return -1;
    }
    data->promise = promise;
    promises_[curr_pid_] = data;
    return curr_pid_++;
}

bool PromiseManager::removePromise(FtPromiseId pid)
{
    if (!promises_.count(pid)) {
        FEATURE_LOG_ERROR("pid %d in instance: %p not exist !", pid, this);
        return false;
    }
    PromiseData* data = promises_[pid];
    FEATURE_CHECK_NE(data, nullptr);
    promises_.erase(pid);
    // free js values
    feature_free_value(js_ctx_, data->promise);
    feature_free_value(js_ctx_, data->resolve_funcs[0]);
    feature_free_value(js_ctx_, data->resolve_funcs[1]);
    free(data);
    return true;
}

void PromiseManager::releasePromises()
{
    for (const auto& pair : promises_) {
        FEATURE_LOG_DEBUG("promise: %d freed !", pair.first);
        PromiseData* data = pair.second;
        feature_free_value(js_ctx_, data->promise);
        feature_free_value(js_ctx_, data->resolve_funcs[0]);
        feature_free_value(js_ctx_, data->resolve_funcs[1]);
        free(data);
    }
    promises_.clear();
}

PromiseData* PromiseManager::getPromiseData(FtPromiseId pid)
{
    if (!promises_.count(pid)) {
        return nullptr;
    }
    return promises_[pid];
}

feature_value_t PromiseManager::getPromise(FtPromiseId pid)
{
    PromiseData* data = getPromiseData(pid);
    if (!data)
        return FEATURE_VALUE_UNDEFINED;

    return data->promise;
}

void PromiseManager::markValues(feature_runtime_ref rt, feature_mark_func mark_func)
{
    // mark promies
    for (auto& pair : promises_) {
        feature_mark_value(rt, pair.second->promise, mark_func);
        feature_mark_value(rt, pair.second->resolve_funcs[0], mark_func);
        feature_mark_value(rt, pair.second->resolve_funcs[1], mark_func);
    }
}

}
