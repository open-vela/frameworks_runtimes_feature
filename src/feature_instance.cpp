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
#include "feature_log.h"
#include "feature_utils.h"
#include "feature.h"
#include "feature_ffi.h"
#include "feature_context_private.h"
#include "feature_context_qjs.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>
#include <strings.h>
#include <tuple>

using namespace FEATURE;

namespace ferry {

/////////////////////////////////////////////////
FeatureInstance::FeatureInstance(FeaturePrototype* proto)
    : native(nullptr)
    , proto_(proto)
    , instance_id_(-1)
{
}

FeatureInstance::~FeatureInstance()
{
    // remove opaque binding
    feature_set_opaque(this->weak_self_.js_value, nullptr);
    // release all callbacks
    JSContext* js_ctx = (JSContext*)ft_context_get_data(proto_->ft_ctx);
    for (const auto& callback : callbacks) {
        feature_free_value(js_ctx, callback.second.cb);
    }
    callbacks.clear();

    // release all promises
    for (const auto& pair : promises) {
        FEATURE_LOG_DEBUG("promise: %" PRId32 " freed !", pair.first);
        feature_free_value(js_ctx, pair.second->promise);
        feature_free_value(js_ctx, pair.second->resolveFuncs[0]);
        feature_free_value(js_ctx, pair.second->resolveFuncs[1]);
        free(pair.second);
    }
    promises.clear();

    // check if all instances deleted, then clear proto object
    if (!proto_->hasInstanceAlive()) {
        FEATURE_LOG_INFO("all instance freed, free proto object...");
        auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(proto_->ft_proto);
        *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
    }
}

FeatureCallbackData FeatureInstance::getCallback(FEATURE::FeatureCallbackId id)
{
    if (!callbacks.count(id)) {
        FeatureCallbackData callback;
	callback.cb = FEATURE_VALUE_UNDEFINED;
	callback.cb_type = nullptr;
        return callback;
    }
    return callbacks[id];
}

FEATURE::FeatureCallbackId FeatureInstance::addCallback(feature_value_t value, CallbackType* callbackType)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(proto_->ft_ctx);
    FeatureCallbackData callback;
    callback.cb = feature_dup_value(js_ctx, value);
    callback.cb_type = callbackType;
    callbacks[curr_cid] = callback;
    return curr_cid++;
}

bool FeatureInstance::removeCallback(FEATURE::FeatureCallbackId id)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(proto_->ft_ctx);
    if (!callbacks.count(id)) {
        FEATURE_LOG_ERROR("callback id %d in instance: %p not exist !", id, this);
        return false;
    }
    feature_free_value(js_ctx, callbacks[id].cb);
    callbacks.erase(id);
    return true;
}

ferry::FeaturePromiseData* FeatureInstance::getPromise(FEATURE::FeaturePromiseHandle promiseHandle)
{
    if (!promises.count(promiseHandle)) {
        return nullptr;
    }
    return promises[promiseHandle];
}

FEATURE::FeaturePromiseHandle FeatureInstance::addPromise(FeaturePromiseData* data)
{
    // auto ctx = proto_->ctx;
    FEATURE_CHECK_NE(data, nullptr);
    FEATURE_CHECK_NE(feature_is_undefined(data->promise), true);
    promises[curr_cid] = data;
    return curr_cid++;
}

bool FeatureInstance::removePromise(FEATURE::FeaturePromiseHandle promiseHandle)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(proto_->ft_ctx);
    if (!promises.count(promiseHandle)) {
        FEATURE_LOG_ERROR("promiseHandle %d in instance: %p not exist !", promiseHandle, this);
        return false;
    }
    FeaturePromiseData* data = promises[promiseHandle];
    FEATURE_CHECK_NE(data, nullptr);
    promises.erase(promiseHandle);
    // free js values
    feature_free_value(js_ctx, data->promise);
    feature_free_value(js_ctx, data->resolveFuncs[0]);
    feature_free_value(js_ctx, data->resolveFuncs[1]);
    free(data);
    return true;
}

}

