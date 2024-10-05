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

#include "feature_manager_qjs.h"
#include "feature.h"
#include "feature_context.h"
#include "feature_context_qjs.h"
// clang-format off
#include "value_translator_qjs.h"
#include "feature_ffi_templates.h"
// clang-format on
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_prototype_qjs.h"
#include "feature_registry.h"
#include "feature_utils.h"

#include <alloca.h>
#include <assert.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string.h>
#include <string>
#include <vector>

#define FEATURE_ENV_NAME "quickjs"

namespace feature_framework {

#ifdef CONFIG_FEATURE_USE_JS_FUNCTION_BINDING
static inline int get_ref_count(feature_value_t val)
{
    if (JS_VALUE_HAS_REF_COUNT(val)) {
        JSRefCountHeader* p = (JSRefCountHeader*)JS_VALUE_GET_PTR(val);
        return p->ref_count;
    }
    return -1;
}

static feature_value_t bind_js_function(feature_context_ref ctx, const feature_value_t func, const feature_value_t this_obj)
{
    if (!JS_IsFunction(ctx, func)) {
        FEATURE_LOG_ERROR("error: func must be a function!");
        return FEATURE_VALUE_UNDEFINED;
    }

    feature_value_t res = FEATURE_VALUE_UNDEFINED;
    feature_value_t bind_func = JS_GetPropertyStr(ctx, func, "bind");
    if (JS_IsException(bind_func) || !JS_IsFunction(ctx, bind_func)) {
        FEATURE_LOG_ERROR("error: bind function is missing");
        feature_free_value(ctx, bind_func);
        return res;
    }

    feature_value_t args[] = { this_obj };
    res = feature_call(ctx, bind_func, func, 1, args);
    if (!JS_IsFunction(ctx, res)) {
        FEATURE_LOG_ERROR("error: result is not a function");
    }
    feature_free_value(ctx, bind_func);
    return res;
}

void free_js_prop_enums(feature_context_ref ctx, JSPropertyEnum* ptab, uint32_t len)
{
    if (!ptab)
        return;

    for (uint32_t i = 0; i < len; i++) {
        JS_FreeAtom(ctx, ptab[i].atom);
    }
    js_free(ctx, ptab);
}

static bool bind_js_functions_to_instance(feature_context_ref ctx, feature_value_t js_proto, feature_value_t js_instance)
{
    JSPropertyEnum* ptab = nullptr;
    uint32_t len = 0;
    if (!JS_GetOwnPropertyNames(ctx, &ptab, &len, js_proto, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK | JS_GPN_ENUM_ONLY)) {
        for (uint32_t i = 0; i < len; i++) {
            const char* prop_name = JS_AtomToCString(ctx, ptab[i].atom);
            if (!prop_name) {
                continue;
            }
            feature_value_t prop_val = JS_GetPropertyStr(ctx, js_proto, prop_name);
            if (JS_IsException(prop_val)) {
                feature_free_cstring(ctx, prop_name);
                feature_free_value(ctx, prop_val);
                continue;
            }
            if (JS_IsFunction(ctx, prop_val)) {
                feature_value_t bound_func = bind_js_function(ctx, prop_val, js_instance);
                FEATURE_LOG_DEBUG("bind function: %s, js instance refcount: %d.", prop_name, get_ref_count(js_instance));
                if (feature_is_undefined(bound_func)) {
                    FEATURE_LOG_ERROR("bind function %s error!", prop_name);
                    feature_free_cstring(ctx, prop_name);
                    feature_free_value(ctx, prop_val);
                    free_js_prop_enums(ctx, ptab, len);
                    return false;
                }
                JS_DefinePropertyValueStr(ctx, js_instance, prop_name, bound_func, FEATURE_PROP_CONFIGURABLE);
            }
            feature_free_cstring(ctx, prop_name);
            feature_free_value(ctx, prop_val);
        }
    }
    free_js_prop_enums(ctx, ptab, len);
    return true;
}
#endif

static inline FeatureInstance* getInstance(feature_value_t val)
{
    auto class_id = FeatureManagerQjs::jsClassId();
    if (class_id == 0)
        return NULL;

    void* ptr = feature_get_opaque(val, class_id);
    return static_cast<FeatureInstance*>(ptr);
}

static feature_value_t reportArgsError(FeatureInstance* instance,
    feature_context_ref ctx, RetCode ret_code, std::string& message, void* argv, int argc)
{
    FEATURE_CHECK_NE(instance, nullptr);
    if (ret_code == RET_OK) {
        return FEATURE_VALUE_UNDEFINED;
    }
    std::ostringstream oss;
    if (ret_code == RET_ARGS_COUNT_ERR) {
        oss << "args count error";
    } else if (ret_code == RET_ARGS_TYPE_ERR) {
        oss << "args type error";
    } else {
        oss << "internal error";
    }
    oss << ", " << message;
    std::string msg = oss.str();
    ArgsErrorInfo error_info;
    error_info.error_code = FT_ERR_ARGS;
    error_info.error_msg = msg.data();
    error_info.argc = argc;
    error_info.argv = argv;

    auto manager = instance->featureManager();
    if (manager && manager->argsErrorCb()) {
        if (manager->argsErrorCb()(manager->argsErrorData(), &error_info))
            return FEATURE_VALUE_UNDEFINED;
    }
    FEATURE_THROW_INTERNAL_ERROR(ctx, "error: %s", error_info.error_msg);
    return FEATURE_EXCEPTION;
}

static void __feature_finalizer(feature_runtime_ref rt, feature_value_t val)
{
    auto instance = getInstance(val);
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_DEBUG("instance or prototype is null, skip resource free...");
        return;
    }
    auto proto = instance->prototype();

    feature_set_opaque(val, nullptr);
    // get proto pointer, it may not be deleted at this time
    auto iid = instance->instanceId();
    // delete instance by removing it from FeaturePrototype.
    bool ret = proto->removeInstance(iid);
    FEATURE_LOG_DEBUG("deleting instance %p with iid %d ret %d", instance, iid, ret);
    if (!ret) {
        FEATURE_LOG_ERROR("delete iid %d failed !", iid);
    }
    FEATURE_CHECK_EQ(ret, true);
    // may have other clean operation.
    // check if all instance be deleted and we can delete the FeaturePrototype
}

static inline FeaturePrototypeQjs* getPrototype(FeatureInstance* instance)
{
    return static_cast<FeaturePrototypeQjs*>(instance->prototype());
}

static void __feature_mark(feature_runtime_ref rt, feature_value_t val, feature_mark_func mark_func)
{
    FeatureInstance* instance = getInstance(val);
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_DEBUG("instance or prototype is null, skip mark it ...");
        return;
    }

    FeatureInstanceQjs* instance_qjs = (FeatureInstanceQjs*)instance;
    // mark all instance values
    instance_qjs->markValues(rt, mark_func);
}

static feature_value_t method_call(feature_context_ref ctx, feature_value_t this_val,
    int argc, feature_value_t* argv, int magic)
{
    int index = magic;
    auto instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    const Member* member = &description->members[index];
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    RetCode ret_code = methodCall(instance, ctx, ctx, member, argc, argv, ret_val);
    if (ret_code != RET_OK) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", method:" << member->name;
        std::string msg = oss.str();
        return reportArgsError(instance, ctx, ret_code, msg, (void*)argv, argc);
    }
    return ret_val;
}

static feature_value_t accessor_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    int index = magic;
    auto instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    const Member* member = &description->members[index];
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    RetCode ret_code = accessorGet(instance, ctx, member, ret_val);
    if (ret_code != RET_OK) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "property get error");
        return FEATURE_EXCEPTION;
    }
    return ret_val;
}

static feature_value_t accessor_set(feature_context_ref ctx, feature_value_t this_val, feature_value_t val, int magic)
{
    int index = magic;
    auto instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    const Member* member = &description->members[index];
    RetCode ret_code = accessorSet(instance, ctx, member, val);
    if (ret_code != RET_OK) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", property:" << member->name;
        std::string msg = oss.str();
        return reportArgsError(instance, ctx, ret_code, msg, (void*)(&val), 1);
    }
    return FEATURE_VALUE_UNDEFINED;
}

static feature_value_t const_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    int index = magic;
    FeatureInstanceQjs* instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    const Member* member = &description->members[index];
    FEATURE_CHECK_EQ(member->type, MEMBER_CONST);
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    if (!constGet(instance, ctx, member, ret_val)) {
        ret_val = FEATURE_EXCEPTION;
    }

    // for const value, redefine the property with result value
    JS_DefinePropertyValueStr(static_cast<feature_context_ref>(ctx), this_val, member->name,
        feature_dup_value(ctx, ret_val), FEATURE_PROP_CONFIGURABLE);
    return ret_val;
}

static feature_value_t event_set(feature_context_ref ctx, feature_value_t this_val, feature_value_t val, int magic)
{
    int index = magic;
    auto instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    const Member* member = &description->members[index];
    RetCode ret_code = eventSet(instance, ctx, member, val);
    if (ret_code != RET_OK) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", property:" << member->name;
        std::string msg = oss.str();
        return reportArgsError(instance, ctx, ret_code, msg, (void*)(&val), 1);
    }
    return FEATURE_VALUE_UNDEFINED;
}

static feature_value_t event_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    int index = magic;
    auto instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    const Member* member = &description->members[index];
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    if (!eventGet(instance, ctx, member, ret_val)) {
        ret_val = FEATURE_EXCEPTION;
    }
    return ret_val;
}

static const Member* get_event_member(feature_context_ref ctx, FeatureInstanceQjs* instance, feature_value_t name_val)
{
    auto proto = instance->prototype();
    const char* event_name = JS_ToCString(ctx, name_val);
    const Member* member = proto->getEventMember(event_name);
    if (!member) {
        FEATURE_LOG_ERROR("invalid event name: %s", event_name);
    }
    JS_FreeCString(ctx, event_name);
    return member;
}

static feature_value_t event_on_off(feature_context_ref ctx, feature_value_t this_val,
    int argc, feature_value_t* argv, int magic)
{
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    auto instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    const Member* member = get_event_member(ctx, instance, argv[0]);
    if (!member) {
        return ret_val;
    }
    eventOnOff(instance, ctx, member, argv[1], (magic == 0 ? true : false));
    return ret_val;
}

#ifdef NO_C_PLUS_TEMPLATE
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
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    if (instance->isDetached() || instance->prototype() == nullptr) {
        FEATURE_LOG_ERROR("feature is detached, handle:%p", instance);
        return ret_val;
    }
    auto description = instance->prototype()->description();
    const Member& member = description->members[index];
    FEATURE_CHECK_EQ_LOG(member.type, MEMBER_METHOD, "feature:%s method:%s", description->name, member.name);
    const auto method = member.method;
    auto param_types = method->parameters;
    FtPromiseId pid = -1;

    FeatureMethodMeasurer(description->name, member.name);

    // feature_value_t promise_obj = FEATURE_VALUE_UNDEFINED;
    //  count size
    bool has_rest_param = false;
    int optional_argc = 0;
    int32_t param_value_count = 0;
    int fixed_argc = getParamCount(param_types, &has_rest_param, &optional_argc, &param_value_count);
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE_LOG(has_rest_param && optional_argc, true, "feature:%s method:%s", description->name, member.name);
    // variadic parameter param
    FtVariParams vari_params;
    memset(&vari_params, 0, sizeof(vari_params));
    // check argument count match.
    // FEATURE_LOG_DEBUG("required param count: %d, received param count: %d", fixed_argc, argc);
    // beacuse we support rest parameters, so argc is greater or equal to fixed_argc.
    FEATURE_LOG_DEBUG("feature: %s, method: %s, has_rest_param: %d, argc: %d, fixed_argc: %d, opt_argc: %d",
        description->name, member.name, has_rest_param, argc, fixed_argc, optional_argc);
    if (has_rest_param) {
        if (argc < fixed_argc) {
            FEATURE_LOG_ERROR("rest args error, fixed: %d, total: %d!", fixed_argc, argc);
            got_error = true;
        }
        FEATURE_CHECK_GE_LOG(argc, fixed_argc, "feature:%s method:%s", description->name, member.name);
        vari_params.vari_count = argc - fixed_argc;
    } else if (optional_argc) {
        // for optional parameters, argc + optional must grater or equal to fixed_argc
        if (argc + optional_argc < fixed_argc) {
            FEATURE_LOG_ERROR("optional args error, optional: %d, fixed: %d, total: %d!",
                optional_argc, fixed_argc, argc);
            got_error = true;
        }
        FEATURE_CHECK_GE_LOG(argc + optional_argc, fixed_argc, "feature:%s method:%s", description->name, member.name);
    } else {
        // for method which do not have rest or optional parameters, argc equals to fixed_argc.
        if (argc != fixed_argc) {
            FEATURE_LOG_WARN("fixed args worng, fixed: %d, total: %d!", fixed_argc,
                argc);
            if (argc < fixed_argc) {
                got_error = true;
                FEATURE_CHECK_EQ_LOG(argc, fixed_argc, "feature:%s method:%s",
                    description->name, member.name);
            }
            // ignore the case when argc is larger than fixed_argc.
        }
    }

    if (got_error) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", method:" << member.name;
        std::string msg = oss.str();
        return reportArgsError(instance, ctx, RET_ARGS_COUNT_ERR, msg, (void*)argv, argc);
    }
    // if has rest parameter, we will pack all variadic parameters together as a param pack
    // use packed_argc instead of argc for ffi call.
    int32_t packed_argc = has_rest_param ? argc - vari_params.vari_count + 1 : argc;
    // if return value is a promise
    bool is_promise = FT_IS_PROMISE(method->return_type);
    int extra_argc = is_promise ? 1 : 0;
    int total_argc = packed_argc + optional_argc + extra_argc;
    // malloc ffi arg buf
    void** ffi_arg_buf = nullptr;
    if (total_argc > 0) {
        // FeaturInstance, data, maybe return promise
        ffi_arg_buf = (void**)alloca(sizeof(void*) * total_argc);
        memset(ffi_arg_buf, 0, sizeof(void*) * total_argc);
    }
    // malloc ffi arg values, only contains packed param, optional args use &
    int ffi_arg_values_len = sizeof(uintptr_t) * param_value_count;
    uintptr_t* ffi_arg_values = (uintptr_t*)alloca(ffi_arg_values_len);
    memset(ffi_arg_values, 0, ffi_arg_values_len);
    void* ffi_ret_value = nullptr;

    do {
        for (int i = 0; i < fixed_argc && i < argc; i++) {
            feature_value_t currArg = argv[i];
            auto param_type = param_types[i];
            // fill ffi_arg_values into ffi_arg_buf
            ffi_arg_buf[extra_argc + i] = ffi_arg_values;
            // go next
            ffi_arg_values += getAlignedCount(param_type);
            if (FT_IS_PROMISE(param_type)) {
                FEATURE_LOG_ERROR("do not support promise as input param !");
                got_error = true;
                break;
            }
            if (!FeatureFFIQjs::convertValueToHost(instance, param_type, ffi_arg_buf[extra_argc + i], ctx, currArg)) {
                FEATURE_LOG_ERROR("convert argument %d failed !", i);
                got_error = true;
                break;
            }
        }
        if (got_error)
            break;

        // process rest parameters
        if (has_rest_param) {
            // prepare vari_params struct
            vari_params.vari_args = nullptr;
            if (vari_params.vari_count > 0) {
                int vari_params_size = sizeof(ft_value_t) * vari_params.vari_count;
                vari_params.vari_args = (ft_value_t*)alloca(vari_params_size);
                memset(vari_params.vari_args, 0, vari_params_size);
            }
            // pass param
            ffi_arg_buf[fixed_argc + extra_argc] = &vari_params;
            for (int i = 0; i + fixed_argc < argc; i++) {
                // just passthrough guest param pointers
                auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(vari_params.vari_args[i]);
                *js_val_ptr = argv[i + fixed_argc];
            }
        } else if (optional_argc) {
            for (int i = argc; i < fixed_argc; i++) {
                auto param_type = param_types[i];
                FEATURE_CHECK_EQ_LOG(FT_IS_COMPLEX(param_type), true, "feature:%s method:%s", description->name, member.name);
                OptionalType* optionalType = (OptionalType*)FT_GET_COMPLEX(param_type);
                FEATURE_CHECK_EQ_LOG(optionalType->header.type, COMPLEX_OPTIONAL, "feature:%s method:%s", description->name, member.name);
                ffi_arg_buf[extra_argc + i] = &optionalType->fval;
                FeatureDupValue(ffi_arg_buf[extra_argc + i]);
            }
        }

        if (got_error)
            break;

        // create return value pointer inneed.
        if (!is_promise && method->return_type != FT_VOID) {
            ALLOCA_PARAM_PTR(method->return_type, ffi_ret_value);
        }

        // special handle for promise
        if (is_promise) {
            ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(method->return_type);
            // create promise
            PromiseType* promise_type = (PromiseType*)complex_type;
            // create promise and add to instance
            pid = ((FeatureInstanceQjs*)instance)->addPromise(promise_type->resolveTypes[0], promise_type->resolveTypes[1]);
            feature_value_t promise = ((FeatureInstanceQjs*)instance)->getPromise(pid);
            // pass pid to native function
            ffi_arg_buf[0] = &pid;
            // dup and return promise object.
            ret_val = feature_dup_value(ctx, promise);
        }
        // invoke method
        StubFunc func_stub = method->func_stub;
        FEATURE_CHECK_NE(func_stub, nullptr);
        func_stub(instance, method->data, ffi_arg_buf, total_argc, ffi_ret_value);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!is_promise && method->return_type != FT_VOID) {
            // process return value
            if ((FT_IS_COMPLEX(method->return_type)) && (((ComplexTypeHeader*)(FT_GET_COMPLEX(method->return_type)))->type == COMPLEX_STRUCT_MAP)
                && (*(void**)ffi_ret_value == nullptr)) {
                FEATURE_LOG_ERROR("struct ffi_ret_value is null !");
                feature_free_value(ctx, ret_val);
                ret_val = FEATURE_UNDEFINED;
                got_error = true;
            } else if (!FeatureFFIQjs::convertValueToGuest(method->return_type, ffi_ret_value, ctx, ret_val)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                feature_free_value(ctx, ret_val);
                ret_val = FEATURE_EXCEPTION;
                got_error = true;
            } else if (FT_IS_PRIMITIVE(method->return_type) && method->return_type == FT_ANY_REF) {
                feature_free_value(ctx, ret_val);
            }
        }
    } while (0);

    // free ffi call resources
    for (int i = 0; i < fixed_argc; i++) {
        // free value
        if (ffi_arg_buf[i + extra_argc] && FT_NEED_FREE(param_types[i])) {
            FeatureFreeValue(*(void**)ffi_arg_buf[i + extra_argc]);
        }
    }
    if (ffi_ret_value && FT_NEED_FREE(method->return_type)) {
        FeatureFreeValue(*(void**)ffi_ret_value);
    }
    // if error occurred, throw internal error
    if (got_error) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", method:" << member.name;
        std::string msg = oss.str();
        return reportArgsError(instance, ctx, RET_ARGS_TYPE_ERR, msg, (void*)argv, argc);
    }

    return ret_val;
}

static feature_value_t const_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    // get info from this_val
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    if (instance->isDetached() || instance->prototype() == nullptr) {
        FEATURE_LOG_ERROR("feature is detached, handle:%p", instance);
        return ret_val;
    }
    Member* member = const_cast<Member*>(&instance->prototype()->description()->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_CONST);
    MemberConst* member_const = const_cast<MemberConst*>(member->value);
    void* data = &member_const->data;
    FeatureType ftype = member_const->type;
    FEATURE_CHECK_NE(ftype, FT_VOID);

    if (!FeatureFFIQjs::convertValueToGuest(ftype, data, ctx, ret_val)) {
        FEATURE_LOG_ERROR("can not convert const value to guest!");
        feature_free_value(ctx, ret_val);
        ret_val = FEATURE_VALUE_UNDEFINED;
    }
    // for const value, redefine the property with result value
    JS_DefinePropertyValueStr(static_cast<feature_context_ref>(ctx), this_val, member->name,
        feature_dup_value(ctx, ret_val), FEATURE_PROP_CONFIGURABLE);
    return ret_val;
}

static feature_value_t accessor_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    // get info from this_val
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    FeatureInstance* instance = getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->prototype()->description()->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = const_cast<MemberAccessor*>(member->accessor);
    FeatureType ftype = accessor->type;
    FEATURE_CHECK_NE(ftype, FT_VOID);
    StubFunc getter_stub = accessor->getter_stub;
    FEATURE_CHECK_NE(getter_stub, nullptr);
    // handle parameters
    void* ffi_ret_value = nullptr;
    ALLOCA_PARAM_PTR(ftype, ffi_ret_value);

    // invoke
    getter_stub(instance, accessor->data, nullptr, 0, ffi_ret_value);
    // process return value
    if (!FeatureFFIQjs::convertValueToGuest(ftype, ffi_ret_value, ctx, ret_val)) {
        FEATURE_LOG_ERROR("can not convert return value to guest!");
        feature_free_value(ctx, ret_val);
        ret_val = FEATURE_EXCEPTION;
    }

    // free resources
    if (FT_NEED_FREE(ftype)) {
        FeatureFreeValue(*(void**)ffi_ret_value);
    }
    return ret_val;
}

static feature_value_t accessor_set(feature_context_ref ctx, feature_value_t this_val, feature_value_t val, int magic)
{
    // get info from this_val
    int index = magic;
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    FeatureInstance* instance = getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    if (instance->isDetached() || instance->prototype() == nullptr) {
        FEATURE_LOG_ERROR("feature is detached, handle:%p", instance);
        return ret_val;
    }
    auto description = instance->prototype()->description();
    const Member& member = description->members[index];
    FEATURE_CHECK_EQ(member.type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = const_cast<MemberAccessor*>(member.accessor);
    FeatureType ftype = accessor->type;
    FEATURE_CHECK_NE(ftype, FT_VOID);
    StubFunc setter_stub = accessor->setter_stub;
    FEATURE_CHECK_NE(setter_stub, nullptr);

    // handle parameter
    void* arg_value = nullptr;
    ALLOCA_PARAM_PTR(ftype, arg_value);
    void* ffi_arg_values[1] = { 0 };
    bool got_error = false;
    do {
        // fill third param using guest value and accesor type
        if (!FeatureFFIQjs::convertValueToHost(instance, ftype, arg_value, ctx, val)) {
            FEATURE_LOG_ERROR("convert to host value failed !");
            got_error = true;
            break;
        }
        ffi_arg_values[0] = arg_value;
        // invoke
        setter_stub(instance, accessor->data, ffi_arg_values, 1, ffi_arg_values[0]);
    } while (0);
    // free resources
    if (FT_NEED_FREE(ftype)) {
        FeatureFreeValue(*(void**)arg_value);
    }

    if (got_error) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", property:" << member.name;
        std::string msg = oss.str();
        return reportArgsError(instance, ctx, RET_ARGS_TYPE_ERR, msg, (void*)(&val), 1);
    }
    return FEATURE_VALUE_UNDEFINED;
}
#endif

static void register_event_on_off_functions(feature_context_ref ctx, feature_value_t js_proto)
{
    JSValue on_func = JS_GetPropertyStr((JSContext*)ctx, js_proto, "on");
    if (JS_IsUndefined(on_func)) {
        FEATURE_LOG_DEBUG("register event_on function!");
        feature_value_t on_call_func = JS_NewCFunctionMagic((JSContext*)ctx, event_on_off, "on", 2, JS_CFUNC_generic_magic, 0);
        feature_define_object_property(ctx, js_proto, "on", on_call_func, FEATURE_PROP_ENUMERABLE);
    }
    JSValue off_func = JS_GetPropertyStr(static_cast<feature_context_ref>(ctx), js_proto, "off");
    if (JS_IsUndefined(off_func)) {
        FEATURE_LOG_DEBUG("register event_off function!");
        feature_value_t off_call_func = JS_NewCFunctionMagic((JSContext*)ctx, event_on_off, "off", 2, JS_CFUNC_generic_magic, 1);
        feature_define_object_property(ctx, js_proto, "off", off_call_func, FEATURE_PROP_ENUMERABLE);
    }
    feature_free_value(ctx, on_func);
    feature_free_value(ctx, off_func);
}

static int init_prototype(context_ref ctx, FeaturePrototype* prototype, feature_value_t js_proto)
{
    FEATURE_CHECK(prototype != nullptr && prototype->description() != nullptr, "");
    auto description = prototype->description();
    for (int i = 0; i < description->member_count; i++) {
        const Member& member = description->members[i];
        switch (member.type) {
        case MEMBER_NULL: {
            // not allowed
            FEATURE_CHECK(false, "invalid member type!");
        } break;
        case MEMBER_METHOD: {
            // register different type
            const MemberMethod* method = member.method;
            feature_value_t methodCallObj = JS_NewCFunctionMagic(static_cast<feature_context_ref>(ctx), method_call, member.name, getParamCount(method->parameters), JS_CFUNC_generic_magic, i);
            feature_define_object_property(ctx, js_proto, member.name, methodCallObj, FEATURE_PROP_ENUMERABLE);
        } break;
        case MEMBER_ACCESSOR: {
            // create getter and setter
            const MemberAccessor* accessor = member.accessor;
            feature_atom_t prop_name = feature_atom(static_cast<feature_context_ref>(ctx), member.name);
            feature_value_t funcs[2] = { FEATURE_VALUE_UNDEFINED, FEATURE_VALUE_UNDEFINED };

            char buf[128];
            JSCFunctionType type;
            if (accessor->getter_stub) {
                type.getter_magic = accessor_get;
                sprintf(buf, "get %s", member.name);
                funcs[0] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 0, JS_CFUNC_getter_magic, i);
            }
            if (accessor->setter_stub) {
                type.setter_magic = accessor_set;
                sprintf(buf, "set %s", member.name);
                funcs[1] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 1, JS_CFUNC_setter_magic, i);
            }
            JS_DefinePropertyGetSet(static_cast<feature_context_ref>(ctx), js_proto, prop_name, funcs[0], funcs[1], FEATURE_PROP_CONFIGURABLE);
            feature_free_atom(static_cast<feature_context_ref>(ctx), prop_name);
        } break;
        case MEMBER_CONST: {
            // handle member const
            char buf[128];
            JSCFunctionType type;
            type.getter_magic = const_get;
            sprintf(buf, "get %s", member.name);
            feature_value_t const_member_getter = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 0, JS_CFUNC_getter_magic, i);
            feature_atom_t prop_name = feature_atom(static_cast<feature_context_ref>(ctx), member.name);
            JS_DefinePropertyGetSet(static_cast<feature_context_ref>(ctx), js_proto, prop_name, const_member_getter, FEATURE_VALUE_UNDEFINED, FEATURE_PROP_CONFIGURABLE);
            feature_free_atom(static_cast<feature_context_ref>(ctx), prop_name);
        } break;
        case MEMBER_EVENT: {
            char buf[128];
            JSCFunctionType type;
            // for getter
            type.getter_magic = event_get;
            sprintf(buf, "get %s", member.name);
            feature_value_t event_getter = JS_NewCFunction2((JSContext*)(ctx), type.generic, buf, 0, JS_CFUNC_getter_magic, i);
            // for setter
            type.setter_magic = event_set;
            sprintf(buf, "set %s", member.name);
            feature_value_t event_setter = JS_NewCFunction2((JSContext*)(ctx), type.generic, buf, 1, JS_CFUNC_setter_magic, i);
            feature_atom_t event_name = feature_atom((JSContext*)(ctx), member.name);
            JS_DefinePropertyGetSet((JSContext*)(ctx), js_proto, event_name, event_getter, event_setter, FEATURE_PROP_CONFIGURABLE);
            feature_free_atom((JSContext*)(ctx), event_name);
            // for on/off funcs
            prototype->setEventMember(member.name, &member);
        } break;
        }
    }
    return 0;
}

static int uninit_prototype(JSContext* ctx, const FeatureDescription* description, feature_value_t js_proto)
{
    FEATURE_CHECK(description != nullptr, "");
    for (int i = 0; i < description->member_count; i++) {
        const Member& member = description->members[i];
        switch (member.type) {
        case MEMBER_NULL: {
            // not allowed
            FEATURE_CHECK(false, "invalid member type!");
        } break;
        case MEMBER_METHOD:
        case MEMBER_ACCESSOR:
        case MEMBER_CONST:
        case MEMBER_EVENT: {
            // register different type
            JSAtom name = JS_NewAtom(ctx, member.name);
            JS_DeleteProperty(ctx, js_proto, name, JS_PROP_THROW);
            JS_FreeAtom(ctx, name);
        } break;
        }
    }
    return 0;
}

FeatureManager* CreateQJSFeatureManager(FeatureRegistry* registry, FeatureManagerCreateInfo* pinfo)
{
    return new FeatureManagerQjs(registry, (feature_context_ref)(pinfo->raw_ctx));
}

// static members
feature_classid_t FeatureManagerQjs::js_class_id_ = 0;
feature_classdef_t FeatureManagerQjs::js_class_def_ = {
    .class_name = "FeatureInstanceObject",
    .finalizer = __feature_finalizer,
    .gc_mark = __feature_mark
};
uv_mutex_t FeatureManagerQjs::js_class_mutex_ = PTHREAD_MUTEX_INITIALIZER;

// static methods
bool FeatureManagerQjs::ensureJsClass(feature_context_ref ctx)
{
    auto rt = JS_GetRuntime(ctx);
    // here we do twice judgement for js_class_id. the one outside the mutex scope is for fast
    // judgement, the other inside the mutex scope is to prevent thread racing coditions.
    if (js_class_id_ != 0 && JS_IsRegisteredClass(rt, js_class_id_)) {
        FEATURE_LOG_DEBUG("class_id already registered.");
        return true;
    }

    uv_mutex_lock(&js_class_mutex_);
    if (js_class_id_ != 0 && JS_IsRegisteredClass(rt, js_class_id_)) {
        FEATURE_LOG_DEBUG("class_id already registered.");
        uv_mutex_unlock(&js_class_mutex_);
        return true;
    }
    FEATURE_LOG_INFO("last class_id: %d.", js_class_id_);

    js_class_id_ = JS_NewClassID(&js_class_id_);
    if (js_class_id_ == 0) {
        FEATURE_LOG_ERROR("create js class_id failed.");
        uv_mutex_unlock(&js_class_mutex_);
        return false;
    }

    FEATURE_LOG_INFO("created class_id: %d.", js_class_id_);
    JS_NewClass(rt, js_class_id_, &js_class_def_);
    uv_mutex_unlock(&js_class_mutex_);
    return true;
}

FeatureManagerQjs::FeatureManagerQjs(FeatureRegistry* registry, feature_context_ref ctx)
    : FeatureManager(registry)
{
    ft_context_ref ft_ctx = CreateFeatureContextQjs(ctx);
    setFeatureContext(ft_ctx);
}

FeatureManagerQjs::~FeatureManagerQjs()
{
}

bool FeatureManagerQjs::ensureJsPrototype(FeaturePrototypeQjs* prototype)
{
    auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(prototype->ft_proto());
    if (!feature_is_undefined(*js_proto_ptr))
        return true;

    auto ctx = (feature_context_ref)ft_context_get_data(getFeatureContext());
    FEATURE_CHECK_NE(ctx, nullptr);
    feature_value_t js_proto = feature_object(ctx);
    if (feature_is_exception(js_proto)) {
        feature_dump_error(ctx);
        return false;
    }

    init_prototype(ctx, prototype, js_proto);
    register_event_on_off_functions(ctx, js_proto);
    // TODO: initialize js_proto using description
    if (prototype->description()->native_callbacks && prototype->description()->native_callbacks->onCreate) {
        FEATURE_LOG_DEBUG("invoke onCreate callback...");

        FeatureMethodMeasurer(prototype->description()->name, "onCreate");
        prototype->description()->native_callbacks->onCreate(ctx, prototype);
    }

    *js_proto_ptr = js_proto;
    return true;
}

feature_value_t FeatureManagerQjs::createJsInstance(FeaturePrototypeQjs* prototype, FeatureInstance* instance)
{
    auto ctx = (feature_context_ref)ft_context_get_data(getFeatureContext());
    FEATURE_CHECK_NE(ctx, nullptr);
    // ensure js feature prototype is created
    if (!ensureJsPrototype(prototype))
        return FEATURE_VALUE_UNDEFINED;

    if (!ensureJsClass(ctx)) {
        FEATURE_LOG_ERROR("invalid js class_id: %d", js_class_id_);
        return FEATURE_VALUE_UNDEFINED;
    }

    // create instance with prototype and set opaque refers to FeatureInstance
    FEATURE_LOG_DEBUG("created js instance with class_id: %d.", js_class_id_);
    auto js_proto = FT_VAL_GET_JS_VAL(prototype->ft_proto());
    feature_value_t js_instance = JS_NewObjectProtoClass(ctx, js_proto, js_class_id_);
    feature_set_opaque(js_instance, instance);
#ifdef CONFIG_FEATURE_USE_JS_FUNCTION_BINDING
    bind_js_functions_to_instance(ctx, js_proto, js_instance);
#endif
    // setup instance WeakRef, refers to js_instance
    ((FeatureInstanceQjs*)instance)->initWeakRef(js_instance);
    return js_instance;
}

ft_value_t FeatureManagerQjs::featureRequire(ft_value_t binding_obj, const char* name)
{
    FEATURE_LOG_DEBUG("featureRequire for '%s'", name);
    ft_context_ref ft_ctx = getFeatureContext();
    FEATURE_CHECK_NE(ft_ctx, nullptr);
    ft_value_t ret;
    auto js_ret_ptr = FT_VAL_GET_JS_VAL_PTR(ret);
    *js_ret_ptr = FEATURE_VALUE_UNDEFINED;
    auto pDesc = getFeatureRegistry()->findFeature(name);
    if (!pDesc) {
        FEATURE_LOG_DEBUG("can't find native feature '%s', fallback to original JS module load!", name);
        return ret;
    }

    auto& prototypes = getFeaturePrototypes();

    auto& prototype = prototypes[name];
    if (!prototype) {
        // create proto
        prototype = new FeaturePrototypeQjs(pDesc);
        prototype->setFeatureManager(this);
        setPackageName(getFeatureRegistry()->getFeaturePackageName());
        setEnvName(FEATURE_ENV_NAME);
    }

    // create feature instance for the required object
    FeatureObjectUniquePtr<FeatureInstanceQjs> instance(new FeatureInstanceQjs(prototype));
    auto instance_ptr = instance.get();
    // save binding_obj into instance
    auto js_binding_obj = FT_VAL_GET_JS_VAL(binding_obj);
    instance->setVmObject(js_binding_obj);

    // insert into instances array, update iid
    int iid = prototype->addInstance(std::move(instance));
    instance_ptr->setInstanceId(iid);

    auto js_instance = createJsInstance((FeaturePrototypeQjs*)prototype, instance_ptr);
    if (pDesc->native_callbacks && pDesc->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        FeatureMethodMeasurer(prototype->description()->name, "onRequired");
        auto ctx = (feature_context_ref)ft_context_get_data(ft_ctx);
        pDesc->native_callbacks->onRequired(ctx, instance_ptr);
    }
    *js_ret_ptr = js_instance;
    return ret;
}

feature_value_t FeatureManagerQjs::createTargetInterface(FeatureInstance* interf)
{
    FEATURE_CHECK_NE(interf, nullptr);
    FeaturePrototypeQjs* proto = static_cast<FeaturePrototypeQjs*>(interf->prototype());
    FEATURE_CHECK_NE(proto, nullptr);
    auto js_interface = createJsInstance(proto, interf);
    return js_interface;
}

void FeatureManagerQjs::uninit()
{
    auto ft_ctx = getFeatureContext();
    if (!ft_ctx) {
        FEATURE_LOG_WARN("ft_ctx is missing");
        delete getFeatureRegistry();
        return;
    }

    detachFeatureInstances();

    JSContext* js_ctx = (JSContext*)ft_context_get_data(ft_ctx);
    auto free_prototype = [js_ctx](FeaturePrototypeQjs* prototype) {
        auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(prototype->ft_proto());
        if (!feature_is_undefined(*js_proto_ptr)) {
            uninit_prototype(js_ctx, prototype->description(), *js_proto_ptr);
            feature_free_value(js_ctx, *js_proto_ptr);
            *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
        }
    };

    for (const auto& pair : getFeaturePrototypes()) {
        auto proto = static_cast<FeaturePrototypeQjs*>(pair.second);
        auto description = getFeatureRegistry()->findFeature(pair.first.c_str());
        FEATURE_CHECK_NE(description, nullptr);
        if (!proto)
            continue;

        for (auto& proto_pair : proto->children()) {
            // clear all interface instances belongs to this instance.
            auto child_proto = static_cast<FeaturePrototypeQjs*>(proto_pair.second.get());
            child_proto->clearAllInstances();
            FEATURE_LOG_INFO("free interface prototype '%s'", proto_pair.first);
            free_prototype(child_proto);
        }

        // clear all feature instance at first, it will free all feature instance and call onDetach for them
        proto->clearAllInstances();
        // call feature's onDestroy
        if (description->native_callbacks && description->native_callbacks->onDestroy) {
            FEATURE_LOG_DEBUG("invoke onDestroy callback...");
            FeatureMethodMeasurer(description->name, "onDestroy");
            description->native_callbacks->onDestroy(js_ctx, proto);
        }
        FEATURE_LOG_DEBUG("free feature prototype '%s'", description->name);
        free_prototype(proto);
        delete proto;
    }

    // uninit registery
    delete getFeatureRegistry();
    ReleaseFeatureContextQjs(ft_ctx);
    setFeatureContext(nullptr);
}

ft_value_t FeatureManagerQjs::findFeature(const char* name)
{
    FEATURE_LOG_DEBUG("findFeature for '%s'", name);
    ft_context_ref ft_ctx = getFeatureContext();
    FEATURE_CHECK_NE(ft_ctx, nullptr);
    ft_value_t ret;
    auto js_ret_ptr = FT_VAL_GET_JS_VAL_PTR(ret);
    *js_ret_ptr = FEATURE_VALUE_UNDEFINED;
    const FeatureDescription* pDesc = getFeatureRegistry()->findFeature(name);
    if (!pDesc) {
        FEATURE_LOG_WARN("can't find description for native feature '%s'!", name);
        return ret;
    }

    // create proto
    auto& prototype = getFeaturePrototypes()[pDesc->name];
    if (!prototype) {
        prototype = new FeaturePrototypeQjs(pDesc);
        prototype->setFeatureManager(this);
        setPackageName(getFeatureRegistry()->getFeaturePackageName());
        setEnvName(FEATURE_ENV_NAME);
    }

    if (!ensureJsPrototype((FeaturePrototypeQjs*)prototype)) {
        FEATURE_LOG_ERROR("ensure js prototype failed !");
        return ret;
    }

    auto js_proto = FT_VAL_GET_JS_VAL(((FeaturePrototypeQjs*)prototype)->ft_proto());
    auto ctx = (feature_context_ref)ft_context_get_data(ft_ctx);
    *js_ret_ptr = feature_dup_value(ctx, js_proto);
    return ret;
}

ft_value_t FeatureManagerQjs::createFeature(ft_value_t proto, ft_value_t binding_obj)
{
    auto ctx = (feature_context_ref)ft_context_get_data(getFeatureContext());
    FEATURE_CHECK_NE(ctx, nullptr);
    ft_value_t ret;
    auto js_ret_ptr = FT_VAL_GET_JS_VAL_PTR(ret);
    *js_ret_ptr = FEATURE_VALUE_UNDEFINED;
    for (const auto& pair : getFeaturePrototypes()) {
        auto prototype = static_cast<FeaturePrototypeQjs*>(pair.second);
        if (!prototype)
            continue;

        auto js_proto = FT_VAL_GET_JS_VAL(prototype->ft_proto());
        auto js_proto_arg = FT_VAL_GET_JS_VAL(proto);
        if (!feature_is_same_value(ctx, js_proto, js_proto_arg))
            continue;

        // create feature instance for the required object
        FeatureObjectUniquePtr<FeatureInstanceQjs> instance(new FeatureInstanceQjs(prototype));
        // save binding_obj into instance
        auto js_binding_obj = FT_VAL_GET_JS_VAL(binding_obj);
        instance->setVmObject(js_binding_obj);
        auto instance_ptr = instance.get();
        // insert into instances array, update iid
        int iid = prototype->addInstance(std::move(instance));
        instance_ptr->setInstanceId(iid);

        // create prototype class instance
        auto description = getFeatureRegistry()->findFeature(pair.first.c_str());
        auto js_instance = createJsInstance(prototype, instance_ptr);
        if (description->native_callbacks && description->native_callbacks->onRequired) {
            FEATURE_LOG_DEBUG("invoke onRequired callback...");
            FeatureMethodMeasurer(description->name, "onRequired");
            description->native_callbacks->onRequired(ctx, instance_ptr);
        }
        *js_ret_ptr = js_instance;
        return ret;
    }

    return ret;
}

}
