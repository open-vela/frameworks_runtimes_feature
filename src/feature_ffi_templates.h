
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

#ifndef __FEATURE_FFI_TEMPLATES_H__
#define __FEATURE_FFI_TEMPLATES_H__

#include "feature_convertor_templates.h"
#include "feature_log.h"
#include "feature_utils.h"

#include "feature_common.h"
#include "feature_ffi.h"

#include <cstdarg>
#include <stdalign.h>

#define HANDLE_ERROR_AND_RETURN(ret_code)                              \
    {                                                                  \
        if (is_promise) {                                              \
            auto err_msg = get_error_msg(ret_code, mthd_msg);          \
            instance->rejectPromise(pid, FT_ERR_ARGS, err_msg.data()); \
            return RET_OK;                                             \
        }                                                              \
        return ret_code;                                               \
    }

namespace feature_framework {

typedef enum RetCode {
    RET_OK,
    RET_ARGS_COUNT_ERR,
    RET_ARGS_TYPE_ERR,
    RET_INTERNAL_ERR
} RetCode;

static inline void free_method_args(int fixed_argc, int extra_argc,
    const FeatureType* param_types, void** ffi_arg_buf)
{
    for (int j = 0; j < fixed_argc; j++) {
        auto arg = (void**)ffi_arg_buf[j + extra_argc];
        if (arg && *arg && FT_NEED_FREE(param_types[j])) {
            FeatureFreeValue(*arg);
        }
    }
}

static std::string get_error_msg(RetCode ret_code, std::string& mthd_msg)
{
    std::string err_msg;
    if (ret_code == RET_OK) {
        err_msg = "no error";
    } else if (ret_code == RET_ARGS_COUNT_ERR) {
        err_msg = "args count error";
    } else if (ret_code == RET_ARGS_TYPE_ERR) {
        err_msg = "args type error";
    } else {
        err_msg = "internal error";
    }
    err_msg += ", ";
    err_msg += mthd_msg;
    return err_msg;
}

template <typename TInstance, typename TCtx, typename TTarget>
RetCode methodCall(TInstance* instance, TCtx ctx, JSContext* js_ctx,
    const Member* member, int argc, TTarget* argv, TTarget& ret_val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    auto description = instance->prototype()->description();
    const auto method = member->method;
    auto param_types = method->parameters;
    FeatureType ret_type = method->return_type;

    FtPromiseId pid = -1;
    bool has_rest_params = false;
    int opt_argc = 0;
    int32_t int32_count = 0;
    int fixed_argc = getParamCount(param_types, &has_rest_params, &opt_argc, &int32_count);
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_params && opt_argc, true);

    // special handle for promise
    bool is_promise = FT_IS_PROMISE(ret_type);
    bool use_promise = is_promise;
    if (is_promise) {
        PromiseType* promise_type = (PromiseType*)FT_GET_COMPLEX(ret_type);
        if (argc > 0) {
            int flag = value_translator::checkAsyncCallbacks(ctx, argv[argc - 1]);
            if (flag == -1) {
                FEATURE_LOG_ERROR("feature: %s, method: %s, checkAsyncCallbacks failed!",
                    description->name, member->name);
                return RET_ARGS_TYPE_ERR;
            }
            use_promise = flag ? false : true;
        }
        // create promise
        if (!use_promise) {
            pid = value_translator::addAsyncCallbacks(ctx, instance, promise_type->resolveType, argv[argc - 1]);
        } else {
            // create promise and add to instance
            pid = instance->addPromise(promise_type->resolveType);
            feature_value_t promise = feature_dup_value(js_ctx, instance->getPromise(pid));
            ret_val = value_translator::toTargetPromise(ctx, js_ctx, promise);
        }
        size_t pcount = instance->promiseCount();
        if (pcount >= CONFIG_FEATURE_MAX_PROMISE_COUNT) {
            FEATURE_LOG_WARN("promise count of instance[%p]: %d, which is larger than %d, feature: %s, method: %s",
                instance, pcount, CONFIG_FEATURE_MAX_PROMISE_COUNT, description->name, member->name);
        }
    }
    std::string mthd_msg("feature:");
    mthd_msg += description->name;
    mthd_msg += ", method:";
    mthd_msg += member->name;
    // variadic parameters
    FtVariParams vari_params;
    memset(&vari_params, 0, sizeof(vari_params));
    FEATURE_LOG_DEBUG("%s, has_rest_params: %d, argc: %d, fixed_argc: %d, opt_argc: %d",
        mthd_msg.data(), has_rest_params, argc, fixed_argc, opt_argc);
    // beacuse we support rest parameters, so argc is greater or equal to fixed_argc.
    if (has_rest_params) {
        if (argc < fixed_argc) {
            FEATURE_LOG_ERROR("%s, rest args error, fixed: %d, total: %d!",
                mthd_msg.data(), fixed_argc, argc);
            HANDLE_ERROR_AND_RETURN(RET_ARGS_COUNT_ERR)
        }
        vari_params.vari_count = argc - fixed_argc;
    } else if (opt_argc > 0) {
        // for optional parameters, argc + optional must grater or equal to fixed_argc
        if (argc + opt_argc < fixed_argc) {
            FEATURE_LOG_ERROR("%s, optional args error, optional: %d, fixed: %d, total: %d!",
                mthd_msg.data(), opt_argc, fixed_argc, argc);
            HANDLE_ERROR_AND_RETURN(RET_ARGS_COUNT_ERR)
        }
    } else {
        // for method which do not have rest or optional parameters, argc equals to fixed_argc.
        if (argc != fixed_argc && (!is_promise || use_promise)) {
            FEATURE_LOG_ERROR("%s, fixed args error, fixed: %d, total: %d!",
                mthd_msg.data(), fixed_argc, argc);
            HANDLE_ERROR_AND_RETURN(RET_ARGS_COUNT_ERR)
        }
    }

    // prepare and get args
    int32_t packed_argc = has_rest_params ? fixed_argc + 1 : fixed_argc;
    int extra_argc = is_promise ? 1 : 0;
    int total_argc = packed_argc + opt_argc + extra_argc;
    // FeaturInstance, data, maybe return promise
    void** ffi_arg_buf = nullptr;
    if (total_argc > 0) {
        ffi_arg_buf = (void**)alloca(sizeof(void*) * total_argc);
        memset(ffi_arg_buf, 0, sizeof(void*) * total_argc);
        if (is_promise) {
            ffi_arg_buf[0] = &pid;
        }
    }
    // malloc ffi arg values, only contains packed param, optional args use &
    int ffi_arg_values_len = sizeof(uintptr_t) * int32_count;
    uintptr_t* ffi_arg_values = (uintptr_t*)alloca(ffi_arg_values_len);
    memset(ffi_arg_values, 0, ffi_arg_values_len);
    void* ffi_ret_value = nullptr;

    for (int i = 0; i < fixed_argc && i < argc; i++) {
        TTarget curr_arg = argv[i];
        auto param_type = param_types[i];
        // fill ffi_arg_values into ffi_arg_buf and go next
        ffi_arg_buf[extra_argc + i] = ffi_arg_values;
        ffi_arg_values += getAlignedCount(param_type);
        if (FT_IS_PROMISE(param_type)) {
            FEATURE_LOG_ERROR("do not support promise as input param!");
            free_method_args(fixed_argc, extra_argc, param_types, ffi_arg_buf);
            HANDLE_ERROR_AND_RETURN(RET_ARGS_TYPE_ERR)
        }
        if (!convertValueToNative(instance, param_type, ctx, curr_arg, ffi_arg_buf[extra_argc + i])) {
            FEATURE_LOG_ERROR("convert argument %d failed!", i);
            free_method_args(fixed_argc, extra_argc, param_types, ffi_arg_buf);
            HANDLE_ERROR_AND_RETURN(RET_ARGS_TYPE_ERR)
        }
    }

    // process rest parameters
    if (has_rest_params) {
        // prepare vari_params struct
        vari_params.vari_args = (ft_value_t*)alloca(sizeof(ft_value_t) * vari_params.vari_count);
        // pass param
        ffi_arg_buf[fixed_argc + extra_argc] = &vari_params;
        for (int i = 0; i + fixed_argc < argc; i++) {
            // just passthrough guest param pointers
            *((JSValue*)&vari_params.vari_args[i]) = value_translator::getVariArg(ctx, (argv + fixed_argc), i);
        }
    } else if (opt_argc > 0) {
        for (int i = argc; i < fixed_argc; i++) {
            auto param_type = param_types[i];
            FEATURE_CHECK_EQ(FT_IS_COMPLEX(param_type), true);
            OptionalType* opt_type = (OptionalType*)FT_GET_COMPLEX(param_type);
            FEATURE_CHECK_EQ(opt_type->header.type, COMPLEX_OPTIONAL);
            ffi_arg_buf[extra_argc + i] = ffi_arg_values;
            if (!convertOptional(opt_type, ffi_arg_buf[extra_argc + i])) {
                free_method_args(fixed_argc, extra_argc, param_types, ffi_arg_buf);
                HANDLE_ERROR_AND_RETURN(RET_ARGS_TYPE_ERR)
            }
            ffi_arg_values += getAlignedCount(param_type);
        }
    }

    // create return value pointer inneed.
    if (!is_promise && ret_type != FT_VOID) {
        ALLOCA_PARAM_PTR(ret_type, ffi_ret_value);
    }

    // invoke method
    StubFunc func_stub = method->func_stub;
    FEATURE_CHECK_NE(func_stub, nullptr);
    instance->setErrorMsg("");
    func_stub(instance, method->data, ffi_arg_buf, total_argc, ffi_ret_value);
    // process return value, do not handle promise, it is handled before we invoke ffi_call.
    if (!is_promise && ret_type != FT_VOID) {
        // process return value
        auto ft_ctx = instance->featureManager()->getFeatureContext();
        if (!convertValueToTarget(ret_type, ctx, ffi_ret_value, ret_val)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            free_method_args(fixed_argc, extra_argc, param_types, ffi_arg_buf);
            freeFtValue(ft_ctx, ret_type, ffi_ret_value);
            if (ffi_ret_value && FT_NEED_FREE(ret_type)) {
                FeatureFreeValue(*(void**)ffi_ret_value);
            }
            return RET_INTERNAL_ERR;
        } else if (FT_IS_PRIMITIVE(ret_type) && ret_type == FT_ANY_REF) {
            value_translator::freeValue(ctx, ret_val);
        }
    }

    // free call resources
    free_method_args(fixed_argc, extra_argc, param_types, ffi_arg_buf);

    if (ffi_ret_value && FT_NEED_FREE(ret_type)) {
        FeatureFreeValue(*(void**)ffi_ret_value);
    }
    if (instance->hasException()) {
        value_translator::freeValue(ctx, ret_val);
        return RET_INTERNAL_ERR;
    }

    return RET_OK;
}

template <typename TInstance, typename TCtx, typename TTarget>
RetCode accessorGet(TInstance* instance, TCtx ctx, const Member* member, TTarget& ret_val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);

    MemberAccessor* accessor = const_cast<MemberAccessor*>(member->accessor);
    FeatureType ftype = accessor->type;
    FEATURE_CHECK_NE(ftype, FT_VOID);

    // handle parameter
    void* ffi_ret_value = nullptr;
    ALLOCA_PARAM_PTR(ftype, ffi_ret_value);

    // invoke
    StubFunc getter_stub = accessor->getter_stub;
    FEATURE_CHECK_NE(getter_stub, nullptr);
    getter_stub(instance, accessor->data, nullptr, 0, ffi_ret_value);

    // process return value
    if (!convertValueToTarget(ftype, ctx, ffi_ret_value, ret_val)) {
        FEATURE_LOG_ERROR("can not convert return value to guest!");
        value_translator::freeValue(ctx, ret_val);
        return RET_INTERNAL_ERR;
    } else if (FT_IS_PRIMITIVE(ftype) && ftype == FT_ANY_REF) {
        value_translator::freeValue(ctx, ret_val);
    }

    if (FT_NEED_FREE(ftype)) {
        FeatureFreeValue(*(void**)ffi_ret_value);
    }
    return RET_OK;
}

template <typename TInstance, typename TCtx, typename TTarget>
RetCode accessorSet(TInstance* instance, TCtx ctx, const Member* member, TTarget& val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = const_cast<MemberAccessor*>(member->accessor);
    FeatureType ftype = accessor->type;
    FEATURE_CHECK_NE(ftype, FT_VOID);

    // handle parameter
    void* ffi_arg_values[1] = { 0 };
    void* arg_value = nullptr;
    ALLOCA_PARAM_PTR(ftype, arg_value);

    // fill third param using guest value and accesor type
    if (!convertValueToNative(instance, ftype, ctx, val, arg_value)) {
        FEATURE_LOG_ERROR("convert value to native failed!");
        return RET_ARGS_TYPE_ERR;
    }
    ffi_arg_values[0] = arg_value;

    // invoke
    StubFunc setter_stub = accessor->setter_stub;
    FEATURE_CHECK_NE(setter_stub, nullptr);
    setter_stub(instance, accessor->data, ffi_arg_values, 1, ffi_arg_values[0]);

    if (FT_NEED_FREE(ftype)) {
        FeatureFreeValue(*(void**)arg_value);
    }
    return RET_OK;
}

template <typename TCtx, typename TTarget>
bool constGet(TCtx ctx, const MemberConst* mconst, TTarget& ret_val)
{
    FEATURE_CHECK_NE(mconst, nullptr);
    MemberConst* member_const = const_cast<MemberConst*>(mconst);
    void* data = &member_const->data;
    FeatureType ftype = member_const->type;
    FEATURE_CHECK_NE(ftype, FT_VOID);

    // process return value
    if (!convertValueToTarget(ftype, ctx, data, ret_val)) {
        FEATURE_LOG_ERROR("can not convert return value to guest!");
        value_translator::freeValue(ctx, ret_val);
        return false;
    }
    return true;
}

template <typename TInstance, typename TCtx, typename TTarget>
bool addEventCallbacks(TInstance* instance, MemberEvent* member_event,
    TCtx ctx, TTarget& target)
{
    if (!member_event) {
        FEATURE_LOG_ERROR("member_event ptr is null!");
        return false;
    }

    if (value_translator::isUndefined(ctx, target) || value_translator::isNull(ctx, target)) {
        FEATURE_LOG_WARN("js event is null or undefined!");
        return true;
    }

    if (value_translator::isFunction(ctx, target)) {
        auto cb_val = value_translator::toCallbackValue(target);
        instance->addEventCallback(member_event, cb_val);
    } else if (value_translator::isArray(ctx, target)) {
        auto asize = value_translator::arraySize(ctx, target);
        for (size_t i = 0; i < asize; i++) {
            TTarget elem_val = value_translator::arrayGet(ctx, target, i);
            if (!value_translator::isFunction(ctx, elem_val)) {
                FEATURE_LOG_ERROR("event array element must be a function!");
                value_translator::freeValue(ctx, elem_val);
                return false;
            }
            auto cb_val = value_translator::toCallbackValue(elem_val);
            instance->addEventCallback(member_event, cb_val);
            value_translator::freeValue(ctx, elem_val);
        }
    } else {
        FEATURE_LOG_ERROR("arg type mismatch, need event function or function array!");
        return false;
    }
    return true;
}

template <typename TInstance, typename TCtx, typename TTarget>
bool getEventCallbacks(TInstance* instance, MemberEvent* member_event,
    TCtx ctx, TTarget& target)
{
    if (!member_event) {
        FEATURE_LOG_ERROR("member_event ptr is null!");
        return false;
    }

    target = value_translator::newArray(ctx);
    std::list<TTarget> callbacks = instance->getEventCallbacks(member_event);
    auto size = callbacks.size();
    if (size == 0) {
        FEATURE_LOG_WARN("no callback!");
    } else if (size == 1) {
        value_translator::arraySet(ctx, target, 0,
            value_translator::dupValue(ctx, callbacks.front()));
    } else {
        int32_t i = 0;
        for (auto& callback : callbacks) {
            value_translator::arraySet(ctx, target, i,
                value_translator::dupValue(ctx, callback));
            i++;
        }
    }
    return true;
}

template <typename TInstance, typename TCtx, typename TTarget>
bool removeEventCallbacks(TInstance* instance, MemberEvent* member_event,
    TCtx ctx, TTarget& target)
{
    if (value_translator::isUndefined(ctx, target) || value_translator::isNull(ctx, target)) {
        FEATURE_LOG_DEBUG("event callback is null or undefined!");
        FtEventId eid = instance->findEventId(member_event);
        if (eid > 0) {
            // remove all old event callbacks
            instance->removeEventCallbacks(eid);
        }
    } else if (value_translator::isFunction(ctx, target)) {
        auto cb_val = value_translator::toCallbackValue(target);
        instance->eraseEventCallback(member_event, cb_val);
    } else if (value_translator::isArray(ctx, target)) {
        auto asize = value_translator::arraySize(ctx, target);
        for (size_t i = 0; i < asize; i++) {
            TTarget elem_val = value_translator::arrayGet(ctx, target, i);
            if (!value_translator::isFunction(ctx, elem_val)) {
                FEATURE_LOG_ERROR("event array element must be function!");
                value_translator::freeValue(ctx, elem_val);
                return false;
            }
            auto cb_val = value_translator::toCallbackValue(elem_val);
            instance->eraseEventCallback(member_event, cb_val);
            value_translator::freeValue(ctx, elem_val);
        }
    } else {
        FEATURE_LOG_ERROR("arg type mismatch, need event function or function array!");
        return false;
    }
    return true;
}

template <typename TInstance, typename TCtx, typename TTarget>
RetCode eventSet(TInstance* instance, TCtx ctx, const Member* member, TTarget& val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type, MEMBER_EVENT);
    MemberEvent* member_event = const_cast<MemberEvent*>(member->event);
    FtEventId eid = instance->findEventId(member_event);
    if (eid > 0) {
        // remove all old event callbacks
        instance->removeEventCallbacks(eid);
    }
    if (!addEventCallbacks(instance, member_event, ctx, val)) {
        return RET_ARGS_TYPE_ERR;
    }

    return RET_OK;
}

template <typename TInstance, typename TCtx, typename TTarget>
bool eventGet(TInstance* instance, TCtx ctx, const Member* member, TTarget& ret_val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type, MEMBER_EVENT);
    MemberEvent* member_event = const_cast<MemberEvent*>(member->event);
    return getEventCallbacks(instance, member_event, ctx, ret_val);
}

template <typename TInstance, typename TCtx, typename TTarget>
bool eventOnOff(TInstance* instance, TCtx ctx, const Member* member, TTarget& val, bool is_on)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type, MEMBER_EVENT);
    MemberEvent* member_event = const_cast<MemberEvent*>(member->event);
    if (is_on) {
        if (!addEventCallbacks(instance, member_event, ctx, val)) {
            return false;
        }
    } else {
        if (!removeEventCallbacks(instance, member_event, ctx, val)) {
            return false;
        }
    }
    return true;
}

}
#endif
