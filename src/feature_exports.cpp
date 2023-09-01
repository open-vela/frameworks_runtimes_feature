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
#include "feature_exports.h"
#include "feature_framework.h"
#include "feature_instance.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "feature_ffi.h"

#include <cstdarg>
#include <cstdint>
#include <string.h>

static int featurePromiseSettle(FeatureInstanceHandle handle, bool resolve, FEATURE::FeaturePromiseHandle promiseHandle, va_list& ap)
{
    // get feature instance
    ferry::FeaturePromiseData* promiseData = static_cast<ferry::FeatureInstance*>(handle)->getPromise(promiseHandle);
    if (!promiseData) {
        FEATURE_LOG_ERROR("get promise data with handle: %" PRId32 " failed !", promiseHandle);
        return -1;
    }
    int idx = resolve ? 0 : 1;
    if (feature_is_undefined(promiseData->resolveFuncs[idx])) {
        FEATURE_LOG_ERROR("callback in undefined !");
        return -1;
    }

    ferry::FeatureInstance* instance = static_cast<ferry::FeatureInstance*>(handle);
    FEATURE_CHECK_NE(instance, nullptr);
    FeatureType param_types[2] = { promiseData->resolveTypes[idx], ferry::FT_VOID };
    int ret = instance->invokeFeatureCallback({ .header = { .type = ferry::COMPLEX_PROMISE, .size = 0 }, .parameters = param_types, .return_type = ferry::FT_VOID }, promiseData->resolveFuncs[idx], ap, 1, 0);
    return ret;
}

namespace FEATURE {

void DupFeatureValue(void* ptr)
{
    ferry::FTObjHeader* header = (ferry::FTObjHeader*)((char*)ptr - FT_OBJ_HEADER_SIZE);
    header->ref_count++;
}

void* GetFeatureProtoData(FeatureProtoHandle handle)
{
    ferry::FeaturePrototype* proto = static_cast<ferry::FeaturePrototype*>(handle);
    return proto->native;
}

void SetFeatureProtoData(FeatureProtoHandle handle, void* data)
{
    ferry::FeaturePrototype* proto = static_cast<ferry::FeaturePrototype*>(handle);
    proto->native = data;
}

/**
 * @brief get the native object pointer bind to feature instance
 *
 * @param handle
 * @return void*
 */
void* GetFeatureObjectData(FEATURE::FeatureInstanceHandle handle)
{
    return static_cast<ferry::FeatureInstance*>(handle)->native;
}

void SetFeatureObjectData(FeatureInstanceHandle handle, void* data)
{
    auto instance = static_cast<ferry::FeatureInstance*>(handle);
    instance->native = data;
}

ft_context_ref GetFeatureContext(FeatureInstanceHandle handle)
{
    return static_cast<ferry::FeatureInstance*>(handle)->prototype()->ft_ctx;
}

//int InvokeFeatureCallback(FEATURE::FeatureRuntimeContext ctx, FEATURE::FeatureInstanceHandle handle, void** ret_value, int cid, ...)
int InvokeFeatureCallback(FEATURE::FeatureInstanceHandle handle, int cid, ...)
{
    auto instance = static_cast<ferry::FeatureInstance*>(handle);
    const auto& callback = instance->getCallback(cid);
    if (feature_is_undefined(callback.cb)) {
        FEATURE_LOG_ERROR("callback with cid %d not found !", cid);
        return -1;
    }

    // get callback description.
    bool has_rest_param = false;
    ferry::CallbackType& callbackType = *callback.cb_type;
    int method_param_count = ferry::getParamCount(callbackType.parameters, &has_rest_param);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with InvokeFeatureCallbackCount!");
        return -1;
    }

    va_list ap;
    va_start(ap, cid);
    // int ret = invokeFeatureCallback(js_ctx, instance, callbackType, pair.first, ap, method_param_count, 0, ret_value);
    int ret = instance->invokeFeatureCallback(callbackType, callback.cb, ap, method_param_count, 0);
    va_end(ap);
    return ret;
}

// int InvokeFeatureCallbackCount(FEATURE::FeatureRuntimeContext ctx, FEATURE::FeatureInstanceHandle handle, void** ret_value, FeatureCallbackId cid, int count, ...)
int InvokeFeatureCallbackCount(FEATURE::FeatureInstanceHandle handle, FeatureCallbackId cid, int count, ...)
{
    auto instance = static_cast<ferry::FeatureInstance*>(handle);
    const auto& callback = instance->getCallback(cid);
    if (feature_is_undefined(callback.cb)) {
        FEATURE_LOG_ERROR("callback with cid %d not found !", cid);
        return -1;
    }

    // get callback description.
    bool has_rest_param = false;
    ferry::CallbackType* callbackType = callback.cb_type;
    int method_param_count = ferry::getParamCount(callbackType->parameters, &has_rest_param);
    FEATURE_CHECK_EQ(has_rest_param, true);
    FEATURE_CHECK_GE(count, method_param_count);

    va_list ap;
    va_start(ap, count);
    // int ret = invokeFeatureCallback(js_ctx, instance, *callbackType, pair.first, ap, method_param_count, count - method_param_count, ret_value);
    int ret = instance->invokeFeatureCallback(*callbackType, callback.cb, ap, method_param_count, count - method_param_count);
    va_end(ap);
    return ret;
}

bool RemoveCallback(FeatureInstanceHandle handle, FeatureCallbackId id)
{
    auto instance = static_cast<ferry::FeatureInstance*>(handle);
    return instance->removeCallback(id);
}

int FeaturePromiseResolve(FeatureInstanceHandle handle, FEATURE::FeaturePromiseHandle promiseHandle, ...)
{
    va_list ap;
    va_start(ap, promiseHandle);
    int ret = featurePromiseSettle(handle, true, promiseHandle, ap);
    va_end(ap);
    // remove
    ferry::FeatureInstance* instance = static_cast<ferry::FeatureInstance*>(handle);
    if (!instance->removePromise(promiseHandle)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", promiseHandle);
        ret = -2;
    }
    return ret;
}

int FeaturePromiseReject(FeatureInstanceHandle handle, FeaturePromiseHandle promiseHandle, ...)
{
    va_list ap;
    va_start(ap, promiseHandle);
    int ret = featurePromiseSettle(handle, false, promiseHandle, ap);
    ferry::FeatureInstance* instance = static_cast<ferry::FeatureInstance*>(handle);
    if (!instance->removePromise(promiseHandle)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", promiseHandle);
        ret = -2;
    }
    va_end(ap);
    return ret;
}

}
