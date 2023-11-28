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
#include "promise_manager.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

using namespace FEATURE;

#define CFUNCDATA_FN(f) ((feature_value_t(*)(feature_context_ref ctx, feature_value_t, int, feature_value_t*, int, feature_value_t*))f)

namespace ferry {

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* proto, VTable* vtable)
    : FeatureInstance(proto, vtable)
    , vm_object_(FEATURE_VALUE_UNDEFINED)
{
    promise_manager_ = new PromiseManager((JSContext*)ft_context_get_data(proto->getFeatureManager()->getFeatureContext()));
}

FeatureInstance* FeatureInstanceQjs::createInterface(VTable* vtable)
{
    // null param proto to be fixed
    return new FeatureInstanceQjs(prototype(), vtable);
}

void FeatureInstanceQjs::setVmObject(feature_value_t vm_object)
{
    vm_object_ = vm_object;
}

feature_value_t FeatureInstanceQjs::getVmObject() const
{
    return vm_object_;
}

feature_value_t FeatureInstanceQjs::getFeatureJsvalue(ft_value_t value)
{
    qjs_val_t q_val = FT_VAL_TO_QJS(value);
    return q_val.js_val;
}

FeatureInstanceQjs::~FeatureInstanceQjs()
{
    // remove opaque binding
    auto js_val = FT_VAL_GET_JS_VAL(weak_self_.ft_value);
    feature_set_opaque(js_val, nullptr);
    auto proto_type = prototype();
    JSContext* js_ctx = (JSContext*)ft_context_get_data(proto_type->getFeatureManager()->getFeatureContext());

    // free weakRef
    freeWeakRef();

    // invoke callback
    if (proto_type->description->native_callbacks && proto_type->description->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto_type->description->native_callbacks->onDetached(js_ctx, this);
    }
    // release all callbacks
    for (const auto& callback : callbacks_) {
        feature_free_value(js_ctx, callback.second.cb);
    }
    callbacks_.clear();

    // release all promises
    promise_manager_->releasePromises();
    delete promise_manager_;

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
    free_instance(proto_type);
    for (auto& pair : prototypes_) {
        // clear all interface instances belongs to this instance.
        pair.second->clearAllInstances();
        free_instance(pair.second);
        delete pair.second;
    }
    prototypes_.clear();
}

FeatureCallbackData FeatureInstanceQjs::getCallback(FtCallbackId cid)
{
    if (!callbacks_.count(cid)) {
        FeatureCallbackData callback;
        callback.cb = FEATURE_VALUE_UNDEFINED;
        callback.cb_type = nullptr;
        return callback;
    }
    return callbacks_[cid];
}

FtCallbackId FeatureInstanceQjs::addCallback(feature_value_t value, CallbackType* callbackType)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->getFeatureManager()->getFeatureContext());
    FeatureCallbackData callback;
    callback.cb = feature_dup_value(js_ctx, value);
    callback.cb_type = callbackType;
    callbacks_[++curr_cid_] = callback;
    return curr_cid_;
}

bool FeatureInstanceQjs::checkCallback(FtCallbackId cid)
{
    const auto callback = getCallback(cid);
    if (feature_is_undefined(callback.cb)) {
        FEATURE_LOG_DEBUG("callback in undefined !");
        return false;
    } else {
        return true;
    }
}

bool FeatureInstanceQjs::removeCallback(FtCallbackId cid)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->getFeatureManager()->getFeatureContext());
    if (!callbacks_.count(cid)) {
        FEATURE_LOG_ERROR("callback id %d in instance: %p not exist !", cid, this);
        return false;
    }
    feature_free_value(js_ctx, callbacks_[cid].cb);
    callbacks_.erase(cid);
    return true;
}

int FeatureInstanceQjs::getSameCallback(FtCallbackId cid)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->getFeatureManager()->getFeatureContext());
    for (auto it = callbacks_.begin(); it != callbacks_.end(); it++) {
        if (it->first != cid && feature_is_same_value(js_ctx, it->second.cb, callbacks_[cid].cb)) {
            return it->first;
        }
    }
    return 0;
}

feature_value_t FeatureInstanceQjs::getPromise(FtPromiseId pid)
{
    return promise_manager_->getPromise(pid);
}

FtPromiseId FeatureInstanceQjs::addPromise(FeatureType resolve_type, FeatureType reject_type)
{
    return promise_manager_->addPromise(resolve_type, reject_type);
}

void FeatureInstanceQjs::markValues(feature_runtime_ref rt, feature_mark_func mark_func)
{
    // mark callbacks
    for (auto& pair : callbacks_) {
        feature_mark_value(rt, pair.second.cb, mark_func);
    }

    // mark promies
    promise_manager_->markValues(rt, mark_func);

    for (auto& pair : prototypes_) {
        auto js_proto = FT_VAL_GET_JS_VAL(pair.second->ft_proto);
        feature_mark_value(rt, js_proto, mark_func);
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

void FeatureInstanceQjs::freeWeakRef()
{
    if (!prototype()) {
        FEATURE_LOG_ERROR("freeWeakRef() get FeatureInstance failed");
        return;
    }

    auto proto = prototype();
    // 遍历proto->weak_ref_list链表，将其中的js_value设置为JSE_UNDEFINED
    WeakRef* node;
    WeakRef* node_temp;
    if (--proto->weak_ref_count <= 0) {
        weakref_list_for_every_entry_safe(&proto->weak_ref_list, node, node_temp, WeakRef, link)
        {
            auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(node->ft_value);
            *js_val_ptr = FEATURE_VALUE_UNDEFINED;
        }
    }
}

int FeatureInstanceQjs::settlePromise(bool resolve, FtPromiseId pid, va_list& ap)
{
    // get feature instance
    FeaturePromiseData* promiseData = promise_manager_->getPromiseData(pid);
    if (!promiseData) {
        FEATURE_LOG_ERROR("get promise data with handle: %" PRId32 " failed !", pid);
        return -1;
    }
    int idx = resolve ? 0 : 1;
    if (feature_is_undefined(promiseData->resolveFuncs[idx])) {
        FEATURE_LOG_ERROR("callback is undefined!");
        return -1;
    }

    FeatureType param_types[2] = { promiseData->resolveTypes[idx], FT_VOID };
    CallbackType cb_type = { .header = { .type = COMPLEX_PROMISE, .size = 0 }, .parameters = param_types, .return_type = FT_VOID };
    int ret = doInvokeCallback(&cb_type, promiseData->resolveFuncs[idx], ap, 1, 0);
    if (!promise_manager_->removePromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }

    return ret;
}

int FeatureInstanceQjs::invokeCallback(FtCallbackId cid, va_list& ap)
{
    const auto callback = getCallback(cid);
    bool has_rest_param = false;
    CallbackType* callbackType = callback.cb_type;
    int method_param_count = getParamCount(callbackType->parameters, &has_rest_param);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return doInvokeCallback(callbackType, callback.cb, ap, method_param_count, 0);
}

int FeatureInstanceQjs::invokeCallbackCount(FtCallbackId cid, va_list& ap, int count)
{
    const auto callback = getCallback(cid);
    bool has_rest_param = false;
    CallbackType* callbackType = callback.cb_type;
    int method_param_count = getParamCount(callbackType->parameters, &has_rest_param);
    if (!has_rest_param || count < method_param_count) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return doInvokeCallback(callbackType, callback.cb, ap, method_param_count, count - method_param_count);
}

int FeatureInstanceQjs::doInvokeCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int method_param_count, int rest_param_count)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->getFeatureManager()->getFeatureContext());
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
