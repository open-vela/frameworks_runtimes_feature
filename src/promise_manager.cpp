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
#include "feature_description.h"
#include "feature_ffi.h"
#include "feature_log.h"
// clang-format off
#include "feature_types.h"
#include "backend/qjs/value_translator_qjs.h"
#include "feature_convertor_templates.h"
// clang-format on

static int invoke_js_Callback(JSContext* ctx, feature_value_t cb, int argc, feature_value_t* argv)
{
    if (feature_is_undefined(cb)) {
        FEATURE_LOG_ERROR("callback is undefined!");
        return -1;
    }

    // create argv list and initialize to undefined
    feature_dup_value(ctx, cb);
    feature_value_t ret = feature_call(ctx, cb, FEATURE_VALUE_UNDEFINED, argc, argv);
    feature_free_value(ctx, cb);
    feature_free_value(ctx, ret);
    return 0;
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

PromiseManager::PromiseData::PromiseData(JSContext* ctx, FeatureType ftype)
    : resolve_type(ftype)
    , promise_type(kPromise)
    , js_ctx(ctx)
{
    promise_info.promise = FEATURE_VALUE_UNDEFINED;
    promise_info.resolve_funcs[0] = FEATURE_VALUE_UNDEFINED;
    promise_info.resolve_funcs[1] = FEATURE_VALUE_UNDEFINED;
}

PromiseManager::PromiseData::PromiseData(JSContext* ctx, FeatureType ftype, feature_value_t success, feature_value_t fail, feature_value_t complete)
    : resolve_type(ftype)
    , promise_type(kCallbacks)
    , js_ctx(ctx)
{
    callbacks.success = success;
    callbacks.fail = fail;
    callbacks.complete = complete;
}

PromiseManager::PromiseData::~PromiseData()
{
    if (promise_type == kPromise) {
        feature_free_value(js_ctx, promise_info.promise);
        feature_free_value(js_ctx, promise_info.resolve_funcs[0]);
        feature_free_value(js_ctx, promise_info.resolve_funcs[1]);
    } else {
        feature_free_value(js_ctx, callbacks.success);
        feature_free_value(js_ctx, callbacks.fail);
        feature_free_value(js_ctx, callbacks.complete);
    }
}

bool PromiseManager::PromiseData::init()
{
    if (promise_type == kPromise) {
        feature_value_t promise = feature_promise_capability(js_ctx, promise_info.resolve_funcs);
        if (feature_is_exception(promise)) {
            feature_free_value(js_ctx, promise_info.resolve_funcs[0]);
            feature_free_value(js_ctx, promise_info.resolve_funcs[1]);
            feature_free_value(js_ctx, promise_info.promise);
            return false;
        }
        promise_info.promise = promise;
    }
    return true;
}

feature_value_t PromiseManager::PromiseData::promise()
{
    if (promise_type == kCallbacks) {
        return FEATURE_VALUE_UNDEFINED;
    }
    return promise_info.promise;
}

int PromiseManager::PromiseData::resolve(va_list& ap)
{
    feature_value_t resolve_func = FEATURE_VALUE_UNDEFINED;
    if (promise_type == kCallbacks) {
        resolve_func = callbacks.success;
    } else {
        resolve_func = promise_info.resolve_funcs[0];
    }
    int ret = -1;
    if (!feature_is_undefined(resolve_func)) {
        JSValue target = JS_UNDEFINED;
        bool is_void = FT_IS_PRIMITIVE(resolve_type) && resolve_type == FT_VOID;
        if (!is_void && !arg_to_target(js_ctx, ap, resolve_type, target)) {
            FEATURE_LOG_ERROR("convert resolve param failed !");
            return ret;
        }
        if (promise_type == kPromise) {
            feature_value_t js_data = feature_object(js_ctx);
            feature_set_object_property(js_ctx, js_data, "data", target);
            feature_value_t argv[] = { js_data };
            ret = invoke_js_Callback(js_ctx, resolve_func, 1, argv);
            feature_free_value(js_ctx, js_data);
            return ret;
        }

        feature_value_t argv[] = { target };
        ret = invoke_js_Callback(js_ctx, resolve_func, 1, argv);
        feature_free_value(js_ctx, target);
    } else {
        FEATURE_LOG_ERROR("resolve func undefined!");
    }

    if (promise_type == kCallbacks) {
        feature_value_t complete_func = callbacks.complete;
        if (feature_is_undefined(complete_func)) {
            FEATURE_LOG_DEBUG("complete func undefined!");
            return ret;
        }
        invoke_js_Callback(js_ctx, complete_func, 0, nullptr);
    }
    return ret;
}

int PromiseManager::PromiseData::reject(int code, const char* msg)
{
    feature_value_t reject_func = FEATURE_VALUE_UNDEFINED;
    if (promise_type == kCallbacks) {
        reject_func = callbacks.fail;
    } else {
        reject_func = promise_info.resolve_funcs[1];
    }

    const char* safe_msg = msg ? msg : "unknown error";
    if (!msg) {
        FEATURE_LOG_WARN("promise reject reason is unknown error !");
    }
    int ret = -1;
    if (!feature_is_undefined(reject_func)) {
        feature_value_t js_code = feature_int(js_ctx, code);
        feature_value_t js_msg = feature_string(js_ctx, safe_msg);
        if (promise_type == kPromise) {
            feature_value_t js_data = feature_object(js_ctx);
            feature_set_object_property(js_ctx, js_data, "code", js_code);
            feature_set_object_property(js_ctx, js_data, "data", js_msg);
            feature_value_t argv[] = { js_data };
            ret = invoke_js_Callback(js_ctx, reject_func, 1, argv);
            feature_free_value(js_ctx, js_data);
            return ret;
        }

        feature_value_t argv[] = { js_msg, js_code };
        ret = invoke_js_Callback(js_ctx, reject_func, 2, argv);
        feature_free_value(js_ctx, js_code);
        feature_free_value(js_ctx, js_msg);
    } else {
        FEATURE_LOG_ERROR("reject func undefined!");
    }

    if (promise_type == kCallbacks) {
        feature_value_t complete_func = callbacks.complete;
        if (feature_is_undefined(complete_func)) {
            FEATURE_LOG_DEBUG("complete func undefined!");
            return ret;
        }
        invoke_js_Callback(js_ctx, complete_func, 0, nullptr);
    }
    return ret;
}

void PromiseManager::PromiseData::mark(feature_runtime_ref rt, feature_mark_func mark_func)
{
    if (promise_type == kPromise) {
        feature_mark_value(rt, promise_info.promise, mark_func);
        feature_mark_value(rt, promise_info.resolve_funcs[0], mark_func);
        feature_mark_value(rt, promise_info.resolve_funcs[1], mark_func);
    } else {
        feature_mark_value(rt, callbacks.success, mark_func);
        feature_mark_value(rt, callbacks.fail, mark_func);
        feature_mark_value(rt, callbacks.complete, mark_func);
    }
}

PromiseManager::PromiseManager(JSContext* js_ctx)
    : js_ctx_(js_ctx)
{
}

PromiseManager::~PromiseManager()
{
    releasePromises();
}

FtPromiseId PromiseManager::addPromise(FeatureType resolve_type)
{
    PromiseData* data = new PromiseData(js_ctx_, resolve_type);
    if (!data->init()) {
        delete data;
        return -1;
    }
    promises_[curr_pid_] = data;
    return curr_pid_++;
}

FtPromiseId PromiseManager::addAsyncCallbacks(FeatureType resolve_type, feature_value_t success, feature_value_t fail, feature_value_t complete)
{
    PromiseData* data = new PromiseData(js_ctx_, resolve_type, success, fail, complete);
    if (!data->init()) {
        delete data;
        return -1;
    }
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
    delete data;
    return true;
}

void PromiseManager::releasePromises()
{
    for (const auto& pair : promises_) {
        FEATURE_LOG_DEBUG("promise: %d freed !", pair.first);
        delete pair.second;
    }
    promises_.clear();
}

int PromiseManager::doResolvePromise(FtPromiseId pid, va_list& ap)
{
    // get feature instance
    PromiseData* data = getPromiseData(pid);
    if (!data) {
        FEATURE_LOG_ERROR("get promise data with handle: %" PRId32 " failed !", pid);
        return -1;
    }
    return data->resolve(ap);
}

int PromiseManager::doRejectPromise(FtPromiseId pid, int code, const char* msg)
{
    // get feature instance
    PromiseData* data = getPromiseData(pid);
    if (!data) {
        FEATURE_LOG_ERROR("get promise data with handle: %" PRId32 " failed !", pid);
        return -1;
    }
    return data->reject(code, msg);
}

int PromiseManager::doGetPromiseType(FtPromiseId pid)
{
    PromiseData* data = getPromiseData(pid);
    if (!data) {
        FEATURE_LOG_ERROR("get promise data with handle: %" PRId32 " failed !", pid);
        return -1;
    }
    return data->getPromiseType();
}

PromiseManager::PromiseData* PromiseManager::getPromiseData(FtPromiseId pid)
{
    if (!promises_.count(pid)) {
        return nullptr;
    }
    return promises_[pid];
}

feature_value_t PromiseManager::getPromise(FtPromiseId pid)
{
    PromiseData* data = getPromiseData(pid);
    if (data) {
        return data->promise();
    }
    return FEATURE_VALUE_UNDEFINED;
}

void PromiseManager::markPromises(feature_runtime_ref rt, feature_mark_func mark_func)
{
    // mark promies
    for (auto& pair : promises_) {
        PromiseData* data = pair.second;
        data->mark(rt, mark_func);
    }
}

}
