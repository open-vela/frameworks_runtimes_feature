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
#include "feature_manager_qjs.h"
#include "feature_log.h"
#include "feature_prototype_qjs.h"
#include "feature_utils.h"
#include "promise_manager.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

using namespace FEATURE;

#define CFUNCDATA_FN(f) ((feature_value_t(*)(feature_context_ref ctx, feature_value_t, int, feature_value_t*, int, feature_value_t*))f)

static inline void free_arg(JSContext* ctx, JSValue& arg)
{
    JS_FreeValue(ctx, arg);
}

static inline JSValue undefined_arg(JSContext* ctx)
{
    return JS_UNDEFINED;
}

namespace ferry {

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* proto)
    : FeatureInstance(proto)
    , vm_object_(FEATURE_VALUE_UNDEFINED)
{
    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(weak_self_.ft_value);
    *js_val_ptr = JS_UNDEFINED;
    promise_manager_ = new PromiseManager((JSContext*)ft_context_get_data(prototype()->featureManager()->getFeatureContext()));
}

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* module_proto, VTable* vtable)
    : FeatureInstance(module_proto, vtable)
    , vm_object_(FEATURE_VALUE_UNDEFINED)
{
    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(weak_self_.ft_value);
    *js_val_ptr = JS_UNDEFINED;
    promise_manager_ = new PromiseManager((JSContext*)ft_context_get_data(prototype()->featureManager()->getFeatureContext()));
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

    // invoke callback
    if (proto->description()->native_callbacks && proto->description()->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto->description()->native_callbacks->onDetached(js_ctx, this);
    }

    // release all promises
    promise_manager_->releasePromises();
    delete promise_manager_;
}

feature_value_t FeatureInstanceQjs::createTargetInterface()
{
    FeatureManagerQjs* manager = (FeatureManagerQjs*)(prototype()->featureManager());
    FEATURE_CHECK_NE(manager, nullptr);
    return manager->createTargetInterface(this);
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
    auto callbacks = getCallbacks();
    for (auto& pair : callbacks) {
        feature_mark_value(rt, pair.second->cb, mark_func);
    }

    // mark promies
    promise_manager_->markValues(rt, mark_func);

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
    if (!promise_manager_->removePromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }
    return ret;
}

FtPromiseId FeatureInstanceQjs::addWamrPromise(FeatureType resolve_type, FeatureType reject_type)
{
    return promise_manager_->addWamrPromise(resolve_type, reject_type);
}

int FeatureInstanceQjs::settleWamrPromise(bool resolve, FtPromiseId pid, va_list& ap)
{
    int ret = doSettlePromise(resolve, pid, ap);
    if (!promise_manager_->freeWamrPromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }
    return ret;
}

int FeatureInstanceQjs::doSettlePromise(bool resolve, FtPromiseId pid, va_list& ap)
{
    // get feature instance
    PromiseData* promise_data = promise_manager_->getPromiseData(pid);
    if (!promise_data) {
        FEATURE_LOG_ERROR("get promise data with handle: %" PRId32 " failed !", pid);
        return -1;
    }
    int idx = resolve ? 0 : 1;
    if (feature_is_undefined(promise_data->resolve_funcs[idx])) {
        FEATURE_LOG_ERROR("callback is undefined!");
        return -1;
    }

    FeatureType param_types[2] = { promise_data->resolve_types[idx], FT_VOID };
    CallbackType cb_type = { .header = { .type = COMPLEX_PROMISE, .size = 0 }, .parameters = param_types, .return_type = FT_VOID };
    return doInvokeCallback(&cb_type, promise_data->resolve_funcs[idx], ap, 1, 0);
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

bool FeatureInstanceQjs::argToTarget(va_list &ap, FeatureType ftype, JSValue& target)
{
    JSContext* ctx = getContext();
    void *param = extractVariadicParam(ap, ftype);
    if (!param) {
        FEATURE_LOG_ERROR("extract callback param failed !");
        return false;
    }
    if (!FeatureFFIQjs::convertValueToGuest(this, ftype, param, ctx, target)) {
        FEATURE_LOG_ERROR("convert callback param failed !");
        free(param);
        return false;
    }
    free(param);
    return true;
}

int FeatureInstanceQjs::doInvokeCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int fixed_argc, int rest_argc)
{
    JSContext* js_ctx = getContext();
    if (feature_is_undefined(callback)) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }

    // create argv list and initialize to undefined
    AutoArgs<JSContext*, JSValue> argv(js_ctx, free_arg, undefined_arg, fixed_argc + rest_argc);
    // convert parameters to feature_value_t
    for (int i = 0; i < fixed_argc; i++) {
        FeatureType ftype = callbackType->parameters[i];
        if (!argToTarget(ap, ftype, argv[i])) {
            FEATURE_LOG_ERROR("extract callback param failed !");
            return 0;
        }
    }

    // prepare for rest parameters
    for (int i = fixed_argc; i < fixed_argc + rest_argc; i++) {
        // it must be FtMalloced.
        void* arg = va_arg(ap, void*);
        void* header_ptr = ((char*)arg - FT_OBJ_HEADER_SIZE);
        FTObjHeader* header = (FTObjHeader*)header_ptr;
        if (!FeatureFFIQjs::convertValueToGuest(this, header->featureType, arg, js_ctx, argv[i])) {
            FEATURE_LOG_ERROR("convert callback rest param failed !");
            argv[i] = FEATURE_VALUE_UNDEFINED;
        }
    }

    feature_value_t ret = feature_call(js_ctx, callback, FEATURE_VALUE_UNDEFINED, fixed_argc + rest_argc, argv);
    feature_free_value(js_ctx, ret);
    return 0;
}

}
