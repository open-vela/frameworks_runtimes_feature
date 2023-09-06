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
#include "feature_instance_qjs.h"
#include "feature_context_qjs.h"
#include "feature_ffi_qjs.h"
#include "feature_log.h"
#include "feature_utils.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

using namespace FEATURE;

#define CFUNCDATA_FN(f) ((feature_value_t(*)(feature_context_ref ctx, feature_value_t, int, feature_value_t*, int, feature_value_t*))f)

namespace ferry {

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* proto, NativeFunc* vtable, int vtable_size)
    : FeatureInstance(proto, vtable, vtable_size)
{
}

FeatureInstanceQjs::~FeatureInstanceQjs()
{
    // remove opaque binding
    auto js_val = FT_VAL_GET_JS_VAL(weak_self_.ft_value);
    feature_set_opaque(js_val, nullptr);
    auto proto = prototype();
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);

    // 遍历proto->weak_ref_list链表，将其中的js_value设置为JSE_UNDEFINED
    WeakRef* node;
    WeakRef* node_temp;
    weakref_list_for_every_entry_safe(&proto->weak_ref_list, node, node_temp, WeakRef, link)
    {
        auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(node->ft_value);
        *js_val_ptr = FEATURE_VALUE_UNDEFINED;
    }

    // invoke callback
    if (proto->description->native_callbacks && proto->description->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto->description->native_callbacks->onDetached(js_ctx, this);
    }
    // release all callbacks
    for (const auto& callback : callbacks_) {
        feature_free_value(js_ctx, callback.second.cb);
    }
    callbacks_.clear();

    // release all promises
    releasePromises();
    auto free_instance = [js_ctx](FeaturePrototype* proto) {
        if (proto && !proto->hasInstanceAlive()) {
            FEATURE_LOG_INFO("all instance freed, free proto object...");
            auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(proto->ft_proto);
            if (!feature_is_undefined(*js_proto_ptr)) {
                feature_free_value(js_ctx, *js_proto_ptr);
                *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
            }
        }
    };
    // check if all instances deleted, then clear proto object
    free_instance(prototype());
    for (auto& pair : prototypes) {
        free_instance(pair.second);
    }
}

FeatureCallbackData FeatureInstanceQjs::getCallback(FeatureCallbackId id)
{
    if (!callbacks_.count(id)) {
        FeatureCallbackData callback;
        callback.cb = FEATURE_VALUE_UNDEFINED;
        callback.cb_type = nullptr;
        return callback;
    }
    return callbacks_[id];
}

FeatureCallbackId FeatureInstanceQjs::addCallback(feature_value_t value, CallbackType* callbackType)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);
    FeatureCallbackData callback;
    callback.cb = feature_dup_value(js_ctx, value);
    callback.cb_type = callbackType;
    callbacks_[curr_cid_] = callback;
    return curr_cid_++;
}

bool FeatureInstanceQjs::removeCallback(FeatureCallbackId id)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);
    if (!callbacks_.count(id)) {
        FEATURE_LOG_ERROR("callback id %d in instance: %p not exist !", id, this);
        return false;
    }
    feature_free_value(js_ctx, callbacks_[id].cb);
    callbacks_.erase(id);
    return true;
}

FeaturePromiseData* FeatureInstanceQjs::getPromiseData(FeaturePromiseHandle promiseHandle)
{
    if (!promises_.count(promiseHandle)) {
        return nullptr;
    }
    return promises_[promiseHandle];
}

feature_value_t FeatureInstanceQjs::getPromise(FeaturePromiseHandle promiseHandle)
{
    FeaturePromiseData* data = getPromiseData(promiseHandle);
    if (!data)
        return FEATURE_VALUE_UNDEFINED;

    return data->promise;
}

FeaturePromiseHandle FeatureInstanceQjs::addPromise(FeatureType resolve_type, FeatureType reject_type)
{
    FeaturePromiseData* data = (FeaturePromiseData*)malloc(sizeof(FeaturePromiseData));
    data->promise = FEATURE_VALUE_UNDEFINED;
    data->resolveFuncs[0] = FEATURE_VALUE_UNDEFINED;
    data->resolveFuncs[1] = FEATURE_VALUE_UNDEFINED;
    data->resolveTypes[0] = resolve_type;
    data->resolveTypes[1] = reject_type;

    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);  // to be fixed
    feature_value_t promise = feature_promise_capability(js_ctx, data->resolveFuncs);
    if (feature_is_exception(promise)) {
        feature_free_value(js_ctx, data->resolveFuncs[0]);
        feature_free_value(js_ctx, data->resolveFuncs[1]);
        feature_free_value(js_ctx, promise);
        free(data);
        return -1;
    }
    data->promise = promise;
    promises_[curr_cid_] = data;
    return curr_cid_++;
}

bool FeatureInstanceQjs::removePromise(FeaturePromiseHandle promiseHandle)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);
    if (!promises_.count(promiseHandle)) {
        FEATURE_LOG_ERROR("promiseHandle %d in instance: %p not exist !", promiseHandle, this);
        return false;
    }
    FeaturePromiseData* data = promises_[promiseHandle];
    FEATURE_CHECK_NE(data, nullptr);
    promises_.erase(promiseHandle);
    // free js values
    feature_free_value(js_ctx, data->promise);
    feature_free_value(js_ctx, data->resolveFuncs[0]);
    feature_free_value(js_ctx, data->resolveFuncs[1]);
    free(data);
    return true;
}

void FeatureInstanceQjs::releasePromises() {
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);
    for (const auto& pair : promises_) {
        FEATURE_LOG_DEBUG("promise: %" PRId32 " freed !", pair.first);
        feature_free_value(js_ctx, pair.second->promise);
        feature_free_value(js_ctx, pair.second->resolveFuncs[0]);
        feature_free_value(js_ctx, pair.second->resolveFuncs[1]);
        free(pair.second);
    }
    promises_.clear();
}

void FeatureInstanceQjs::markValues(feature_runtime_ref rt, feature_mark_func mark_func) {
    // mark callbacks
    for (auto& pair : callbacks_) {
        feature_mark_value(rt, pair.second.cb, mark_func);
    }

    // mark promies
    for (auto& pair : promises_) {
        feature_mark_value(rt, pair.second->promise, mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[0], mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[1], mark_func);
    }
}

bool FeatureInstanceQjs::initWeakRef(feature_value_t feature_object)
{
    if (!prototype()) {
        FEATURE_LOG_ERROR("WeakRefInit() get FeatureInstance failed");
        return false;
    }

    auto proto = prototype();
    WeakRef* node = &weak_self_;
    weakref_list_initialize(&node->link);
    weakref_list_add_tail(&node->link, &proto->weak_ref_list);

    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(node->ft_value);
    *js_val_ptr = feature_object;
    proto->weak_ref_count++;

    return true;
}

int FeatureInstanceQjs::settlePromise(bool resolve, FeaturePromiseHandle promiseHandle, va_list& ap)
{
    // get feature instance
    FeaturePromiseData* promiseData = getPromiseData(promiseHandle);
    if (!promiseData) {
        FEATURE_LOG_ERROR("get promise data with handle: %" PRId32 " failed !", promiseHandle);
        return -1;
    }
    int idx = resolve ? 0 : 1;
    if (feature_is_undefined(promiseData->resolveFuncs[idx])) {
        FEATURE_LOG_ERROR("callback in undefined !");
        return -1;
    }

    FeatureType param_types[2] = { promiseData->resolveTypes[idx], FT_VOID };
    CallbackType cb_type = { .header = { .type = COMPLEX_PROMISE, .size = 0 }, .parameters = param_types, .return_type = FT_VOID };
    return doInvokeCallback(&cb_type, promiseData->resolveFuncs[idx], ap, 1, 0);
}

int FeatureInstanceQjs::invokeCallback(int cid, va_list& ap)
{
    const auto callback = getCallback(cid);
    bool has_rest_param = false;
    CallbackType* callbackType = callback.cb_type;
    int method_param_count = getParamCount(callbackType->parameters, &has_rest_param);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with InvokeFeatureCallbackCount!");
        return -1;
    }

    return doInvokeCallback(callbackType, callback.cb, ap, method_param_count, 0);
}

int FeatureInstanceQjs::invokeCallbackCount(int cid, va_list& ap, int count)
{
    const auto callback = getCallback(cid);
    bool has_rest_param = false;
    CallbackType* callbackType = callback.cb_type;
    int method_param_count = getParamCount(callbackType->parameters, &has_rest_param);
    if (!has_rest_param || count < method_param_count) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with InvokeFeatureCallbackCount!");
        return -1;
    }

    return doInvokeCallback(callbackType, callback.cb, ap, method_param_count, count - method_param_count);
}

int FeatureInstanceQjs::doInvokeCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int method_param_count, int rest_param_count)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);
    bool got_error = false;
    feature_value_t ret = FEATURE_VALUE_UNDEFINED;

    if (feature_is_undefined(callback)) {
        FEATURE_LOG_ERROR("callback in undefined !");
        return -1;
    }
    // create argv list and initialize to undefined
    feature_value_t* argv = new feature_value_t[method_param_count + rest_param_count];
    for (int i = 0; i < method_param_count + rest_param_count; i++) {
        argv[i] = FEATURE_VALUE_UNDEFINED;
    }

    do {
        // convert parameters to feature_value_t
        for (int i = 0; i < method_param_count; i++) {
            FeatureType featureType = callbackType->parameters[i];
            void* ptr = exactVariadicParameter(ap, featureType);
            if (!ptr) {
                got_error = true;
                break;
            }
            if (!FeatureFFIQjs::convertValueToGuest(this, featureType, ptr, js_ctx, argv[i])) {
                FEATURE_LOG_ERROR("convert callback param failed !");
                free(ptr);
                got_error = true;
                break;
            }
            free(ptr);
        }
        if (got_error) {
            FEATURE_LOG_ERROR("invoke callback failed !");
            break;
        }
        // prepare for rest parameters
        for (int i = method_param_count; i < method_param_count + rest_param_count; i++) {
            // it must be FtMalloced.
            void* arg = va_arg(ap, void*);
            void* header_ptr = ((char*)arg - FT_OBJ_HEADER_SIZE);
            FTObjHeader* header = (FTObjHeader*)header_ptr;
            if (!FeatureFFIQjs::convertValueToGuest(this, header->featureType, arg, js_ctx, argv[i])) {
                FEATURE_LOG_ERROR("convert callback rest param failed !");
                argv[i] = FEATURE_VALUE_UNDEFINED;
            }
        }

        ret = feature_call(js_ctx, callback, FEATURE_VALUE_UNDEFINED, method_param_count + rest_param_count, argv);
    } while (0);

    for (int i = 0; i < method_param_count + rest_param_count; i++) {
        feature_free_value(js_ctx, argv[i]);
    }
    delete[] argv;
    feature_free_value(js_ctx, ret);

    return 0;
}

}
