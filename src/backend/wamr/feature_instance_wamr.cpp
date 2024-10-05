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
#include "feature_instance_wamr.h"
#include "feature_ffi.h"
#include "feature_log.h"
#include "feature_manager_wamr.h"
#include "feature_prototype.h"
#include "feature_utils.h"
#include "feature_wamr_utils.h"
// clang-format off
#include "value_translator_wamr.h"
#include "feature_convertor_templates.h"
// clang-format on

#include <cstdarg>
#include <cstdint>
#include <string.h>

static void fillArg(char* argp, uint32 args, FeatureType& ftype, uint64_t target, uint32& filled)
{
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (ftype) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void feature type not supported !");
        } break;
        case FT_INT:
        case FT_INT32:
        case FT_UINT32:
        case FT_DOUBLE: {
            *(double*)(argp + filled) = (double)target;
            filled += sizeof(double);
        } break;
        case FT_STRING: {
            wasm_stringref_obj_t obj = (wasm_stringref_obj_t)target;
            b_memcpy_s(argp + filled, args - filled, &(obj),
                sizeof(wasm_stringref_obj_t));
            filled += sizeof(wasm_stringref_obj_t);
        } break;
        default: {
            FEATURE_LOG_ERROR("feature type not supported !");
        } break;
        }
    }
}

namespace feature_framework {

FeatureInstanceWamr::FeatureInstanceWamr(FeaturePrototype* proto)
    : FeatureInstance(proto, proto->description())
    , PromiseManager((JSContext*)ft_context_get_data(proto->featureManager()->getFeatureContext()))
{
}

FeatureInstanceWamr::FeatureInstanceWamr(FeaturePrototype* module_proto, VTable* vtable)
    : FeatureInstance(module_proto, vtable, module_proto->description())
    , PromiseManager((JSContext*)ft_context_get_data(module_proto->featureManager()->getFeatureContext()))
{
}

FeatureInstanceWamr::~FeatureInstanceWamr()
{
    auto proto = prototype();
    wasm_exec_env_t env = getContext();
    // invoke callback
    if (proto->description()->native_callbacks && proto->description()->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto->description()->native_callbacks->onDetached(env, this);
    }
    release();
}

bool FeatureInstanceWamr::removeCallback(FtCallbackId cid)
{
    return eraseCallback(cid);
}

int FeatureInstanceWamr::resolvePromise(FtPromiseId pid, va_list& ap)
{
    int ret = doResolvePromise(pid, ap);
    if (!removePromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }
    return ret;
}

int FeatureInstanceWamr::rejectPromise(FtPromiseId pid, int code, const char* msg)
{
    int ret = doRejectPromise(pid, code, msg);
    if (!removePromise(pid)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", pid);
        ret = -2;
    }
    return ret;
}

int FeatureInstanceWamr::invokeCallback(FtCallbackId cid, va_list& ap)
{
    auto cb_data = getCallbackData(cid);
    if (!cb_data) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }
    bool has_rest_param = false;
    int32_t int32_count = 0;
    const CallbackType* cb_type = cb_data->type;
    int fixed_argc = getParamCount(cb_type->parameters, &has_rest_param, nullptr, &int32_count);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return callCallback(cb_data, ap, fixed_argc, 0);
}

int FeatureInstanceWamr::invokeCallbackCount(FtCallbackId cid, va_list& ap, int count)
{
    auto cb_data = getCallbackData(cid);
    if (!cb_data) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }
    bool has_rest_param = false;
    int32_t int32_count = 0;
    const CallbackType* cb_type = cb_data->type;
    int fixed_argc = getParamCount(cb_type->parameters, &has_rest_param, nullptr, &int32_count);
    if (!has_rest_param || count < fixed_argc) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return callCallback(cb_data, ap, fixed_argc, count - fixed_argc);
}

bool FeatureInstanceWamr::emitEvent(FtEventId eid, va_list& ap)
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

void FeatureInstanceWamr::setEventChangeListener(FeatureEventChangeListener listener)
{
    doSetEventChangeListener(listener, this);
}

FtEventId FeatureInstanceWamr::getEventId(const char* name)
{
    return doGetEventId(name);
}

const char* FeatureInstanceWamr::getEventName(FtEventId eid)
{
    return doGetEventName(eid);
}

int FeatureInstanceWamr::getEventCallbackCount(FtEventId eid)
{
    return doGetEventCallbackCount(eid);
}

wasm_exec_env_t FeatureInstanceWamr::getContext()
{
    auto manager = (FeatureManagerWamr*)(prototype()->featureManager());
    return (wasm_exec_env_t)(manager->wamrEnv());
}

bool FeatureInstanceWamr::argToTarget(va_list& ap, FeatureType ftype, uint64_t& target)
{
    wasm_exec_env_t env = getContext();
    void* param = extractVariadicParam(ap, ftype);
    if (!param) {
        FEATURE_LOG_ERROR("extract callback param failed !");
        return false;
    }
    if (!convertValueToTarget(ftype, env, param, target)) {
        FEATURE_LOG_ERROR("convert callback param failed !");
        free(param);
        return false;
    }
    free(param);
    return true;
}

bool FeatureInstanceWamr::variArgToTarget(void* arg, wasm_value_t& target)
{
    wasm_exec_env_t env = getContext();
    dyn_ctx_t dyn_ctx = dyntype_get_context();
    FTObjHeader* header = (FTObjHeader*)((char*)arg - FT_OBJ_HEADER_SIZE);
    FeatureType ftype = *(FeatureType*)((uintptr_t)header - sizeof(FeatureType));
    uint64_t guest;
    if (!convertValueToTarget(ftype, env, FT_IS_REFERENCE(ftype) ? &arg : arg, guest)) {
        FEATURE_LOG_ERROR("convert callback param failed !");
        return false;
    }

    wasm_anyref_obj_t any = nullptr;
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (ftype) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void feature type not supported !");
            return false;
        }
        case FT_INT:
        case FT_INT32:
        case FT_UINT32: {
            /* call create_anyref_obj api to box element as any */
            int32_t iguest = (int32_t)guest;
            if (iguest == 1 || iguest == 0)
                any = create_anyref_obj(env, dyntype_new_boolean(dyn_ctx, iguest));
            else
                any = create_anyref_obj(env, dyntype_new_number(dyn_ctx, iguest));
            target.gc_obj = (wasm_obj_t)any;
        } break;
        case FT_DOUBLE: {
            /* call create_anyref_obj api to box element as any */
            any = create_anyref_obj(env, dyntype_new_number(dyn_ctx, (double)guest));
            target.gc_obj = (wasm_obj_t)any;
        } break;
        case FT_STRING: {
            wasm_stringref_obj_t obj = (wasm_stringref_obj_t)guest;
            /* call create_anyref_obj api to box element as any */
            any = create_anyref_obj(env,
                dyntype_new_string(dyn_ctx, (void*)wasm_stringref_obj_get_value(obj)));
            target.gc_obj = (wasm_obj_t)any;
        } break;
        default: {
            FEATURE_LOG_ERROR("feature type not supported !");
            return false;
        }
        }
    }
    return true;
}

int FeatureInstanceWamr::doInvokeCallback(const FeatureType* param_types, wasm_obj_t callback, va_list& ap, int fixed_argc, int rest_argc)
{
    if (callback == nullptr) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }

    wasm_exec_env_t env = getContext();
    wasm_value_t context = { 0 }, thiz = { 0 }, func_obj = { 0 };
    /* get closure context and func ref */
    wasm_struct_obj_get_field((WASMStructObjectRef)callback, 0, false, &context);
    wasm_struct_obj_get_field((WASMStructObjectRef)callback, 1, false, &thiz);
    wasm_struct_obj_get_field((WASMStructObjectRef)callback, 2, false, &func_obj);

    uint32 argv[64];
    uint32 args = sizeof(argv);
    char* argp = (char*)argv;
    uint32 filled = 0;
    /* arg0: context */
    bh_memcpy_s(argp, args, &context.gc_obj, sizeof(void*));
    filled = sizeof(void*);
    /* arg1: thiz */
    bh_memcpy_s(argp + filled, args - filled, &thiz.gc_obj, sizeof(void*));
    filled += sizeof(void*);

    wasm_local_obj_ref_t* obj_ref_head = wasm_runtime_get_cur_local_obj_ref(env);
    /* convert parameters to feature_value_t */
    for (int i = 0; i < fixed_argc; i++) {
        FeatureType ftype = param_types[i];
        uint64_t target = 0;
        if (!argToTarget(ap, ftype, target)) {
            FEATURE_LOG_ERROR("extract callback param failed !");
            return -1;
        }
        fillArg(argp, args, ftype, target, filled);
    }

    /* create an array object with element type any rest_argc number of elements if the callback func have rest_argc. */
    if (rest_argc > 0) {
        wasm_struct_obj_t array_struct = create_array_with_type(env, rest_argc, VALUE_TYPE_ANYREF);
        /*  Take out the array data field of the array object,
         *  then wrap and assign any type to each element of the array.
         */
        wasm_value_t array = { 0 };
        wasm_struct_obj_get_field(array_struct, 0, false, &array);
        wasm_array_obj_t array_obj = (wasm_array_obj_t)array.gc_obj;

        /* Unify the variable parameters into "any" and add each element to the any array object */
        for (int i = 0; i < rest_argc; i++) {
            void* arg = va_arg(ap, void*);
            wasm_value_t target = { 0 };
            if (!variArgToTarget(arg, target)) {
                FEATURE_LOG_ERROR("convert vari params failed !");
                return -1;
            }
            wasm_array_obj_set_elem(array_obj, i, &target);
        }
        /* at last, add the any array object to the return parameter argv */
        b_memcpy_s(argp + filled, args - filled, &(array_struct), sizeof(wasm_struct_obj_t));
        filled += sizeof(wasm_struct_obj_t);
    }

    // convert filled form bytes to wamr slots
    filled = filled / (sizeof(uint32) / sizeof(char));
    wasm_runtime_call_func_ref(env, (wasm_func_obj_t)func_obj.gc_obj, filled, argv);

    /* pop native createD obj local ref ptr */
    pop_local_obj_ref_to_head(env, obj_ref_head);
    return 0;
}

void FeatureInstanceWamr::release()
{
    clearCallbacks();
    // release all promises
    releasePromises();
}

}
