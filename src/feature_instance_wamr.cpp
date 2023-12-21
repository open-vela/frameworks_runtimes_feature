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
#include "feature_manager_wamr.h"

#include "feature_ffi_wamr.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_prototype.h"
#include "feature_utils.h"
#include "wasm_export.h"

/* import support dyntype head file */
#include "libdyntype.h"
#include "libdyntype_export.h"
#include "gc_export.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

using namespace FEATURE;

extern "C"
{
    wasm_stringref_obj_t create_wasm_string(wasm_exec_env_t exec_env, const char *str);
    wasm_stringref_obj_t create_wasm_string_with_len(wasm_exec_env_t exec_env, const char *str, uint32_t len);
    int32_t get_array_struct_type(wasm_module_t wasm_module, int32_t array_type_idx, wasm_struct_type_t *p_struct_type);
    int get_array_length(wasm_struct_obj_t obj);
}

static void dynamic_object_finalizer(wasm_anyref_obj_t obj, void *data)
{
    dyn_value_t value = (dyn_value_t)wasm_anyref_obj_get_value(obj);
    dyntype_release((dyn_ctx_t)data, value);
}

static wasm_anyref_obj_t return_box_anyref(wasm_exec_env_t exec_env, const void *ptr)
{
    do {
        wasm_anyref_obj_t any_obj =
            (wasm_anyref_obj_t)wasm_anyref_obj_new(exec_env, ptr);
        if (!any_obj) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "alloc memory failed");
            return NULL;
        }
        wasm_obj_set_gc_finalizer(
            exec_env, (wasm_obj_t)any_obj,
            (wasm_obj_finalizer_t)dynamic_object_finalizer,
            dyntype_get_context());
        return any_obj;
    } while (0);
    return nullptr;
}
namespace ferry {

FeatureInstanceWamr::FeatureInstanceWamr(FeaturePrototype* proto)
    : FeatureInstance(proto)
    , instance_qjs_(new FeatureInstanceQjs(prototype()))
{
}

FeatureInstanceWamr::FeatureInstanceWamr(FeaturePrototype* module_proto, VTable* vtable)
    : FeatureInstance(module_proto, vtable)
    , instance_qjs_(new FeatureInstanceQjs(prototype()))
{
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

FtCallbackId FeatureInstanceWamr::addCallback(uint64_t& value, CallbackType* callbackType)
{
    wasm_obj_t cb_value = *((wasm_obj_t *)(&value));
    WamrCallbackData callback;
    callback.cb = cb_value;
    callback.cb_type = callbackType;
    callbacks_[curr_cid_] = callback;
    return curr_cid_++;
}

static uint32_t get_any_array_type(wasm_module_t module, wasm_array_type_t *p_array_type_t)
{
    uint32_t i, type_count;
    bool is_mutable = true;
    type_count = wasm_get_defined_type_count(module);
    for (i = 0; i < type_count; i++) {
        wasm_defined_type_t type = wasm_get_defined_type(module, i);
        if (wasm_defined_type_is_array_type(type)) {
            bool mutable_ref = false;
            wasm_ref_type_t arr_elem_ref_type = wasm_array_type_get_elem_type(
                (wasm_array_type_t)type, &mutable_ref);

            if (arr_elem_ref_type.value_type == VALUE_TYPE_ANYREF && mutable_ref == is_mutable) {
                if (p_array_type_t) {
                    *p_array_type_t = (wasm_array_type_t)type;
                }
                return i;
            }
        }
    }
    if (p_array_type_t) {
        *p_array_type_t = nullptr;
    }

    return -1;
}

static wasm_struct_obj_t creat_any_array_obj(wasm_exec_env_t exec_env, uint32_t arr_elem_count)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    wasm_local_obj_ref_t local_ref = {0};
    wasm_array_type_t any_array_type = nullptr;
    uint32_t res_arr_type_idx = get_any_array_type(module, &any_array_type);

    wasm_struct_type_t res_arr_struct_type = nullptr;

    /* get result array struct type */
    get_array_struct_type(module, res_arr_type_idx, &res_arr_struct_type);

    wasm_struct_obj_t new_any_array_struct =
        wasm_struct_obj_new_with_type(exec_env, res_arr_struct_type);

    if (!new_any_array_struct) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "alloc memory failed");
        return nullptr;
    }

    /* Push object to local ref to avoid being freed at next allocation */
    wasm_runtime_push_local_object_ref(exec_env, &local_ref);
    local_ref.val = (wasm_obj_t)new_any_array_struct;

    wasm_value_t val = {0};
    val.gc_obj = nullptr;
    wasm_array_obj_t new_arr = wasm_array_obj_new_with_type(exec_env, any_array_type, arr_elem_count,
                                                            &val);
    if (!new_arr) {
        wasm_runtime_pop_local_object_ref(exec_env);
        wasm_runtime_set_exception(module_inst, "alloc memory failed");
        return nullptr;
    }

    val.gc_obj = (wasm_obj_t)new_arr;
    wasm_struct_obj_set_field(new_any_array_struct, 0, &val);

    wasm_runtime_pop_local_object_ref(exec_env);
    return new_any_array_struct;
}

int FeatureInstanceWamr::settlePromise(bool resolve, FtPromiseId pid, va_list& ap)
{
    return instance_qjs_->settleWamrPromise(resolve, pid, ap);
}

int FeatureInstanceWamr::invokeCallback(FtCallbackId cid, va_list &ap)
{
    const auto callback = getCallback(cid);
    bool has_rest_param = false;
    CallbackType *callbackType = callback.cb_type;
    int method_param_count = getParamCount(callbackType->parameters, &has_rest_param);
    if (has_rest_param) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with InvokeFeatureCallbackCount!");
        return -1;
    }

    return doInvokeCallback(callbackType, callback, ap, method_param_count, 0);
}

int FeatureInstanceWamr::invokeCallbackCount(FtCallbackId cid, va_list &ap, int count)
{
    const auto callback = getCallback(cid);
    bool has_rest_param = false;
    CallbackType *callbackType = callback.cb_type;
    int method_param_count = getParamCount(callbackType->parameters, &has_rest_param); {
        FEATURE_LOG_ERROR("no resut parameter callback must invoke with FeatureInvokeCallback!");
        return -1;
    }
    return doInvokeCallback(callbackType, callback, ap, method_param_count, count - method_param_count);
}

int FeatureInstanceWamr::doInvokeCallback(const CallbackType *callbackType, WamrCallbackData callback, va_list &ap, int method_param_count, int rest_param_count)
{
    auto manager = (FeatureManagerWamr*)(prototype()->getFeatureManager());
    wasm_exec_env_t exec_env = (wasm_exec_env_t)(manager->wamrEnv());
    wasm_value_t context = { 0 }, thiz = { 0 }, func_obj = { 0 };

    if (callback.cb == nullptr) {
        FEATURE_LOG_ERROR("callback in undefined !");
        return -1;
    }

    /* get closure context and func ref */
    wasm_struct_obj_get_field((WASMStructObjectRef)callback.cb, 0, false, &context);
    wasm_struct_obj_get_field((WASMStructObjectRef)callback.cb, 1, false, &thiz);
    wasm_struct_obj_get_field((WASMStructObjectRef)callback.cb, 2, false, &func_obj);

    uint32 argv[64];
    uint32 occupied_slots = 0;
    /* arg0: context */
    bh_memcpy_s(argv, sizeof(argv), &context.gc_obj, sizeof(void *));
    occupied_slots += sizeof(void *) / sizeof(uint32);

    /* arg1: thiz */
    bh_memcpy_s(argv + occupied_slots,
                sizeof(argv) - occupied_slots * sizeof(uint32), &thiz.gc_obj,
                sizeof(void *));
    occupied_slots += sizeof(void *) / sizeof(uint32);

    do {
        /* convert parameters to feature_value_t */
        for (int i = 0; i < method_param_count; i++) {
            FeatureType featureType = callbackType->parameters[i];
            void *ptr = exactVariadicParameter(ap, featureType);
            if (!ptr) {
                // got_error = true;
                break;
            }

            uint64_t wasm_ret;
            if (!FeatureFFIWamr::convertValueToGuest(this, featureType, ptr, exec_env, wasm_ret)) {
                FEATURE_LOG_ERROR("convert callback param failed !");
                free(ptr);
                break;
            }
            if (FT_IS_PRIMITIVE(featureType)) {
                switch (FT_GET_VALUE(featureType)) {
                    case FT_VOID: {
                        FEATURE_LOG_ERROR("void feature type not supported !");
                    } break;
                    case FT_INT:
                    case FT_INT32:
                    case FT_UINT32:
                    case FT_DOUBLE: {
                        *(double *)(argv + occupied_slots) = (double)wasm_ret;
                        occupied_slots += sizeof(double) / sizeof(uint32);
                    } break;
                    case FT_CHAR: {
                        wasm_stringref_obj_t obj = (wasm_stringref_obj_t)wasm_ret;
                        b_memcpy_s(argv + occupied_slots, sizeof(argv) - occupied_slots, &(obj),
                                    sizeof(wasm_stringref_obj_t));
                        occupied_slots += sizeof(wasm_stringref_obj_t) / sizeof(uint32);
                        break;
                    }
                    default:{
                        FEATURE_LOG_ERROR("feature type not supported !");
                    } break;
                }
	    }
            free(ptr);
        }

        /* Call the creat_any_array_obj api to create an array object with element type any 
        *  rest_param_count： number of elements.
        */
        wasm_struct_obj_t obj_ref = creat_any_array_obj(exec_env, rest_param_count);
        wasm_value_t wasm_array = {0}, len_val = {0};

        /*  Take out the array data field of the array object,
            *  then wrap and assign any type to each element of the array.
            */
        wasm_struct_obj_get_field(obj_ref, 0, false, &wasm_array);
        wasm_array_obj_t any_array = (wasm_array_obj_t)wasm_array.gc_obj;

        dyn_ctx_t dyn_ctx;
        dyn_ctx = dyntype_get_context();
        /* Set the field of the array length */
        len_val.i32 = (int32_t)rest_param_count;
        wasm_struct_obj_set_field(obj_ref, 1, &len_val);
        // printf("[doInvokeCallback] have variable parameter %d\n", rest_param_count);
        /* Unify the variable parameters into "any" and add each element to the any array object */
        for (int i = 0; i < rest_param_count; i++) {
            void *arg = va_arg(ap, void *);
            void *header_ptr = ((char *)arg - FT_OBJ_HEADER_SIZE);
            FTObjHeader *header = (FTObjHeader *)header_ptr;
            uint64_t wasm_ret;
            if (!FeatureFFIWamr::convertValueToGuest(this, header->featureType, arg, exec_env, wasm_ret)) {
                FEATURE_LOG_ERROR("convert callback param failed !");
                break;
            }
            if (FT_IS_PRIMITIVE(header->featureType)) {
                switch (FT_GET_VALUE(header->featureType)) {
                    case FT_VOID: {
                        FEATURE_LOG_ERROR("void feature type not supported !");
                    } break;
                    case FT_INT:
                    case FT_INT32:
                    case FT_UINT32: {
                        wasm_value_t tmp_val = { 0 };
                        /* call return_box_anyref api to box element as any */
                        wasm_anyref_obj_t any_obj = nullptr;
                        int32_t i32_ret = (int32_t)wasm_ret;
                        if (i32_ret == 1 || i32_ret == 0) {
                            any_obj = return_box_anyref(exec_env, dyntype_new_boolean(dyn_ctx, i32_ret));
                        } else {
                            any_obj = return_box_anyref(exec_env, dyntype_new_number(dyn_ctx, i32_ret));
                        }
                        // wasm_anyref_obj_t any_obj = return_box_anyref(exec_env, dyntype_new_number(dyn_ctx, val.of.i32));
                        tmp_val.gc_obj = (wasm_obj_t)any_obj;
                        wasm_array_obj_set_elem(any_array, i, &tmp_val);
                    } break;
                    case FT_DOUBLE: {
                        wasm_value_t tmp_val = { 0 };
                        /* call return_box_anyref api to box element as any */
                        wasm_anyref_obj_t any_obj = return_box_anyref(exec_env, dyntype_new_number(dyn_ctx, (double)wasm_ret));
                        tmp_val.gc_obj = (wasm_obj_t)any_obj;
                        wasm_array_obj_set_elem(any_array, i, &tmp_val);
                    } break;
                    case FT_CHAR: {
                        wasm_stringref_obj_t obj = (wasm_stringref_obj_t)wasm_ret;
                        wasm_value_t tmp_val = {0};
                        /* call return_box_anyref api to box element as any */
                        wasm_anyref_obj_t any_obj = return_box_anyref(exec_env, dyntype_new_string(dyn_ctx, (void *)wasm_stringref_obj_get_value(obj)));
                        tmp_val.gc_obj = (wasm_obj_t)any_obj;
                        wasm_array_obj_set_elem(any_array, i, &tmp_val);
                    } break;
                    default:{
                        FEATURE_LOG_ERROR("feature type not supported !");
                    } break;
                }
	    }
            /* at least, add the any array object to the return parameter argv */
            b_memcpy_s(argv + occupied_slots, sizeof(argv) - occupied_slots, &(obj_ref),
                        sizeof(wasm_struct_obj_t));
            occupied_slots += sizeof(wasm_struct_obj_t) / sizeof(uint32);
        }
        wasm_runtime_call_func_ref(exec_env, (wasm_func_obj_t)func_obj.gc_obj,
                                        occupied_slots, argv);
    } while (0);

    return 0;
}

FtPromiseId FeatureInstanceWamr::addPromise(FeatureType resolve_type, FeatureType reject_type)
{
    return instance_qjs_->addWamrPromise(resolve_type, reject_type);
}

feature_value_t FeatureInstanceWamr::getPromise(FtPromiseId pid)
{
    return instance_qjs_->getPromise(pid);
}

void FeatureInstanceWamr::release()
{
    callbacks_.clear();
}

int FeatureInstanceWamr::getSameCallback(FtCallbackId cid)
{
    return -1; // to do
}

}
