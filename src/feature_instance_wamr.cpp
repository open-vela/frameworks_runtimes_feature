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

#include "feature_framework.h"
#include "feature_ffi_wamr.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "wasm_export.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

using namespace FEATURE;

extern "C" wasm_struct_obj_t create_wasm_string(wasm_exec_env_t exec_env, const char *value);
namespace ferry {

FeatureInstanceWamr::FeatureInstanceWamr(FeaturePrototype* proto, VTable* vtable)
    : FeatureInstance(proto, vtable)
    , instance_qjs_(new FeatureInstanceQjs(proto, nullptr))
{
}

FeatureInstance* FeatureInstanceWamr::createInterface(VTable* vtable)
{
    // null param proto to be fixed
    return new FeatureInstanceWamr(nullptr, vtable);
}

FeatureInstanceWamr::~FeatureInstanceWamr()
{
}

bool FeatureInstanceWamr::removeCallback(FtCallbackId cid)
{
    if (!callbacks_.count(cid)) {
        FEATURE_LOG_ERROR("callback_wamr id %d in instance: %p not exist !", cid, this);
        return false;
    }
    callbacks_.erase(cid);
    return true;
}

int FeatureInstanceWamr::invokeCallback(FtCallbackId cid, va_list& ap) {
    const auto callback = getCallback(cid);
    bool has_rest_param = false;
    CallbackType* callbackType = callback.cb_type;
    int method_param_count = getParamCount(callbackType->parameters, &has_rest_param);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with InvokeFeatureCallbackCount!");
        return -1;
    }

    wasm_exec_env_t exec_env = (wasm_exec_env_t)(prototype()->wamr_env);
    wasm_value_t context = { 0 }, func_obj = { 0 };

    if (callback.cb == NULL) {
        FEATURE_LOG_ERROR("callback in undefined !");
        return -1;
    }

    /* get closure context and func ref */
    wasm_struct_obj_get_field((WASMStructObjectRef)callback.cb, 0, false, &context);
    wasm_struct_obj_get_field((WASMStructObjectRef)callback.cb, 1, false, &func_obj);

    uint32 argv[64];
    uint32 occupied_slots = 0;
    bh_memcpy_s(argv, sizeof(argv), &context.gc_obj, sizeof(void *));
    occupied_slots += sizeof(void *) / sizeof(uint32);

    do {
        // convert parameters to feature_value_t
        for (int i = 0; i < method_param_count; i++) {
            FeatureType featureType = callbackType->parameters[i];
            void* ptr = exactVariadicParameter(ap, featureType);
            if (!ptr) {
                //got_error = true;
                break;
            }
            wasm_val_t val;
            if (!FeatureFFIWamr::convertValueToGuest(this, featureType, ptr, exec_env, val)) {
                FEATURE_LOG_ERROR("convert callback param failed !");
                free(ptr);
                break;
            }
            switch (val.kind)
            {
            case WASM_I32:
            {
                *(double *)(argv + occupied_slots) = val.of.i32;
                occupied_slots += sizeof(double) / sizeof(uint32);
            }
            break;
            case WASM_F64:
            {
                *(double *)(argv + occupied_slots) = val.of.f64;
                occupied_slots += sizeof(double) / sizeof(uint32);
            }
            break;
            case WASM_ANYREF:
            {
                const char *str = (char *)val.of.foreign;
                wasm_struct_obj_t obj = create_wasm_string(exec_env, str);
                b_memcpy_s(argv + occupied_slots, sizeof(argv) - occupied_slots, &(obj),
                           sizeof(wasm_struct_obj_t));
                occupied_slots += sizeof(wasm_struct_obj_t) / sizeof(uint32);
                }
                    break;
                default:
                    break;
                }
            free(ptr);
        }
        bool ret = wasm_runtime_call_func_ref(exec_env, (wasm_func_obj_t)func_obj.gc_obj,
                                   occupied_slots, argv);
        printf("call back ret:%d\n",ret);
    } while (0);

    return 0;
}

int FeatureInstanceWamr::invokeCallbackCount(FtCallbackId cid, va_list& ap, int count) {
    // to be fixed
    return 0;
}

WamrCallbackData FeatureInstanceWamr::getCallback(FtCallbackId cid)
{
    if (!callbacks_.count(cid)) {
        WamrCallbackData callback;
        callback.cb = nullptr;
        callback.cb_type = nullptr;
        return callback;
    }
    return callbacks_[cid];
}

FtCallbackId FeatureInstanceWamr::addCallback(wasm_obj_t value, CallbackType* callbackType)
{
    //auto ctx = proto->ctx;
    WamrCallbackData callback;
    callback.cb = value;
    callback.cb_type = callbackType;
    callbacks_[curr_cid_] = callback;
    return curr_cid_++;
}

int FeatureInstanceWamr::settlePromise(bool resolve, FtPromiseId pid, va_list& ap)
{
    return instance_qjs_->settlePromise(resolve, pid, ap);
}

FtPromiseId FeatureInstanceWamr::addPromise(FeatureType resolve_type, FeatureType reject_type)
{
    return instance_qjs_->addPromise(resolve_type, reject_type);
}

feature_value_t FeatureInstanceWamr::getPromise(FtPromiseId pid)
{
    return instance_qjs_->getPromise(pid);
}

void FeatureInstanceWamr::addPromise_wamr(feature_value_t data)
{
    promises_wamr.push_back(data);
}

void FeatureInstanceWamr::release() {
    callbacks_.clear();
}

}
