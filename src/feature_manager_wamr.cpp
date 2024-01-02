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

#include "feature_manager_wamr.h"
#include "feature.h"
#include "feature_context_wamr.h"
#include "feature_context_qjs.h"
#include "feature_exports.h"
#include "value_translator_wamr.h"
#include "feature_ffi_template.h"
#include "feature_ffi_wamr.h"
#include "feature_instance_wamr.h"
#include "feature_prototype_wamr.h"
#include "feature_log.h"
#include "feature_registry.h"
#include "feature_utils.h"
#include "feature_wamr_utils.h"

#include "libdyntype_export.h"

#include <assert.h>
#include <ffi.h>
#include <memory>
#include <string>

#define FEATURE_ENV_NAME "wamr"

typedef struct DynTypeContext {
    JSRuntime *js_rt;
    JSContext *js_ctx;
    JSValue *js_undefined;
    JSValue *js_null;
    dyntype_callback_dispatcher_t cb_dispatcher;
    JSClassID extref_class_id;
    JSValue *extref_class;
} DynTypeContext;

using namespace FEATURE;

namespace ferry {

extern "C" feature_value_t*
dynamic_dup_value(feature_context_ref ctx, feature_value_t value);
/* wasm runtime lib */
extern "C" uint32_t
get_libdyntype_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
extern "C" uint32_t
get_lib_console_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
extern "C" uint32_t
get_lib_array_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
extern "C" uint32_t
get_lib_timer_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
extern "C" uint32_t
get_struct_indirect_symbols(char **p_module_name, NativeSymbol **p_native_symbols);

static inline FeatureManagerWamr* manager_from_instance(FeatureInstance *instance)
{
    return static_cast<FeatureManagerWamr*>(instance->prototype()->featureManager());
}

static inline FeatureInstance* instance_from_target(wasm_obj_t target)
{
    wasm_value_t val = { 0 };
    // every instance class have the first field to hold native instance
    wasm_struct_obj_get_field((wasm_struct_obj_t)target, 1, false, &val);
    return (FeatureInstance *)(val.gc_obj);
}

static inline void set_instance_to_target(wasm_obj_t target, FeatureInstance* instance)
{
    wasm_value_t val = { 0 };
    val.gc_obj = (wasm_obj_t)instance;
    // every instance class have the first field to hold native instance
    wasm_struct_obj_set_field((wasm_struct_obj_t)target, 1, &val);
}

static void module_object_finalizer(wasm_obj_t obj, void *data)
{
    auto instance = (FeatureInstanceWamr*)instance_from_target(obj);
    printf("module object finalizer:%p, featureinstance:%p\n", obj, instance);
    instance->release();
}

static void init_native(wasm_exec_env_t exec_env, uint64_t *args){
    native_raw_get_arg(void*, thiz_ptr, args);
    native_raw_get_arg(void *, str, args);

    wasm_stringref_obj_t str_ref = (wasm_stringref_obj_t)str;

    /* get cstring from wasm string (stringref args) */
    uint32_t str_len = 0;
    if (wasm_obj_is_stringref_obj((wasm_obj_t)str)) {
        str_len = wasm_string_get_length(str_ref);
    }
    char *buffer = str_len > 0 ? (char *)malloc(str_len + 1) : nullptr;
    if (buffer != nullptr) {
        wasm_string_to_cstring(str_ref, buffer, str_len + 1);
    }

    FEATURE_LOG_INFO("class name: %s", buffer);
    auto manager = (FeatureManagerWamr*)wasm_runtime_get_function_attachment(exec_env);
    manager->require(exec_env, (wasm_obj_t)thiz_ptr, buffer);

    // set object destructor func
    wasm_obj_set_gc_finalizer(exec_env, (wasm_obj_t)thiz_ptr,(wasm_obj_finalizer_t)module_object_finalizer, nullptr);
}

static void accessor_get(wasm_exec_env_t exec_env, uint64_t *args)
{
    uint64_t* wasm_ret_p = args;
    uint64_t wasm_ret;
    native_raw_get_arg(void *, thiz_ptr, args); // pop this pointer
    auto instance = instance_from_target((wasm_obj_t)thiz_ptr);
    auto member = (const Member*)wasm_runtime_get_function_attachment(exec_env);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    auto& accessor = member->accessor;
    auto description = instance->prototype()->description();
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    NativeFunc callback = description->dynamic ? instance->getVirtualFunction(accessor.getter.vtable_idx) : accessor.getter.callback;
    FEATURE_CHECK_NE(callback, nullptr);

    ffi_type *ffi_arg_types[2] = {&ffi_type_pointer, &ffi_type_sint64};
    ffi_type *ffi_ret = nullptr;
    void *ffi_arg_values[2] = {&instance, (void*)(&accessor.data)};
    void *ret_value = nullptr;
    do {
        if (!createTypeDeclaration(accessor.type, ffi_ret)) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        if (!createHostValue(accessor.type, ret_value, true)) {
            FEATURE_LOG_ERROR("create return value failed !");
            break;
        }

        // prepare and call method
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, ffi_ret, ffi_arg_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            break;
        }
        // invoke
        ffi_call(&cif, callback, ret_value, ffi_arg_values);
        // process return value
        if (!FeatureFFIWamr::convertValueToGuest(instance, accessor.type, ret_value, exec_env, wasm_ret)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
        }
    } while (0);

    // free resources
    freeTypeDeclaration(ffi_ret);
    FeatureFreeValue(ret_value);
    *wasm_ret_p = wasm_ret;
}

static void accessor_set(wasm_exec_env_t exec_env, uint64_t *args)
{
    native_raw_get_arg(void *, thiz_ptr, args); // pop this pointer
    auto instance = instance_from_target((wasm_obj_t)thiz_ptr);
    auto member = (const Member*)wasm_runtime_get_function_attachment(exec_env);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    auto& accessor = member->accessor;
    auto description = instance->prototype()->description();

    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type *ffi_arg_types[3] = {&ffi_type_pointer, &ffi_type_sint64, nullptr};
    void *arg_value_input = nullptr;
    void *ffi_arg_values[3] = {&instance, (void*)(&accessor.data), nullptr};

    NativeFunc callback = description->dynamic ? instance->getVirtualFunction(accessor.setter.vtable_idx) : accessor.setter.callback;
    FEATURE_CHECK_NE(callback, nullptr);

    do {
        // prepare third param type declaration, create by accessor type
        if (!createTypeDeclaration(accessor.type, ffi_arg_types[2])) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        // fill third param using guest value and accesor type
        if (!FeatureFFIWamr::convertValueToHost(instance, accessor.type, arg_value_input, exec_env, *args)) {
            FEATURE_LOG_ERROR("convert to host value failed !");
            break;
        }
        ffi_arg_values[2] = arg_value_input;
        // prepare and call method
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 3, &ffi_type_void, ffi_arg_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            break;
        }
        // invoke
        ffi_call(&cif, callback, ffi_arg_values[2], ffi_arg_values);
    } while (0);

    // free resources
    freeTypeDeclaration(ffi_arg_types[2]);
    FeatureFreeValue(arg_value_input);
}

static void const_get(wasm_exec_env_t exec_env, uint64_t *args)
{
    uint64_t *wasm_ret_p = args;
    uint64_t wasm_ret;
    native_raw_get_arg(void *, thiz_ptr, args); // pop this pointer
    FeatureInstance *instance = instance_from_target((wasm_obj_t)thiz_ptr);
    const Member* member = (const Member*)wasm_runtime_get_function_attachment(exec_env);
    FEATURE_CHECK_EQ(member->type, MEMBER_CONST);
    auto& member_const = member->value;
    do {
        if (!FeatureFFIWamr::convertConstToGuest(exec_env,
                member_const.type, member_const.data, wasm_ret)) {
            FEATURE_LOG_ERROR("can not convert const value to guest!");
            break;
        }
    } while (0);
    *wasm_ret_p = wasm_ret;
}

static void method_call(wasm_exec_env_t exec_env, uint64_t *args)
{
    bool got_error = false;
    uint64_t* wasm_ret_p = args;
    uint64_t wasm_ret;
    native_raw_get_arg(void*, thiz_ptr, args);

    // wasm array values for rest parameters
    wasm_value_t wasm_array_data = { 0 }, wasm_array_len = { 0 };
    wasm_array_obj_t wasm_arr_ref = NULL;

    auto instance = instance_from_target((wasm_obj_t)thiz_ptr);
    auto manager = manager_from_instance(instance);
    auto member = (const Member*)wasm_runtime_get_function_attachment(exec_env);
    auto js_ctx = (JSContext*)ft_context_get_data(manager->getFeatureContext());
    auto description = instance->prototype()->description();

    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    const auto& method = member->method;
    auto method_params = method.parameters;
    FtPromiseId pid = -1;

    // count size
    bool has_rest_param = false;
    int optional_argc = 0;
    size_t fixed_argc = getParamCount(const_cast<FeatureType*>(method_params), &has_rest_param, &optional_argc);
    size_t argc = fixed_argc;

    // variadic parameters type
    ffi_type vari_arg_type;
    ffi_type* vari_arg_elem_types[3];
    FtVariParams vari_params;
    memset(&vari_params, 0, sizeof(vari_params));

    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_param && optional_argc, true);
    int32_t vari_argc = 0;
    if (has_rest_param) {
        wasm_struct_obj_t arr_struct_ref;
        uint64_t * vari_argv = args + fixed_argc;
        native_raw_get_arg(wasm_obj_t, obj_ref, vari_argv);
        assert(wasm_obj_is_struct_obj(obj_ref));
        arr_struct_ref = (wasm_struct_obj_t)obj_ref;
        wasm_struct_obj_get_field(arr_struct_ref, 0, false, &wasm_array_data);
        wasm_struct_obj_get_field(arr_struct_ref, 1, false, &wasm_array_len);
        wasm_arr_ref = (wasm_array_obj_t)(wasm_array_data.gc_obj);
        vari_argc = wasm_array_len.i32;
        argc += vari_argc;
        FEATURE_CHECK_GT(argc, fixed_argc);
        vari_params.vari_count = vari_argc;
    } else if (optional_argc > 0) {
        // for optional parameters, argc + optional must grater or equal to fixed_argc
        FEATURE_CHECK_GE(argc + optional_argc, fixed_argc);
    } else {
        // for method which do not have rest or optional parameters, argc equals to fixed_argc.
        FEATURE_CHECK_EQ(argc, fixed_argc);
    }

    // prepare and get args
    int32_t packed_argc = has_rest_param ? fixed_argc + 1 : fixed_argc;
    bool is_promise = FT_IS_PROMISE(method.return_type);
    int extra_argc = is_promise ? 3 : 2;
    ffi_type** ffi_arg_types = new ffi_type*[extra_argc + packed_argc + optional_argc + 1]; // FeaturInstance, data, maybe return promise, maybe variadic count, empty placeholder
    memset(ffi_arg_types, 0, sizeof(ffi_type*) * (extra_argc + packed_argc + optional_argc + 1));
    void** ffi_arg_values = new void*[extra_argc + packed_argc + optional_argc]; // FeaturInstance, data, maybe return promise, maybe variadic count
    memset(ffi_arg_values, 0, sizeof(void*) * (extra_argc + packed_argc + optional_argc));
    ffi_type* ffi_ret_type = nullptr;
    void* ffi_ret_value = nullptr;

    // prepare first two param
    ffi_arg_types[0] = &ffi_type_pointer; // FeatureContext
    ffi_arg_values[0] = &instance;
    ffi_arg_types[1] = &ffi_type_sint64; // data
    ffi_arg_values[1] = (void*)&method.data;
    if (is_promise) {
        ffi_arg_types[2] = &ffi_type_sint32;
    }

    do {
        for (size_t i = 0; i < fixed_argc; i++) {
            uint64_t curr_arg = args[i];
            auto param = method_params[i];
            if (FT_IS_PROMISE(param)) {
                FEATURE_LOG_ERROR("do not support promise as input param !");
                got_error = true;
                break;
            }
            if (!createTypeDeclaration(param, ffi_arg_types[extra_argc + i])) {
                FEATURE_LOG_ERROR("prepareType for type failed !");
                got_error = true;
                break;
            }
            if (!FeatureFFIWamr::convertValueToHost(instance, param, ffi_arg_values[extra_argc + i], exec_env, curr_arg)) {
                FEATURE_LOG_ERROR("convert argument %d failed !", i);
                got_error = true;
                break;
            }
        }
        if (got_error)
            break;

        // process rest parameters
        if (has_rest_param) {
            // prepare vari_params type
            vari_arg_type.size = 0;
            vari_arg_type.type = FFI_TYPE_STRUCT;
            vari_arg_type.elements = vari_arg_elem_types;
            vari_arg_elem_types[0] = &ffi_type_sint32;
            vari_arg_elem_types[1] = &ffi_type_pointer;
            vari_arg_elem_types[2] = nullptr;
            // prepare vari_params struct
            vari_params.vari_args = new ft_value_t[vari_params.vari_count];
            // pass param
            ffi_arg_types[fixed_argc + extra_argc] = &vari_arg_type;
            ffi_arg_values[fixed_argc + extra_argc] = &vari_params;
            for (int i = 0; i + fixed_argc < argc; i++) {
                void *addr = wasm_array_obj_elem_addr(wasm_arr_ref, i);
                wasm_anyref_obj_t anyref = *((wasm_anyref_obj_t *)addr);
                feature_value_t *js_any_ptr = (feature_value_t *)wasm_anyref_obj_get_value(anyref);
                // just passthrough guest param pointers
                auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(vari_params.vari_args[i]);
                *js_val_ptr = *js_any_ptr;
            }
        } else if (optional_argc > 0) {
            for (size_t i = argc; i < fixed_argc; i++) {
                auto param = method_params[i];
                FEATURE_CHECK_EQ(FT_IS_COMPLEX(param), true);
                OptionalType* optional_type = (OptionalType*)FT_GET_COMPLEX(param);
                FEATURE_CHECK_EQ(optional_type->header.type, COMPLEX_OPTIONAL);
                if (!createTypeDeclaration(param, ffi_arg_types[extra_argc + i])) {
                    FEATURE_LOG_ERROR("prepareType for type failed !");
                    got_error = true;
                    break;
                }
                ffi_arg_values[extra_argc + i] = &optional_type->fval;
            }
        }
        if (got_error)
            break;

        // prepeare return type
        if (!createTypeDeclaration(method.return_type, ffi_ret_type)) {
            FEATURE_LOG_ERROR("prepareType for complex type failed !");
            got_error = true;
            break;
        }
        // create return value pointer inneed.
        if (!is_promise && method.return_type != FT_VOID) {
            if (!createHostValue(method.return_type, ffi_ret_value, true)) {
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
            ret = ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, fixed_argc + extra_argc, packed_argc + extra_argc, ffi_ret_type, ffi_arg_types);
        } else {
            // FEATURE_LOG_DEBUG("prepare for function...");
            ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, fixed_argc + extra_argc, ffi_ret_type, ffi_arg_types);
        }
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            got_error = true;
            break;
        }

        // special handle for promise
        feature_value_t promise;
        auto w_instance = (FeatureInstanceWamr*)instance;
        if (is_promise) {
            ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(method.return_type);
            // create promise
            PromiseType* promise_type = (PromiseType*)complex_type;
            // create promise and add to instance
            pid = w_instance->addPromise(promise_type->resolveTypes[0], promise_type->resolveTypes[1]);
            promise = feature_dup_value(js_ctx, w_instance->getPromise(pid));
            // pass pid to native function
            ffi_arg_values[2] = &pid;
        }

        // invoke method
        NativeFunc callback = description->dynamic ? instance->getVirtualFunction(method.func.vtable_idx) : method.func.callback;
        FEATURE_CHECK_NE(callback, nullptr);
        ffi_call(&cif, callback, ffi_ret_value, ffi_arg_values);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!is_promise && method.return_type != FT_VOID) {
            //process return value
            if (!FeatureFFIWamr::convertValueToGuest(instance, method.return_type, ffi_ret_value, exec_env, wasm_ret)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                got_error = true;
            }
        } else if (is_promise) {
            feature_value_t* ppromise = dynamic_dup_value(js_ctx, promise);
            //把promise返回给ts层
            native_raw_return_type(void*, &wasm_ret);
            wasm_anyref_obj_t p_obj = wasm_anyref_obj_new(exec_env, ppromise);
            native_raw_set_return(p_obj);
        }
    } while (0);

    // free ffi call resources
    for (size_t i = 0; i < fixed_argc; i++) {
        // free type
        if (ffi_arg_types[i + extra_argc]) {
            freeTypeDeclaration(ffi_arg_types[i + extra_argc]);
        }
        // free value
        if (ffi_arg_values[i + extra_argc]) {
            FeatureFreeValue(ffi_arg_values[i + extra_argc]);
        }
    }

    freeTypeDeclaration(ffi_ret_type);
    // ffi_ret_value will cause "segmentation fault " when reture string to ts
    if (ffi_ret_value) {
        FeatureFreeValue(ffi_ret_value);
    }
    delete[] ffi_arg_values;
    delete[] ffi_arg_types;
    if (vari_params.vari_args) {
        delete[] vari_params.vari_args;
    }

    *wasm_ret_p = wasm_ret;

    // if error occurred, throw internal error
    // if (got_error) {
    //     FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native method failed !");
    // }
}

FeatureManagerWamr::FeatureManagerWamr(FeatureRegistry* registry)
  : FeatureManager(registry)
  , wamr_env_(nullptr)
{
}

bool FeatureManagerWamr::init()
{
    /* Register APIs required by ts2wasm */
    NativeSymbol *native_symbols;
    char *module_name;
    uint32_t symbol_count;

    symbol_count = get_libdyntype_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        FEATURE_LOG_ERROR("Register libdyntype APIs failed.");
        return false;
    }

    symbol_count = get_lib_console_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        FEATURE_LOG_ERROR("Register stdlib APIs failed.");
        return false;
    }

    symbol_count = get_lib_array_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        FEATURE_LOG_ERROR("Register stdlib APIs failed.");
        return false;
    }

    symbol_count = get_lib_timer_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        FEATURE_LOG_ERROR("Register stdlib APIs failed.");
        return false;
    }

    symbol_count = get_struct_indirect_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        FEATURE_LOG_ERROR("Register struct-dyn APIs failed.");
        return false;
    }

    if (!getFeatureRegistry()) {
        FEATURE_LOG_ERROR("feature register not exists!");
        return false;
    }

    for (const auto& pair : getFeatureRegistry()->getRegisteredFeatures()) {
        auto name = pair.first;
        auto description = pair.second.first;
        FEATURE_CHECK_NE(description, nullptr);

        if (strcmp(name.data(), "ATest") != 0 &&
                strcmp(name.data(), "Simple") != 0 &&
                strcmp(name.data(), "struct_test") != 0 &&
                strcmp(name.data(), "promise_test") != 0 &&
                strcmp(name.data(), "interface_test") != 0) {
            FEATURE_LOG_WARN("Feature '%s' is not for wamr!", name.data());
            continue;
        }

        FEATURE_LOG_WARN("register feature: '%s'", name.data());
        registerFeature(description);
    }
    return true;
}

void FeatureManagerWamr::release()
{
    if (!getFeatureRegistry())
        return;

    JSContext* js_ctx = (JSContext*)ft_context_get_data(getFeatureContext());
    auto release_prototype = [js_ctx](const FeatureRegistry::FeatureRegistryPair& pair) {
        auto proto = pair.second;
        if (!proto)
            return;

        auto description = pair.first;
        FEATURE_CHECK_NE(description, nullptr);
        // clear all feature instance at first, it will free all feature instance and call onDetach for them
        proto->clearAllInstances();
        // call feature's onDestroy
        if (description->native_callbacks && description->native_callbacks->onDestroy) {
            FEATURE_LOG_DEBUG("invoke onDestroy callback...");
            description->native_callbacks->onDestroy(js_ctx, proto);
        }
        delete proto;
    };

    // check if all instances deleted, then clear proto object
    for (const auto& feature_pair : getFeatureRegistry()->getRegisteredFeatures()) {
        release_prototype(feature_pair.second);
    }
    // uninit registery
    delete getFeatureRegistry();

    /* delete native symbols */
    if(!native_symbols_.empty()){
      for(size_t i = 0; i < native_symbols_.size(); i++){
        delete native_symbols_[i];
      }
    }

    if (getFeatureContext()) {
        ReleaseFeatureContextQjs(getFeatureContext());
        setFeatureContext(nullptr);
    }
}

bool FeatureManagerWamr::require(wasm_exec_env_t ctx, wasm_obj_t thiz, const char* name)
{
    FEATURE_LOG_INFO("featureRequire for name: %s", name);
    auto feature_pair = getFeatureRegistry()->findFeature(name);
    // FeatureRegistry::FeatureRegistryPair* feature_pair = registry_->findFeature(name);
    if (!feature_pair || !feature_pair->first) {
        FEATURE_LOG_WARN("can't find native feature '%s'!", name);
        return false;
    }
    auto description = feature_pair->first;

    wamr_env_ = ctx;
    if (!getFeatureContext()) {
        ft_context_ref ft_ctx = CreateFeatureContextQjs(dyntype_get_context()->js_ctx);
        setFeatureContext(ft_ctx);
    }

    auto& proto = feature_pair->second;

    if (!proto) {
        // create proto
        proto = new FeaturePrototypeWamr(description);
        if (!description->dynamic && description->native_callbacks->onCreate) {
            FEATURE_LOG_DEBUG("invoke onCreate callback...");
            description->native_callbacks->onCreate(ctx, proto);
        }
        proto->setFeatureManager(this);
        setPackageName(getFeatureRegistry()->getFeaturePackageName());
        setEnvName(FEATURE_ENV_NAME);
    }

    // create feature instance for the required object
    auto instance = std::make_unique<FeatureInstanceWamr>(proto, nullptr);
    auto instance_ptr = instance.get();
    set_instance_to_target(thiz, instance_ptr);

    // insert into instances array, update iid
    int iid = proto->addInstance(std::move(instance));
    instance_ptr->setInstanceId(iid);

    // create prototype class instance
    if (description->native_callbacks && description->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        description->native_callbacks->onRequired(ctx, instance_ptr);
    }

    return true;
}

bool FeatureManagerWamr::registerSymbol(void* func, const char* name, const char* sig, void* attach)
{
    auto symbol = new NativeSymbol();
    symbol->func_ptr = (void*)func;
    symbol->symbol = name;
    symbol->signature = sig;
    symbol->attachment = attach;
    FEATURE_LOG_INFO("register native symbol, name: %s, signature:%s", name, sig);
    if (!wasm_runtime_register_natives_raw("env", symbol, 1)) {
        FEATURE_LOG_ERROR("register native symbol: '%s' failed !", name);
        delete symbol;
        return false;
    }
    native_symbols_.push_back(symbol);

    return true;
}

int FeatureManagerWamr::registerFeature(const FeatureDescription* description)
{
   /* register interface api */
    if (description->members->type == MEMBER_METHOD) {
        for (int i = 0; i < description->member_count; i++) {
            Member member = description->members[i];
            FeatureType feature_type =  member.method.return_type;
            if (feature_type == FT_VOID || !FT_IS_COMPLEX(feature_type))
                continue;
            ComplexTypeHeader *complex_type = (ComplexTypeHeader *)FT_GET_COMPLEX(feature_type);
            if (complex_type->type != COMPLEX_INTERFACE)
                continue;

            InterfaceType *interface_type = (InterfaceType *)complex_type;
            const FeatureDescription* desc = interface_type->desc;
            registerFeature(desc);
        }
    }

    /* register class initNative api */
    char* init_name = new char[128];
    strcpy(init_name, description->name);
    strcat(init_name,"_init_native");
    if (!registerSymbol((void*)init_native, init_name, "(rr)", this)) {
        return false;
    }

    for (int i = 0; i < description->member_count; i++) {
        const Member& member = description->members[i];
        switch (member.type) {
            case MEMBER_NULL: {
                // not allowed
                FEATURE_CHECK(false && "invalid member type!");
                break;
            }
            case MEMBER_METHOD: {
                // register different type
                auto& method = member.method;
                char *method_name = new char[128];
                strcpy(method_name, description->name);
                /* special treat for interface */
                if (FT_IS_COMPLEX(method.return_type)) {
                    ComplexTypeHeader *complexType = (ComplexTypeHeader *)FT_GET_COMPLEX(method.return_type);
                    (complexType->type == COMPLEX_INTERFACE) ? strcat(method_name, "__") : strcat(method_name, "_");
                } else {
                    /* method->return_type is PRIMITIVE TYPE, method_name as before */
                    strcat(method_name, "_");
                }

                strcat(method_name, member.name);
                char *signature = new char[64];
                memset(signature, 0, 64);
                strcpy(signature, "(r");
                const FeatureType *ftype = (FeatureType *)method.parameters;
                while ((*ftype) != 0) {
                    if (FT_PARAM_REST_END == *ftype) {
                        signature[strlen(signature)] = 'r';
                        break;
                    }
                    char sig = FeatureFFIWamr::getFeatureSignature(*ftype);
                    if (sig != 0) {
                        signature[strlen(signature)] = sig;
                    }
                    ftype += 1;
                }
                strcat(signature, ")");
                /* if method->return_type is COMPLEX_INTERFACE, it's means the method is createxxx, and return is feature instance ptr
                there use f64 express it's return type */
                if (FT_IS_COMPLEX(method.return_type)) {
                    ComplexTypeHeader *complexType = (ComplexTypeHeader *)FT_GET_COMPLEX(method.return_type);
                    /* special treat for interface */
                    if (complexType->type == COMPLEX_INTERFACE) {
                        signature[strlen(signature)] = 'F';
                    } else {
                        /* COMPLEX TYPE, such as FTArray, deal with is as brefore */
                        char retc = FeatureFFIWamr::getFeatureSignature(method.return_type);
                        if (retc != 0)
                            signature[strlen(signature)] = retc;
                    }
                } else {
                    /* PRIMITIVE TYPE, deal with is as brefore */
                    char retc = FeatureFFIWamr::getFeatureSignature(method.return_type);
                    if (retc != 0)
                        signature[strlen(signature)] = retc;
                }
                if (!registerSymbol((void*)method_call, method_name, signature, (void*)(&member))) {
                    return false;
                }
                break;
            }
            case MEMBER_ACCESSOR: {
                // register accessor_get and accessor_set
                const MemberAccessor& accessor = member.accessor;
                if (accessor.getter.vtable_idx >= 0 || accessor.getter.callback) {
                    char *getter_name = new char[128];
                    strcpy(getter_name, description->name);
                    strcat(getter_name, "_get_");
                    strcat(getter_name, member.name);
                    strcat(getter_name, "_0");
                    char *signature = new char[64];
                    memset(signature, 0, 64);
                    strcpy(signature, "(r");
                    char type = FeatureFFIWamr::getFeatureSignature(accessor.type);
                    strcat(signature, ")");
                    if (type != 0)
                        signature[strlen(signature)] = type;
                    if (!registerSymbol((void*)accessor_get, getter_name, signature, (void*)(&member))) {
                        return false;
                    }
                }
                if(accessor.setter.vtable_idx >= 0 || accessor.setter.callback) {
                    char *setter_name = new char[128];
                    strcpy(setter_name, description->name);
                    strcat(setter_name, "_set_");
                    strcat(setter_name, member.name);
                    strcat(setter_name, "_0");
                    char *signature = new char[64];
                    memset(signature, 0, 64);
                    strcpy(signature, "(r");
                    char sig = FeatureFFIWamr::getFeatureSignature(accessor.type);
                    if (sig != 0)
                        signature[strlen(signature)] = sig;
                    strcat(signature, ")");
                    if (!registerSymbol((void*)accessor_set, setter_name, signature, (void*)(&member))) {
                        return false;
                    }
                }
                break;
            }
            case MEMBER_CONST: {
                // handle member const
                const MemberConst& member_const = member.value;
                char *const_name = new char[128];
                strcpy(const_name, description->name);
                strcat(const_name, "_const_");
                strcat(const_name, member.name);
                char *signature = new char[64];
                memset(signature, 0, 64);
                strcpy(signature, "(r");
                char sig = FeatureFFIWamr::getFeatureSignature(member_const.type);
                strcat(signature, ")");
                if (sig != 0)
                    signature[strlen(signature)] = sig;
                if (!registerSymbol((void*)const_get, const_name, signature, (void*)(&member))) {
                    return false;
                }
                break;
            }
        }
    }
    return 0;
}
}
