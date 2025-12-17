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
#include "feature_exports.h"
#include "feature_ffi.h"
#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_prototype_qjs.h"
#include "feature_utils.h"
#include "thread_checker.h"
// clang-format off
#include "backend/qjs/value_translator_qjs.h"
#include "feature_convertor_templates.h"
// clang-format on

#include <cstdarg>
#include <cstdint>
#include <string.h>

#define CFUNCDATA_FN(f) ((feature_value_t(*)(feature_context_ref ctx, feature_value_t, int, feature_value_t*, int, feature_value_t*))f)

static inline void free_arg(JSContext* ctx, JSValue& arg)
{
    JS_FreeValue(ctx, arg);
}

static inline JSValue undefined_arg(JSContext* ctx)
{
    return JS_UNDEFINED;
}

static bool arg_to_target(JSContext* js_ctx, va_list& ap, FeatureType ftype, JSValue& target)
{
    void* param = feature_framework::extractVariadicParam(ap, ftype);
    if (!param) {
        FEATURE_LOG_ERROR("extract callback param failed !");
        return false;
    }
    if (!feature_framework::convertValueToTarget(ftype, js_ctx, param, target)) {
        FEATURE_LOG_ERROR("convert callback param failed !");
        free(param);
        return false;
    }
    free(param);
    return true;
}

namespace feature_framework {

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* proto)
    : FeatureInstance(proto, proto->description())
    , PromiseManager((JSContext*)ft_context_get_data(proto->featureManager()->getFeatureContext()))
    , vm_object_(FEATURE_VALUE_UNDEFINED)
    , target_(FEATURE_VALUE_UNDEFINED)
{
    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(weak_self_.ft_value);
    *js_val_ptr = JS_UNDEFINED;
}

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* module_proto, VTable* vtable)
    : FeatureInstance(module_proto, vtable, module_proto->description())
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
    onDetached();
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

void FeatureInstanceQjs::markValues(feature_runtime_ref rt, feature_mark_func mark_func)
{
    // mark callbacks
    auto callbacks = getCallbacks();
    for (auto& pair : callbacks) {
        feature_mark_value(rt, pair.second->cb, mark_func);
    }

    // mark promies
    markPromises(rt, mark_func);
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
    // 把node->link添加到proto->weak_ref_list()的尾部
    weakref_list_add_tail(&proto->weak_ref_list(), &node->link);

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

    // 从&proto->weak_ref_list()里面找到并删除instance对应的node节点
    weakref_list_for_every_entry_safe(&proto->weak_ref_list(), node, node_temp, WeakRef, link)
    {
        if (node == &weak_self_) {
            FEATURE_LOG_DEBUG("node is %p, &node->link is %p", node, &node->link);
            auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(node->ft_value);
            *js_val_ptr = FEATURE_VALUE_UNDEFINED;
            weakref_list_delete(&node->link);
            proto->dec_ref_count();
            break;
        }
    }
}

int FeatureInstanceQjs::resolvePromise(FtPromiseId pid, va_list& ap)
{
    THREAD_CHECK(featureManager()->getFeatureContext()->thread_checker);
    int ret = doResolvePromise(pid, ap);
    if (!removePromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }
    return ret;
}

int FeatureInstanceQjs::rejectPromise(FtPromiseId pid, int code, const char* msg)
{
    int ret = doRejectPromise(pid, code, msg);
    if (!removePromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }
    return ret;
}

int FeatureInstanceQjs::getPromiseType(FtPromiseId pid)
{
    return doGetPromiseType(pid);
}

int FeatureInstanceQjs::invokeCallback(FtCallbackId cid, va_list& ap)
{
    if (cid <= 0) {
        FEATURE_LOG_DEBUG("callback is undefined !");
        return -1;
    }
    auto cb_data = getCallbackData(cid);
    if (!cb_data) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }
    bool has_rest_param = false;
    int32_t int32_count = 0;
    const CallbackType* callbackType = cb_data->type;
    int fixed_argc = getParamCount(callbackType->parameters, &has_rest_param, nullptr, &int32_count);
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
    const CallbackType* callbackType = cb_data->type;
    int32_t int32_count = 0;
    int fixed_argc = getParamCount(callbackType->parameters, &has_rest_param, nullptr, &int32_count);
    if (!has_rest_param || count < fixed_argc) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return callCallback(cb_data, ap, fixed_argc, count - fixed_argc);
}

bool FeatureInstanceQjs::emitEvent(FtEventId eid, va_list& ap)
{
    if (eid <= 0) {
        FEATURE_LOG_DEBUG("event is undefined !");
        return false;
    }
    auto ev_data = getEventData(eid);
    if (!ev_data) {
        FEATURE_LOG_ERROR("event is undefined !");
        return false;
    }
    bool has_rest_param = false;
    int32_t int32_count = 0;
    const MemberEvent* member_event = ev_data->memberEvent();
    int fixed_argc = getParamCount(member_event->parameters, &has_rest_param, nullptr, &int32_count);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("wrong param count!");
        return false;
    }

    return doEmitEvent(ev_data, ap, fixed_argc, 0);
}

void FeatureInstanceQjs::setEventChangeListener(FeatureEventChangeListener listener)
{
    doSetEventChangeListener(listener, this);
}

FtEventId FeatureInstanceQjs::getEventId(const char* name)
{
    return doGetEventId(name);
}

const char* FeatureInstanceQjs::getEventName(FtEventId eid)
{
    return doGetEventName(eid);
}

int FeatureInstanceQjs::getEventCallbackCount(FtEventId eid)
{
    return doGetEventCallbackCount(eid);
}

void FeatureInstanceQjs::throwError(const char* msg)
{
    THREAD_CHECK(featureManager()->getFeatureContext()->thread_checker);
    setErrorMsg(msg);
}

int FeatureInstanceQjs::doInvokeCallback(const FeatureType* param_types, feature_value_t callback, va_list& ap, int fixed_argc, int rest_argc)
{
    THREAD_CHECK(featureManager()->getFeatureContext()->thread_checker);

    JSContext* js_ctx = getContext();
    if (feature_is_undefined(callback)) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }

    // create argv list and initialize to undefined
    AutoArgs<JSContext*, JSValue> argv(js_ctx, free_arg, undefined_arg, fixed_argc + rest_argc);
    // convert params to feature_value_t
    for (int i = 0; i < fixed_argc; i++) {
        FeatureType ftype = param_types[i];
        if (!arg_to_target(js_ctx, ap, ftype, argv[i])) {
            FEATURE_LOG_ERROR("extract callback param failed !");
            return -1;
        }
    }

    // prepare for rest params
    for (int i = fixed_argc; i < fixed_argc + rest_argc; i++) {
        // it must be FtMalloced.
        void* arg = va_arg(ap, void*);
        void* header_ptr = ((char*)arg - FT_OBJ_HEADER_SIZE);
        FTObjHeader* header = (FTObjHeader*)header_ptr;

        if (header->type == MEMORY_FEATURE_TYPE) {
            FeatureType ftype = *(FeatureType*)((char*)header - sizeof(FeatureType));
            if (!convertValueToTarget(ftype, js_ctx, FT_IS_REFERENCE(ftype) ? &arg : arg, argv[i])) {
                FEATURE_LOG_ERROR("convert callback rest param failed !");
                argv[i] = FEATURE_VALUE_UNDEFINED;
            }
        }
    }
    feature_dup_value(js_ctx, callback);
    feature_value_t ret = feature_call(js_ctx, callback, FEATURE_VALUE_UNDEFINED, fixed_argc + rest_argc, argv);
    if (feature_is_exception(ret)) {
        feature_dump_error(js_ctx);
    }
    feature_free_value(js_ctx, callback);
    feature_free_value(js_ctx, ret);
    return 0;
}

void FeatureInstanceQjs::onDetached()
{
    if (isDetached()) {
        return;
    }

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
#ifdef CONFIG_FEATURE_ENABLE_TRACKER
        auto& feature_tracker = proto->featureTracker();
        feature_tracker.begin("onDetached");
#endif
        proto->description()->native_callbacks->onDetached(js_ctx, this);
#ifdef CONFIG_FEATURE_ENABLE_TRACKER
        feature_tracker.end("onDetached");
#endif
    }

    // release all promises
    releasePromises();

    // release all callback
    clearCallbacks();
    clearAllEvents();
    FeatureInstance::onDetached();
}

void FeatureInstanceQjs::onDumpMemory(FeatureMemoryDump* dump, void* userdata)
{
    FEATURE_LOG_INFO("FeatureInstanceQjs cb: %d", getCallbacks().size());
    dump->count_meta("js_cb", getCallbacks().size(), userdata);
}

}
