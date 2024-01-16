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
#include "feature_ffi_qjs.h"

static inline void free_arg(JSContext* ctx, JSValue& arg)
{
    JS_FreeValue(ctx, arg);
}

static inline JSValue undefined_arg(JSContext* ctx)
{
    return JS_UNDEFINED;
}

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

int PromiseManager::doSettlePromise(bool resolve, FtPromiseId pid, va_list& ap)
{
    // get feature instance
    PromiseData* promise_data = getPromiseData(pid);
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
    return invokeJsCallback(&cb_type, promise_data->resolve_funcs[idx], ap, 1, 0);
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
    if (!data)
        return FEATURE_VALUE_UNDEFINED;

    return data->promise;
}

void PromiseManager::markPromises(feature_runtime_ref rt, feature_mark_func mark_func)
{
    // mark promies
    for (auto& pair : promises_) {
        feature_mark_value(rt, pair.second->promise, mark_func);
        feature_mark_value(rt, pair.second->resolve_funcs[0], mark_func);
        feature_mark_value(rt, pair.second->resolve_funcs[1], mark_func);
    }
}

static bool argToTarget(JSContext* js_ctx, va_list &ap, FeatureType ftype, JSValue& target)
{
    void *param = extractVariadicParam(ap, ftype);
    if (!param) {
        FEATURE_LOG_ERROR("extract callback param failed !");
        return false;
    }
    if (!FeatureFFIQjs::convertValueToGuest(ftype, param, js_ctx, target)) {
        FEATURE_LOG_ERROR("convert callback param failed !");
        free(param);
        return false;
    }
    free(param);
    return true;
}

int PromiseManager::invokeJsCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int fixed_argc, int rest_argc)
{
    if (feature_is_undefined(callback)) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }

    // create argv list and initialize to undefined
    AutoArgs<JSContext*, JSValue> argv(js_ctx_, free_arg, undefined_arg, fixed_argc + rest_argc);
    // convert parameters to feature_value_t
    for (int i = 0; i < fixed_argc; i++) {
        FeatureType ftype = callbackType->parameters[i];
        if (!argToTarget(js_ctx_, ap, ftype, argv[i])) {
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
        if (!FeatureFFIQjs::convertValueToGuest(header->featureType, arg, js_ctx_, argv[i])) {
            FEATURE_LOG_ERROR("convert callback rest param failed !");
            argv[i] = FEATURE_VALUE_UNDEFINED;
        }
    }

    feature_value_t ret = feature_call(js_ctx_, callback, FEATURE_VALUE_UNDEFINED, fixed_argc + rest_argc, argv);
    feature_free_value(js_ctx_, ret);
    return 0;
}

}
