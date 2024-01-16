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
#include "feature_manager_qjs.h"
#include "feature_log.h"
#include "feature_prototype_qjs.h"
#include "feature_utils.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

using namespace FEATURE;

#define CFUNCDATA_FN(f) ((feature_value_t(*)(feature_context_ref ctx, feature_value_t, int, feature_value_t*, int, feature_value_t*))f)

namespace ferry {

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* proto)
    : FeatureInstance(proto)
    , PromiseManager((JSContext*)ft_context_get_data(proto->featureManager()->getFeatureContext()))
    , vm_object_(FEATURE_VALUE_UNDEFINED)
    , target_(FEATURE_VALUE_UNDEFINED)
{
    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(weak_self_.ft_value);
    *js_val_ptr = JS_UNDEFINED;
}

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* module_proto, VTable* vtable)
    : FeatureInstance(module_proto, vtable)
    , PromiseManager((JSContext*)ft_context_get_data(module_proto->featureManager()->getFeatureContext()))
    , vm_object_(FEATURE_VALUE_UNDEFINED)
    , target_(FEATURE_VALUE_UNDEFINED)
{
    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(weak_self_.ft_value);
    *js_val_ptr = JS_UNDEFINED;
}

void FeatureInstanceQjs::initialize()
{
    if (isInitialized())
        return;

    FeatureInstance::initialize();
    FeatureManagerQjs* manager = (FeatureManagerQjs*)(prototype()->featureManager());
    FEATURE_CHECK_NE(manager, nullptr);
    target_ = manager->createTargetInterface(this);
}

feature_value_t FeatureInstanceQjs::dupTarget()
{
    JSContext* js_ctx = getContext();
    return feature_dup_value(js_ctx, target_);
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

JSContext* FeatureInstanceQjs::getContext()
{
    return (JSContext*)ft_context_get_data(prototype()->featureManager()->getFeatureContext());
}

FeatureInstanceQjs::~FeatureInstanceQjs()
{
    // remove opaque binding
    auto js_val = FT_VAL_GET_JS_VAL(weak_self_.ft_value);

    auto proto = prototype();
    JSContext* js_ctx = getContext();

    // free weakRef
    if (!JS_IsUndefined(js_val)) {
        feature_set_opaque(js_val, nullptr);
        freeWeakRef();
    }

    // free target instance
    if (!JS_IsUndefined(target_)) {
        feature_free_value(js_ctx, target_);
    }

    // invoke callback
    if (proto->description()->native_callbacks && proto->description()->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto->description()->native_callbacks->onDetached(js_ctx, this);
    }

    // release all promises
    releasePromises();
}

bool FeatureInstanceQjs::checkCallback(FtCallbackId cid)
{
    auto cb_data = getCallbackData(cid);
    if (!cb_data) {
        FEATURE_LOG_DEBUG("callback is undefined !");
        return false;
    } else {
        return true;
    }
}

bool FeatureInstanceQjs::removeCallback(FtCallbackId cid)
{
    return eraseCallback(cid);
}

int FeatureInstanceQjs::getSameCallback(FtCallbackId cid)
{
    return getInitialCallbackId(cid);
}

void FeatureInstanceQjs::markValues(feature_runtime_ref rt, feature_mark_func mark_func)
{
    // mark callbacks
    auto callbacks = getCallbacks();
    for (auto& pair : callbacks) {
        feature_mark_value(rt, pair.second->cb, mark_func);
    }

    // mark promies
    markPromises(rt, mark_func);

    for (auto& pair : prototype()->children()) {
        FeaturePrototypeQjs* proto_qjs = static_cast<FeaturePrototypeQjs*>(pair.second.get());
        auto js_proto = FT_VAL_GET_JS_VAL(proto_qjs->ft_proto());
        feature_mark_value(rt, js_proto, mark_func);
    }
}

bool FeatureInstanceQjs::initWeakRef(feature_value_t feature_object)
{
    if (!prototype()) {
        FEATURE_LOG_ERROR("prototype missing");
        return false;
    }

    auto proto = static_cast<FeaturePrototypeQjs*>(prototype());
    WeakRef* node = &weak_self_;
    weakref_list_initialize(&node->link);
    weakref_list_add_tail(&node->link, &proto->weak_ref_list());

    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(node->ft_value);
    *js_val_ptr = feature_object;
    proto->inc_ref_count();

    return true;
}

void FeatureInstanceQjs::freeWeakRef()
{
    if (!prototype()) {
        FEATURE_LOG_ERROR("prototype missing");
        return;
    }

    auto proto = static_cast<FeaturePrototypeQjs*>(prototype());
    // 遍历proto->weak_ref_list链表，将其中的js_value设置为JSE_UNDEFINED
    WeakRef* node;
    WeakRef* node_temp;
    if (proto->dec_ref_count() <= 0) {
        weakref_list_for_every_entry_safe(&proto->weak_ref_list(), node, node_temp, WeakRef, link)
        {
            auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(node->ft_value);
            *js_val_ptr = FEATURE_VALUE_UNDEFINED;
        }
    }
}

int FeatureInstanceQjs::settlePromise(bool resolve, FtPromiseId pid, va_list& ap)
{
    int ret = doSettlePromise(resolve, pid, ap);
    if (!removePromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }
    return ret;
}

int FeatureInstanceQjs::invokeCallback(FtCallbackId cid, va_list& ap)
{
    auto cb_data = getCallbackData(cid);
    if (!cb_data) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }
    bool has_rest_param = false;
    CallbackType* callbackType = cb_data->type;
    int fixed_argc = getParamCount(callbackType->parameters, &has_rest_param);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return callCallback(cb_data, ap, fixed_argc, 0);
}

int FeatureInstanceQjs::invokeCallbackCount(FtCallbackId cid, va_list& ap, int count)
{
    auto cb_data = getCallbackData(cid);
    if (!cb_data) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }
    bool has_rest_param = false;
    CallbackType* callbackType = cb_data->type;
    int fixed_argc = getParamCount(callbackType->parameters, &has_rest_param);
    if (!has_rest_param || count < fixed_argc) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return callCallback(cb_data, ap, fixed_argc, count - fixed_argc);
}

int FeatureInstanceQjs::doInvokeCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int fixed_argc, int rest_argc)
{
    return invokeJsCallback(callbackType, callback, ap, fixed_argc, rest_argc);
}

}
