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
#include "feature_framework.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "feature.h"
#include "feature_ffi.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>
#include <strings.h>
#include <tuple>

using namespace FEATURE;

#define CFUNCDATA_FN(f) ((feature_value_t(*)(feature_context_ref ctx, feature_value_t, int, feature_value_t*, int, feature_value_t*))f)

inline int get_param_count(const FeatureType* param, bool* hasRest = nullptr, int* optional_size = nullptr)
{
    int count = 0;
    if (optional_size) {
        *optional_size = 0;
    }
    while (param && FT_GET_VALUE(*param)) {
        count++;
        if (optional_size && FT_IS_COMPLEX(*param)) {
            ferry::ComplexTypeHeader* complexHeader = (ferry::ComplexTypeHeader*)FT_GET_COMPLEX(*param);
            if (complexHeader->type == ferry::COMPLEX_OPTIONAL) {
                *optional_size = *optional_size + 1;
            }
        }
        param++;
    }
    if (hasRest) {
        *hasRest = param ? FT_IS_REST(*param) : false;
    }
    return count;
}

// int invokeFeatureCallback(context_ref ctx, ferry::FeatureInstance* instance, ferry::CallbackType& callbackType, feature_value_t callback, va_list& ap, int method_param_count, int rest_param_count, void** ret_value)
int invokeFeatureCallback(context_ref ctx, ferry::FeatureInstance* instance, const ferry::CallbackType& callbackType, feature_value_t callback, va_list& ap, int method_param_count, int rest_param_count)
{
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
            FeatureType featureType = callbackType.parameters[i];
            void* ptr = ferry::FeatureFFI::exactVariadicParameter(ap, featureType);
            if (!ptr) {
                got_error = true;
                break;
            }
            if (!ferry::FeatureFFI::convertValueToGuest(instance, featureType, ptr, ctx, argv[i])) {
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
            ferry::FTObjHeader* header = (ferry::FTObjHeader*)header_ptr;
            if (!ferry::FeatureFFI::convertValueToGuest(instance, header->featureType, arg, ctx, argv[i])) {
                FEATURE_LOG_ERROR("convert callback rest param failed !");
                argv[i] = FEATURE_VALUE_UNDEFINED;
            }
        }

        ret = feature_call(ctx, callback, FEATURE_VALUE_UNDEFINED, method_param_count + rest_param_count, argv);
    } while (0);
    for (int i = 0; i < method_param_count + rest_param_count; i++) {
        feature_free_value(ctx, argv[i]);
    }
    delete[] argv;
    /*
    if (callbackType.return_type != ferry::FT_VOID && ret_value && !jse_is_undefined(ret)) {
        // allocate ret_value first
        ffi_type* ret_type = nullptr;
        if (!ferry::FeatureFFI::createTypeDeclaration(callbackType.return_type, ret_type)) {
            ferry::FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(*ret_value);
            *ret_value = nullptr;
            return -1;
        }
        if (!ferry::FeatureFFI::convertValueToHost(instance, callbackType.return_type, *ret_value, ctx, ret)) {
            ferry::FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(*ret_value);
            *ret_value = nullptr;
            return -1;
        }
        ferry::FeatureFFI::freeTypeDeclaration(ret_type);
        // for reference type, remove the pointer's pointer.
        if (FT_IS_REFERENCE(callbackType.return_type)) {
            auto result = **(void***)ret_value;
            free(*ret_value);
            *ret_value = result;
        }
    }
*/
    feature_free_value(ctx, ret);

    return 0;
}

ferry::FeaturePromiseData* FeatureCreatePromise(FeatureInstanceHandle handle, FeatureType resolve_type, FeatureType reject_type)
{
    ferry::FeaturePromiseData* data = (ferry::FeaturePromiseData*)malloc(sizeof(ferry::FeaturePromiseData));

    data->promise = FEATURE_VALUE_UNDEFINED;
    data->resolveFuncs[0] = FEATURE_VALUE_UNDEFINED;
    data->resolveFuncs[1] = FEATURE_VALUE_UNDEFINED;
    data->resolveTypes[0] = resolve_type;
    data->resolveTypes[1] = reject_type;

    auto ctx = GetFeatureContext(handle);
    feature_value_t promise = feature_promise_capability(ctx, data->resolveFuncs);
    if (feature_is_exception(promise)) {
        feature_free_value(ctx, data->resolveFuncs[0]);
        feature_free_value(ctx, data->resolveFuncs[1]);
        feature_free_value(ctx, promise);
        free(data);
        return nullptr;
    }
    data->promise = promise;
    return data;
}

static int featurePromiseSettle(FeatureInstanceHandle handle, bool resolve, FEATURE::FeaturePromiseHandle promiseHandle, va_list& ap)
{
    auto ctx = GetFeatureContext(handle);
    FEATURE_CHECK_NE(ctx, nullptr);

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
    int ret = invokeFeatureCallback(ctx, instance, { .header = { .type = ferry::COMPLEX_PROMISE, .size = 0 }, .parameters = param_types, .return_type = ferry::FT_VOID }, promiseData->resolveFuncs[idx], ap, 1, 0);
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

context_ref GetFeatureContext(FeatureInstanceHandle handle)
{
    return static_cast<ferry::FeatureInstance*>(handle)->proto->ctx;
}

//int InvokeFeatureCallback(FEATURE::FeatureRuntimeContext ctx, FEATURE::FeatureInstanceHandle handle, void** ret_value, int cid, ...)
int InvokeFeatureCallback(FEATURE::FeatureInstanceHandle handle, int cid, ...)
{
    auto ctx = GetFeatureContext(handle);
    auto instance = static_cast<ferry::FeatureInstance*>(handle);
    const auto& pair = instance->getCallback(cid);
    if (feature_is_undefined(pair.first)) {
        FEATURE_LOG_ERROR("callback with cid %d not found !", cid);
        return -1;
    }

    context_ref js_ctx = static_cast<context_ref>(ctx);
    // get callback description.
    bool has_rest_param = false;
    ferry::CallbackType& callbackType = *pair.second;
    int method_param_count = get_param_count(callbackType.parameters, &has_rest_param);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with InvokeFeatureCallbackCount!");
        return -1;
    }

    va_list ap;
    va_start(ap, cid);
    // int ret = invokeFeatureCallback(js_ctx, instance, callbackType, pair.first, ap, method_param_count, 0, ret_value);
    int ret = invokeFeatureCallback(js_ctx, instance, callbackType, pair.first, ap, method_param_count, 0);
    va_end(ap);
    return ret;
}

// int InvokeFeatureCallbackCount(FEATURE::FeatureRuntimeContext ctx, FEATURE::FeatureInstanceHandle handle, void** ret_value, FeatureCallbackId cid, int count, ...)
int InvokeFeatureCallbackCount(FEATURE::FeatureInstanceHandle handle, FeatureCallbackId cid, int count, ...)
{
    auto ctx = GetFeatureContext(handle);
    auto instance = static_cast<ferry::FeatureInstance*>(handle);
    const auto& pair = instance->getCallback(cid);
    if (feature_is_undefined(pair.first)) {
        FEATURE_LOG_ERROR("callback with cid %d not found !", cid);
        return -1;
    }

    context_ref js_ctx = static_cast<context_ref>(ctx);
    // get callback description.
    bool has_rest_param = false;
    ferry::CallbackType* callbackType = pair.second;
    int method_param_count = get_param_count(callbackType->parameters, &has_rest_param);
    FEATURE_CHECK_EQ(has_rest_param, true);
    FEATURE_CHECK_GE(count, method_param_count);

    va_list ap;
    va_start(ap, count);
    // int ret = invokeFeatureCallback(js_ctx, instance, *callbackType, pair.first, ap, method_param_count, count - method_param_count, ret_value);
    int ret = invokeFeatureCallback(js_ctx, instance, *callbackType, pair.first, ap, method_param_count, count - method_param_count);
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

namespace ferry {
extern struct FeatureInstance* getInstance(feature_value_t val);

/////////////////////////////////////////////////
FeatureInstance::FeatureInstance(FeaturePrototype* featurePrototype)
    : native(nullptr)
    , proto(featurePrototype)
    , iid(-1)
{
}

FeatureInstance::~FeatureInstance()
{
    // remove opaque binding
    feature_set_opaque(this->js_self.js_value, nullptr);
    // release all callbacks
    for (const auto& pair : callbacks) {
        feature_free_value(proto->ctx, pair.second.first);
    }
    callbacks.clear();

    // release all promises
    for (const auto& pair : promises) {
        FEATURE_LOG_DEBUG("promise: %" PRId32 " freed !", pair.first);
        feature_free_value(proto->ctx, pair.second->promise);
        feature_free_value(proto->ctx, pair.second->resolveFuncs[0]);
        feature_free_value(proto->ctx, pair.second->resolveFuncs[1]);
        free(pair.second);
    }
    promises.clear();

    // check if all instances deleted, then clear proto object
    if (!proto->hasInstanceAlive()) {
        FEATURE_LOG_INFO("all instance freed, free proto object...");
        proto->js_proto = FEATURE_VALUE_UNDEFINED;
    }
}

std::pair<feature_value_t, CallbackType*> FeatureInstance::getCallback(FEATURE::FeatureCallbackId id)
{
    if (!callbacks.count(id)) {
        return std::make_pair(FEATURE_VALUE_UNDEFINED, nullptr);
    }
    return callbacks[id];
}

FEATURE::FeatureCallbackId FeatureInstance::addCallback(feature_value_t value, CallbackType* callbackType)
{
    auto ctx = proto->ctx;
    callbacks[curr_cid] = std::make_pair(feature_dup_value(ctx, value), callbackType);
    return curr_cid++;
}

bool FeatureInstance::removeCallback(FEATURE::FeatureCallbackId id)
{
    auto ctx = proto->ctx;
    if (!callbacks.count(id)) {
        FEATURE_LOG_ERROR("callback id %d in instance: %p not exist !", id, this);
        return false;
    }
    feature_free_value(ctx, callbacks[id].first);
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
    // auto ctx = proto->ctx;
    FEATURE_CHECK_NE(data, nullptr);
    FEATURE_CHECK_NE(feature_is_undefined(data->promise), true);
    promises[curr_cid] = data;
    return curr_cid++;
}

bool FeatureInstance::removePromise(FEATURE::FeaturePromiseHandle promiseHandle)
{
    auto ctx = proto->ctx;
    if (!promises.count(promiseHandle)) {
        FEATURE_LOG_ERROR("promiseHandle %d in instance: %p not exist !", promiseHandle, this);
        return false;
    }
    FeaturePromiseData* data = promises[promiseHandle];
    FEATURE_CHECK_NE(data, nullptr);
    promises.erase(promiseHandle);
    // free js values
    feature_free_value(ctx, data->promise);
    feature_free_value(ctx, data->resolveFuncs[0]);
    feature_free_value(ctx, data->resolveFuncs[1]);
    free(data);
    return true;
}

/**
 * @brief FeaturePrototype constructor
 *
 * @param description
 */
FeaturePrototype::FeaturePrototype(context_ref js_ctx, FeatureDescription* feature_desc)
    : ctx(js_ctx)
    , native(nullptr)
    , js_proto(FEATURE_VALUE_UNDEFINED)
    , description(feature_desc)
{
    // default capacity as 10 element
    instances.reserve(10);
}

FeaturePrototype::~FeaturePrototype()
{
    instances.clear();
    // call onDestroy
    if (description->native_callbacks->onDestroy) {
        FEATURE_LOG_DEBUG("invoke onDestroy callback...");
        description->native_callbacks->onDestroy(ctx, this);
    }
    if (!feature_is_undefined(js_proto)) {
        feature_free_value(ctx, js_proto);
        js_proto = FEATURE_VALUE_UNDEFINED;
    }
}

/**
 * @brief add FeatureInstance
 *
 * @param inst
 * @return int
 */
int FeaturePrototype::addInstance(std::unique_ptr<FeatureInstance>&& inst)
{
    auto pos = std::find_if(instances.begin(), instances.end(), [](const std::unique_ptr<FeatureInstance>& target) {
        return target == nullptr;
    });
    // it's full, append at end
    if (pos == instances.end()) {
        instances.emplace_back(std::move(inst));
        return instances.size() - 1;
    }
    // insert into pos
    *pos = std::move(inst);
    return std::distance(instances.begin(), pos);
}

/**
 * @brief Remove FeatureInstance by index
 *
 * @param pos
 * @return true
 * @return false
 */
bool FeaturePrototype::removeInstance(size_t pos)
{
    if (pos >= instances.size())
        return false;
    instances[pos] = nullptr;
    return true;
}

bool FeaturePrototype::hasInstanceAlive()
{
    for (auto& inst : instances) {
        if (inst) {
            return true;
        }
    }
    return false;
}

FeatureUnit::FeatureUnit(const FeatureDescription* desc)
    : description(const_cast<FeatureDescription*>(desc))
    , proto(nullptr)
{
}

/**
 * @brief we need the default constructor to support put into containers
 *
 */
FeatureUnit::FeatureUnit()
    : description(nullptr)
    , proto(nullptr)
{
}

FeatureUnit::~FeatureUnit()
{
    if (proto) {
        delete proto;
    }
}

/**
 * @brief invoke method, support：
 * 1. parameter and return value auto convert
 * 2. support optional parameter/default value
 * 3. support variadic parameters which passed as feature_value_t to native(consider better solution, need runtime type reflection mechanics)
 * 4. support complex class types, complex type only support reference, support return by value.
 *
 * @param ctx
 * @param this_val
 * @param argc
 * @param argv
 * @param magic
 * @return feature_value_t
 */
static feature_value_t method_call(feature_context_ref ctx, feature_value_t this_val,
    int argc, feature_value_t* argv, int magic)
{
    bool got_error = false;
    feature_value_t method_ret_value = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = ferry::getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->proto->description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    const auto& method = member->method;
    auto currParam = method.parameters;
    FEATURE::FeaturePromiseHandle promiseHandle = -1;
    //feature_value_t promise_obj = FEATURE_VALUE_UNDEFINED;
    // count size
    bool has_rest_param = false;
    int optional_count = 0;
    int method_param_count = get_param_count(currParam, &has_rest_param, &optional_count);
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_param && optional_count, true);
    // variadic parameters type
    ffi_type variadicParameters_type;
    ffi_type* variadicParameters_type_element[3];
    // variadic parameter param
    FtVariadicParameters variadicParameters;
    memset(&variadicParameters, 0, sizeof(variadicParameters));
    // check argument count match.
    // FEATURE_LOG_DEBUG("required param count: %d, received param count: %d", method_param_count, argc);
    // beacuse we support rest parameters, so argc is greater or equal to method_param_count.
    if (has_rest_param) {
        FEATURE_CHECK_GE(argc, method_param_count);
        variadicParameters.variadic_count = argc - method_param_count;
    } else if (optional_count) {
        // for optional parameters, argc + optional must grater or equal to method_param_count
        FEATURE_CHECK_GE(argc + optional_count, method_param_count);
    } else {
        // for method which do not have rest or optional parameters, argc equals to method_param_count.
        FEATURE_CHECK_EQ(argc, method_param_count);
    }
    // if has rest parameter, we will pack all variadic parameters together as a param pack
    // use packed_argc instead of argc for ffi call.
    int32_t packed_argc = has_rest_param ? argc - variadicParameters.variadic_count + 1 : argc;
    // if return value is a promise
    bool isPromise = FT_IS_PROMISE(method.return_type);
    int external_count = isPromise ? 3 : 2;
    ffi_type** ffi_params = new ffi_type*[packed_argc + optional_count + external_count + 1]; // FeaturInstance, data, maybe return promise, empty placeholder
    ffi_type* ffi_ret = nullptr;
    memset(ffi_params, 0, sizeof(ffi_type*) * (packed_argc + optional_count + external_count + 1));

    void** ffi_arg_values = new void*[packed_argc + optional_count + external_count]; // FeaturInstance, data, maybe return promise
    memset(ffi_arg_values, 0, sizeof(void*) * (packed_argc + optional_count + external_count));
    void* ffi_ret_value = nullptr;

    // prepare first two param
    ffi_params[0] = &ffi_type_pointer; // FeatureContext
    ffi_arg_values[0] = &instance;
    ffi_params[1] = &ffi_type_sint64; // data
    ffi_arg_values[1] = (void*)&method.data;
    if (isPromise) {
        ffi_params[2] = &ffi_type_sint32;
    }

    do {
        for (int i = 0; i < method_param_count && i < argc; i++) {
            feature_value_t currArg = argv[i];
            auto param = currParam[i];
            if (FT_IS_PROMISE(param)) {
                FEATURE_LOG_ERROR("do not support promise as input param !");
                got_error = true;
                break;
            }
            if (!ferry::FeatureFFI::createTypeDeclaration(param, ffi_params[external_count + i])) {
                FEATURE_LOG_ERROR("prepareType for type failed !");
                got_error = true;
                break;
            }
            if (!ferry::FeatureFFI::convertValueToHost(instance, param, ffi_arg_values[external_count + i], ctx, currArg)) {
                FEATURE_LOG_ERROR("convert argument %d failed !", i);
                got_error = true;
                break;
            }
        }
        if (got_error)
            break;

        // process rest parameters
        if (has_rest_param) {
            // prepare variadicParameters type
            variadicParameters_type.size = 0;
            variadicParameters_type.type = FFI_TYPE_STRUCT;
            variadicParameters_type.elements = variadicParameters_type_element;
            variadicParameters_type_element[0] = &ffi_type_sint32;
            variadicParameters_type_element[1] = &ffi_type_pointer;
            variadicParameters_type_element[2] = nullptr;
            // prepare variadicParameters struct
            variadicParameters.variadic_args = new feature_value_t*[variadicParameters.variadic_count];
            // pass param
            ffi_params[method_param_count + external_count] = &variadicParameters_type;
            ffi_arg_values[method_param_count + external_count] = &variadicParameters;
            for (int i = 0; i + method_param_count < argc; i++) {
                // just passthrough guest param pointers
                variadicParameters.variadic_args[i] = &argv[i + method_param_count];
            }
        } else if (optional_count) {
            for (int i = argc; i < method_param_count; i++) {
                auto param = currParam[i];
                FEATURE_CHECK_EQ(FT_IS_COMPLEX(param), true);
                OptionalType* optionalType = (OptionalType*)FT_GET_COMPLEX(param);
                FEATURE_CHECK_EQ(optionalType->header.type, COMPLEX_OPTIONAL);
                if (!ferry::FeatureFFI::createTypeDeclaration(param, ffi_params[external_count + i])) {
                    FEATURE_LOG_ERROR("prepareType for type failed !");
                    got_error = true;
                    break;
                }
                ffi_arg_values[external_count + i] = &optionalType->fval;
            }
        }

        if (got_error)
            break;

        // prepeare return type
        if (!ferry::FeatureFFI::createTypeDeclaration(method.return_type, ffi_ret)) {
            FEATURE_LOG_ERROR("prepareType for complex type failed !");
            got_error = true;
            break;
        }
        // create return value pointer inneed.
        if (!isPromise && method.return_type != FT_VOID) {
            if (!ferry::FeatureFFI::createHostValue(method.return_type, ffi_ret_value, true)) {
                FEATURE_LOG_ERROR("create return value failed !");
                got_error = true;
                break;
            }
        }
        // prepare ffi call
        ffi_cif cif;
        ffi_status ret = FFI_OK;
        if (has_rest_param) {
            // FEATURE_LOG_DEBUG("prepare for variadic parameter function...");
            ret = ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, method_param_count + external_count, packed_argc + external_count, ffi_ret, ffi_params);
        } else {
            // FEATURE_LOG_DEBUG("prepare for function...");
            ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, method_param_count + external_count, ffi_ret, ffi_params);
        }
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            got_error = true;
            break;
        }
        // special handle for promise
        if (isPromise) {
            ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(method.return_type);
            // create promise
            PromiseType* promiseType = (PromiseType*)complexType;
            // create promise and add to instance
            auto promiseData = FeatureCreatePromise(instance, promiseType->resolveTypes[0], promiseType->resolveTypes[1]);
            FEATURE_CHECK_NE(promiseData, nullptr);
            promiseHandle = instance->addPromise(promiseData);
            // pass promiseHandle to native function
            ffi_arg_values[2] = &promiseHandle;
            // dup and return promise object.
            method_ret_value = feature_dup_value(ctx, promiseData->promise);
        }
        // invoke method
        ffi_call(&cif, method.callback, ffi_ret_value, ffi_arg_values);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!isPromise && method.return_type != FT_VOID) {
            // process return value
            if (!ferry::FeatureFFI::convertValueToGuest(instance, method.return_type, ffi_ret_value, ctx, method_ret_value)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                feature_free_value(ctx, method_ret_value);
                method_ret_value = FEATURE_EXCEPTION;
                got_error = true;
            }
        }
    } while (0);

    // free ffi call resources
    for (int i = 0; i < method_param_count; i++) {
        // free type
        if (ffi_params[i + external_count]) {
            ferry::FeatureFFI::freeTypeDeclaration(ffi_params[i + external_count]);
        }
        // free value
        if (ffi_arg_values[i + external_count]) {
            FreeFeatureValue(ffi_arg_values[i + external_count]);
        }
    }
    ferry::FeatureFFI::freeTypeDeclaration(ffi_ret);
    if (ffi_ret_value) {
        FreeFeatureValue(ffi_ret_value);
    }
    delete[] ffi_arg_values;
    delete[] ffi_params;
    if (variadicParameters.variadic_args) {
        delete[] variadicParameters.variadic_args;
    }

    // if error occurred, throw internal error
    if (got_error) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native method failed !");
    }

    return method_ret_value;
}

static feature_value_t accessor_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    // get info from this_val
    feature_value_t method_ret_value = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = ferry::getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->proto->description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = &member->accessor;
    FEATURE_CHECK_NE(accessor->type, FT_VOID);
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type* ffi_params[2] = { &ffi_type_pointer, &ffi_type_sint64 };
    ffi_type* ffi_ret = nullptr;
    void* arg_values[2] = { &instance, &accessor->data };
    void* ret_value = nullptr;
    do {
        if (!ferry::FeatureFFI::createTypeDeclaration(accessor->type, ffi_ret)) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        if (!ferry::FeatureFFI::createHostValue(accessor->type, ret_value, true)) {
            FEATURE_LOG_ERROR("create return value failed !");
            break;
        }

        // prepare and call method
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, ffi_ret, ffi_params);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            break;
        }
        // invoke
        ffi_call(&cif, accessor->getter, ret_value, arg_values);
        // process return value
        if (!ferry::FeatureFFI::convertValueToGuest(instance, accessor->type, ret_value, ctx, method_ret_value)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            feature_free_value(ctx, method_ret_value);
            method_ret_value = FEATURE_EXCEPTION;
        }
    } while (0);
    // free resources
    ferry::FeatureFFI::freeTypeDeclaration(ffi_ret);
    FreeFeatureValue(ret_value);

    return method_ret_value;
}

static feature_value_t accessor_set(feature_context_ref ctx, feature_value_t this_val, feature_value_t val, int magic)
{
    // get info from this_val
    int index = magic;
    FeatureInstance* instance = ferry::getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->proto->description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = &member->accessor;
    FEATURE_CHECK_NE(accessor->type, FT_VOID);
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type* ffi_params[3] = { &ffi_type_pointer, &ffi_type_sint64, nullptr };
    void* arg_value_input = nullptr;
    void* arg_values[3] = { &instance, &accessor->data, nullptr };
    do {
        // prepare third param type declaration, create by accessor type
        if (!ferry::FeatureFFI::createTypeDeclaration(accessor->type, ffi_params[2])) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        // fill third param using guest value and accesor type
        if (!ferry::FeatureFFI::convertValueToHost(instance, accessor->type, arg_value_input, ctx, val)) {
            FEATURE_LOG_ERROR("convert to host value failed !");
            break;
        }
        arg_values[2] = arg_value_input;

        // prepare and call method
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 3, &ffi_type_void, ffi_params);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            break;
        }
        // invoke
        ffi_call(&cif, accessor->setter, arg_values[2], arg_values);
    } while (0);
    // free resources
    ferry::FeatureFFI::freeTypeDeclaration(ffi_params[2]);
    FreeFeatureValue(arg_value_input);
    return FEATURE_VALUE_UNDEFINED;
}

static feature_value_t const_variable_initialize(context_ref ctx, FeaturePrototype* prototype, const MemberConst& memberConst)
{
    feature_value_t val = FEATURE_VALUE_UNDEFINED;
    FEATURE_CHECK_NE(memberConst.type, FT_VOID);
    // invoke callback to get constant value
    if (memberConst.callback) {
        // create type using featureType description
        ffi_type* ret_type = nullptr;
        void* ret_value = nullptr;
        ffi_type* param_types[2] = { &ffi_type_pointer, &ffi_type_sint64 };
        void* arg_values[2] = { &prototype, (void*)&memberConst.data };
        if (!ferry::FeatureFFI::createTypeDeclaration(memberConst.type, ret_type)) {
            FEATURE_LOG_ERROR("create type failed !");
            FeatureFFI::freeTypeDeclaration(ret_type);
            return val;
        }
        if (!ferry::FeatureFFI::createHostValue(memberConst.type, ret_value, true)) {
            FEATURE_LOG_ERROR("create return value failed !");
            FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(ret_value);
            return val;
        }
        // prepare and call
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, ret_type, param_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(ret_value);
            return val;
        }
        // invoke
        ffi_call(&cif, memberConst.callback, ret_value, arg_values);
        // process return value
        if (!ferry::FeatureFFI::convertValueToGuest(nullptr, memberConst.type, ret_value, ctx, val)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            feature_free_value(ctx, val);
            val = FEATURE_VALUE_UNDEFINED;
        }
        FeatureFFI::freeTypeDeclaration(ret_type);
        FreeFeatureValue(ret_value);
        if (FT_IS_REFERENCE(ret_value)) {
            free(ret_value);
        }
    } else {
        // check type
        if (!ferry::FeatureFFI::convertValueToGuest(nullptr, memberConst.type, (void*)&memberConst.data, ctx, val)) {
            FEATURE_LOG_ERROR("can not convert const value to guest!");
            feature_free_value(ctx, val);
            val = FEATURE_VALUE_UNDEFINED;
        }
    }
    return val;
}

int initialize_prototype(context_ref ctx, FeatureUnit* unit, feature_value_t proto)
{
    assert(unit != nullptr);
    for (int i = 0; i < unit->description->member_count; i++) {
        const Member& member = unit->description->members[i];
        switch (member.type) {
        case MEMBER_NULL: {
            // not allowed
            FEATURE_CHECK(false && "invalid member type!");
        } break;
        case MEMBER_METHOD: {
            // register different type
            const MemberMethod& method = member.method;
            feature_value_t methodCallObj = JS_NewCFunctionMagic(static_cast<feature_context_ref>(ctx), method_call, member.name, get_param_count(method.parameters), JS_CFUNC_generic_magic, i);
            feature_define_object_property(ctx, proto, member.name, methodCallObj, FEATURE_PROP_ENUMERABLE);
        } break;
        case MEMBER_ACCESSOR: {
            // create getter and setter
            const MemberAccessor& accessor = member.accessor;
            feature_atom_t prop_name = feature_atom(static_cast<feature_context_ref>(ctx), member.name);
            feature_value_t funcs[2] = { FEATURE_VALUE_UNDEFINED, FEATURE_VALUE_UNDEFINED };

            char buf[128];
            JSCFunctionType type;
            if (accessor.getter) {
                type.getter_magic = accessor_get;
                sprintf(buf, "get %s", member.name);
                funcs[0] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 0, JS_CFUNC_getter_magic, i);
            }
            if (accessor.setter) {
                type.setter_magic = accessor_set;
                sprintf(buf, "set %s", member.name);
                funcs[1] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 1, JS_CFUNC_setter_magic, i);
            }
            JS_DefinePropertyGetSet(static_cast<feature_context_ref>(ctx), proto, prop_name, funcs[0], funcs[1], FEATURE_PROP_CONFIGURABLE);
            feature_free_atom(static_cast<feature_context_ref>(ctx), prop_name);
        } break;
        case MEMBER_CONST: {
            // handle member const, maybe initialized or directly constantant
            const MemberConst& constMember = member.value;
            feature_value_t constantVal = const_variable_initialize(ctx, unit->proto, constMember);
            feature_define_object_property(ctx, proto, member.name, constantVal, FEATURE_PROP_ENUMERABLE);

        } break;
        }
    }
    return 0;
}

bool WeakRefInit(context_ref js_ctx, feature_value_t feature_object)
{
    // 根据cid获取FeaturePrototype
    FeatureInstance* instance = getInstance(feature_object);
    if (instance == nullptr) {
        FEATURE_LOG_ERROR("WeakRefInit() get FeatureInstance failed");
        return false;
    }
    auto proto = instance->proto;

    // 创建WeakRef节点添加到proto->weak_ref_list链表中
    WeakRef* node = &instance->js_self;
    weakref_list_initialize(&node->link);
    weakref_list_add_tail(&node->link, &proto->weak_ref_list);

    node->js_value = feature_object;
    proto->weak_ref_count++;

    return true;
}

bool WeakRefFree(context_ref js_ctx, feature_value_t feature_object)
{
    // 获取feature_object的cid
    int ret = -1;
    FeatureInstance* instance = getInstance(feature_object);
    if (instance == nullptr) {
        FEATURE_LOG_ERROR("WeakRefFree() get FeatureInstance failed");
        return false;
    }
    auto proto = instance->proto;

    // 遍历proto->weak_ref_list链表，将其中所有js_value为feature_object的节点删除
    WeakRef* node;
    WeakRef* node_temp;
    weakref_list_for_every_entry_safe(&proto->weak_ref_list, node, node_temp, WeakRef, link)
    {
        ret = feature_is_same_value(static_cast<feature_context_ref>(js_ctx), node->js_value, feature_object);
        if (ret == 1) {
            weakref_list_delete(&node->link);
            proto->weak_ref_count--;
        }
    }

    return true;
}

}
