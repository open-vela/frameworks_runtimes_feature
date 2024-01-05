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
#include "feature_wamr_utils.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

static void fillArg(char* argp, uint32 args, FeatureType& ftype, uint64_t target, uint32& filled)
{
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (FT_GET_VALUE(ftype)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void feature type not supported !");
            } break;
            case FT_INT:
            case FT_INT32:
            case FT_UINT32:
            case FT_DOUBLE: {
                *(double *)(argp + filled) = (double)target;
                filled += sizeof(double);
            } break;
            case FT_CHAR: {
                wasm_stringref_obj_t obj = (wasm_stringref_obj_t)target;
                b_memcpy_s(argp + filled, args - filled, &(obj),
                        sizeof(wasm_stringref_obj_t));
                filled += sizeof(wasm_stringref_obj_t);
            } break;
            default:{
                FEATURE_LOG_ERROR("feature type not supported !");
            } break;
        }
    }
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
    return eraseCallback(cid);
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

static wasm_struct_obj_t create_any_array_obj(wasm_exec_env_t exec_env, uint32_t arr_elem_count)
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


int FeatureInstanceWamr::invokeCallback(FtCallbackId cid, va_list& ap)
{
    auto cb_data = getCallbackData(cid);
    if (!cb_data) {
        FEATURE_LOG_ERROR("callback is undefined !");
        return -1;
    }
    bool has_rest_param = false;
    CallbackType* cb_type = cb_data->type;
    int fixed_argc = getParamCount(cb_type->parameters, &has_rest_param);
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
    CallbackType* cb_type = cb_data->type;
    int fixed_argc = getParamCount(cb_type->parameters, &has_rest_param);
    if (!has_rest_param || count < fixed_argc) {
        FEATURE_LOG_ERROR("resut parameter callback must invoke with FeatureInvokeCallbackCount!");
        return -1;
    }

    return callCallback(cb_data, ap, fixed_argc, count - fixed_argc);
}

wasm_exec_env_t FeatureInstanceWamr::getContext()
{
    auto manager = (FeatureManagerWamr*)(prototype()->featureManager());
    return (wasm_exec_env_t)(manager->wamrEnv());
}

bool FeatureInstanceWamr::argToTarget(va_list &ap, FeatureType ftype, uint64_t& target)
{
    wasm_exec_env_t env = getContext();
    void *param = extractVariadicParam(ap, ftype);
    if (!param) {
        FEATURE_LOG_ERROR("extract callback param failed !");
        return false;
    }
    if (!FeatureFFIWamr::convertValueToGuest(this, ftype, param, env, target)) {
        FEATURE_LOG_ERROR("convert callback param failed !");
        free(param);
        return false;
    }
    free(param);
    return true;
}

bool FeatureInstanceWamr::variArgToTarget(void *arg, wasm_value_t& target)
{
    wasm_exec_env_t env = getContext();
    dyn_ctx_t dyn_ctx = dyntype_get_context();
    FTObjHeader *header = (FTObjHeader *)((char *)arg - FT_OBJ_HEADER_SIZE);
    uint64_t guest;
    if (!FeatureFFIWamr::convertValueToGuest(this, header->featureType, arg, env, guest)) {
        FEATURE_LOG_ERROR("convert callback param failed !");
        return false;
    }

    wasm_anyref_obj_t any = nullptr;
    if (FT_IS_PRIMITIVE(header->featureType)) {
        switch (FT_GET_VALUE(header->featureType)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void feature type not supported !");
                return false;
            }
            case FT_INT:
            case FT_INT32:
            case FT_UINT32: {
                /* call return_box_anyref api to box element as any */
                int32_t iguest = (int32_t)guest;
                if (iguest == 1 || iguest == 0)
                    any = return_box_anyref(env, dyntype_new_boolean(dyn_ctx, iguest));
                else
                    any = return_box_anyref(env, dyntype_new_number(dyn_ctx, iguest));
                target.gc_obj = (wasm_obj_t)any;
            } break;
            case FT_DOUBLE: {
                /* call return_box_anyref api to box element as any */
                any = return_box_anyref(env, dyntype_new_number(dyn_ctx, (double)guest));
                target.gc_obj = (wasm_obj_t)any;
            } break;
            case FT_CHAR: {
                wasm_stringref_obj_t obj = (wasm_stringref_obj_t)guest;
                /* call return_box_anyref api to box element as any */
                any = return_box_anyref(env,
                        dyntype_new_string(dyn_ctx, (void *)wasm_stringref_obj_get_value(obj)));
                target.gc_obj = (wasm_obj_t)any;
            } break;
            default:{
                FEATURE_LOG_ERROR("feature type not supported !");
                return false;
            }
        }
    }
    return true;
}

int FeatureInstanceWamr::doInvokeCallback(const CallbackType *cb_type, wasm_obj_t callback, va_list &ap, int fixed_argc, int rest_argc)
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
    bh_memcpy_s(argp, args, &context.gc_obj, sizeof(void *));
    filled = sizeof(void *);
    /* arg1: thiz */
    bh_memcpy_s(argp + filled, args -filled, &thiz.gc_obj, sizeof(void *));
    filled += sizeof(void *);

    /* convert parameters to feature_value_t */
    for (int i = 0; i < fixed_argc; i++) {
        FeatureType ftype = cb_type->parameters[i];
        uint64_t target = 0;
        if (!argToTarget(ap, ftype, target)) {
            FEATURE_LOG_ERROR("extract callback param failed !");
            return -1;
        }
        fillArg(argp, args, ftype, target, filled);
    }

    // create an array object with element type any rest_argc number of elements.
    wasm_value_t array_size = { .i32 = (int32_t)rest_argc };
    wasm_struct_obj_t array_struct = create_any_array_obj(env, rest_argc);
    wasm_struct_obj_set_field(array_struct, 1, &array_size);

    /*  Take out the array data field of the array object,
    *  then wrap and assign any type to each element of the array.
    */
    wasm_value_t array = { 0 };
    wasm_struct_obj_get_field(array_struct, 0, false, &array);
    wasm_array_obj_t array_obj = (wasm_array_obj_t)array.gc_obj;

    /* Unify the variable parameters into "any" and add each element to the any array object */
    for (int i = 0; i < rest_argc; i++) {
        void *arg = va_arg(ap, void *);
        wasm_value_t target = { 0 };
        if (!variArgToTarget(arg, target)) {
            FEATURE_LOG_ERROR("convert vari params failed !");
            return -1;
        }
        wasm_array_obj_set_elem(array_obj, i, &target);
        /* at least, add the any array object to the return parameter argv */
        b_memcpy_s(argp + filled, args -filled, &(array_struct), sizeof(wasm_struct_obj_t));
        filled += sizeof(wasm_struct_obj_t);
    }
    filled = filled /(sizeof(uint32) /sizeof(char));
    wasm_runtime_call_func_ref(env, (wasm_func_obj_t)func_obj.gc_obj, filled, argv);

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
    clearCallbacks();
}

int FeatureInstanceWamr::getSameCallback(FtCallbackId cid)
{
    return getInitialCallbackId(cid); // to do
}

}
