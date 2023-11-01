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
#include "feature_ffi_wamr.h"
#include "feature_framework.h"
#include "feature_instance_wamr.h"
#include "feature_log.h"
#include "feature_registry.h"
#include "feature_utils.h"
#include "feature_wamr_utils.h"

#include "libdyntype_export.h"

#include <assert.h>
#include <ffi.h>
#include <memory>
#include <string>

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

using FeatureRegistryPair = std::pair<const FeatureDescription*, FeaturePrototype*>;

static std::map<std::string, FeatureRegistryPair> registeredInterfaceFeatures_;

static void module_object_finalizer(wasm_obj_t obj, void *data)
{
    FeatureManagerWamr* manager = (FeatureManagerWamr*)data;
    FeatureInstance *instance = manager->getFeatureInstance(obj);
    printf("module object finalizer:%p, featureinstance:%p\n",obj,instance);

    for (const auto& prom : ((FeatureInstanceWamr*)instance)->promises_wamr) {
        JSContext* js_ctx = (JSContext*)ft_context_get_data(instance->prototype()->ft_ctx);
        feature_free_value(js_ctx, prom);
    }
    ((FeatureInstanceWamr*)instance)->promises_wamr.clear();

    ((FeatureInstanceWamr*)instance)->release();
}

static void init_native(wasm_exec_env_t exec_env, uint64_t *args){
    native_raw_get_arg(void*, thiz_ptr, args);

    native_raw_get_arg(void *, str, args);

    wasm_value_t arr_obj = { 0 };
    wasm_obj_t str_ref = (wasm_obj_t)str;
    wasm_struct_obj_get_field((wasm_struct_obj_t)str, 1, false, &arr_obj);
    WASMArrayObjectRef arr_ref = (WASMArrayObjectRef)(arr_obj.gc_obj);
    int arrlen = wasm_array_obj_length(arr_ref);
    char *p_src = (char *)wasm_array_obj_first_elem_addr(arr_ref);

    FEATURE_LOG_INFO("class name: %s", p_src);
    WamrAttachment* attachment = (WamrAttachment*)wasm_runtime_get_function_attachment(exec_env);
    FeatureManagerWamr* manager = attachment->manager;

    manager->require(exec_env, (wasm_obj_t)thiz_ptr, p_src);

    // set object destructor func
    bool ret = wasm_obj_set_gc_finalizer(exec_env, (wasm_obj_t)thiz_ptr,(wasm_obj_finalizer_t)module_object_finalizer, manager);
}

static void accessor_get(wasm_exec_env_t exec_env, uint64_t *args)
{
    uint64_t *tmp_args = args;
    native_raw_get_arg(void *, thiz_ptr, args); // pop this pointer
    wasm_val_t method_ret_value;
    WamrAttachment* attachment = (WamrAttachment*)wasm_runtime_get_function_attachment(exec_env);
    FeatureManagerWamr* manager = attachment->manager;
    FeatureInstance *instance = manager->getFeatureInstance((wasm_obj_t)thiz_ptr);
    Member* member = manager->getFeatureMember(attachment->description, attachment->index);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor *accessor = &member->accessor;
    auto description = instance->prototype()->description;
    /* deal with interface real instance (include member vatable) */
    wasm_value_t val = { 0 };
    if (description->dynamic)
    {
        wasm_obj_t obj_ref = (wasm_obj_t)thiz_ptr;
        /* every interface class have a field and name is instance, it's index in the class obj(because the index 0 is obj this) is 1 */
        wasm_struct_obj_get_field((wasm_struct_obj_t)obj_ref, 1, false, &val);
        instance = (FeatureInstance *)val.gc_obj;
    }
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    NativeFunc callback = description->dynamic ? instance->getVirtualFunction(accessor->getter.vtable_idx) : accessor->getter.callback;
    FEATURE_CHECK_NE(callback, nullptr);

    ffi_type *ffi_params[2] = {&ffi_type_pointer, &ffi_type_sint64};
    ffi_type *ffi_ret = nullptr;
    void *arg_values[2] = {&instance, &accessor->data};
    void *ret_value = nullptr;
    do
    {
        if (!createTypeDeclaration(accessor->type, ffi_ret)) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        if (!createHostValue(accessor->type, ret_value, true)) {
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
        ffi_call(&cif, callback, ret_value, arg_values);
        // process return value
        if (!FeatureFFIWamr::convertValueToGuest(instance, accessor->type, ret_value, exec_env, method_ret_value)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            // feature_free_value(instance->prototype()->ctx, method_ret_value);
            // method_ret_value = JSE_EXCEPTION;
        }
        switch (method_ret_value.kind) {
            case WASM_I32:
            {
                native_raw_return_type(double, tmp_args);
                native_raw_set_return(method_ret_value.of.i32);
            }
            break;
            case WASM_F64:
            {
                native_raw_return_type(double, tmp_args);
                native_raw_set_return(method_ret_value.of.f64);
            } 
            break;
            case WASM_ANYREF:
            {
                native_raw_return_type(void *, tmp_args);
                const char *str = (char *)method_ret_value.of.foreign;
                wasm_struct_obj_t obj = create_wasm_string(exec_env, str);
                native_raw_set_return(obj);

            }
            break;
            default:
                break;
        }

    } while (0);

    // free resources
    freeTypeDeclaration(ffi_ret);
    FeatureFreeValue(ret_value);
}

static void accessor_set(wasm_exec_env_t exec_env, uint64_t *args)
{
    native_raw_get_arg(void *, thiz_ptr, args); // pop this pointer
    wasm_val_t method_ret_value;
    WamrAttachment* attachment = (WamrAttachment*)wasm_runtime_get_function_attachment(exec_env);
    FeatureManagerWamr* manager = attachment->manager;
    FeatureInstance *instance = manager->getFeatureInstance((wasm_obj_t)thiz_ptr);
    Member* member = manager->getFeatureMember(attachment->description, attachment->index);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor *accessor = &member->accessor;
    auto description = instance->prototype()->description;
    /* deal with interface real instance (include member vatable) */
    wasm_value_t val = { 0 };
    if (description->dynamic)
    {
        wasm_obj_t obj_ref = (wasm_obj_t)thiz_ptr;
        /* every interface class have a field and name is instance, it's index in the class obj(because the index 0 is obj this) is 1 */
        wasm_struct_obj_get_field((wasm_struct_obj_t)obj_ref, 1, false, &val);
        instance = (FeatureInstance *)val.gc_obj;
    }

    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type *ffi_params[3] = {&ffi_type_pointer, &ffi_type_sint64, nullptr};
    void *arg_value_input = nullptr;
    void *arg_values[3] = {&instance, &accessor->data, nullptr};

    NativeFunc callback = description->dynamic ? instance->getVirtualFunction(accessor->setter.vtable_idx) : accessor->setter.callback;
    FEATURE_CHECK_NE(callback, nullptr);

    do {
        // prepare third param type declaration, create by accessor type
        if (!createTypeDeclaration(accessor->type, ffi_params[2])) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        // fill third param using guest value and accesor type
        if (!FeatureFFIWamr::convertValueToHost(instance, accessor->type, arg_value_input, exec_env, args++)) {
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
        ffi_call(&cif, callback, arg_values[2], arg_values);
    } while (0);

    // free resources
    freeTypeDeclaration(ffi_params[2]);
    FeatureFreeValue(arg_value_input);
}

static inline void fill_struct_data(ObjectMapType &obj_type, uint64_t ptr, ts_value_t obj_arr[], uint32_t count)
{
    if (count <= 0) {
        FEATURE_LOG_ERROR("need fill struct is null!");
    }
    ts_value_t obj_field;
    for (uint32_t i = 0; i < count; i++)
    {
        // fill it
        auto member = obj_type.members[i];
        void *member_ptr = (void *)((char *)ptr + member.offset);
        FeatureType featureType = member.type;
        TRY_GET_REAL_TYPE(featureType);
        if (FT_IS_PRIMITIVE(featureType))
        {
            switch (FT_GET_VALUE(featureType))
            {
            case FT_BOOLEAN:
            {
                obj_field.of.i32 = *(int32_t *)member_ptr;
                obj_field.type = TS_BOOLEAN;
                obj_arr[i] = obj_field;
            }
            break;
            case FT_INT:
            case FT_INT8:
            case FT_UINT8:
            case FT_INT16:
            case FT_UINT16:
            case FT_INT32:
            case FT_UINT32:
            case FT_INT64:
            case FT_UINT64:
            case FT_FLOAT:
            case FT_DOUBLE:
            {
                obj_field.of.f64 = *(int64_t *)member_ptr;
                obj_field.type = TS_NUMBER;
                obj_arr[i] = obj_field;
            }
            break;
            case FT_CHAR:
            {
                char **title_ptr = (char **)member_ptr;
                obj_field.of.ref = *title_ptr;
                obj_field.type = TS_STRING;
                obj_arr[i] = obj_field;
            }
            break;
            default:
                break;
            }
        }
    }
}

static void method_call(wasm_exec_env_t exec_env, uint64_t *args)
{
    bool got_error = false;
    feature_value_t* ret_promise;
    wasm_val_t wasm_ret_value;
    uint64_t *wasm_ret_ptr = args;
    native_raw_get_arg(void*, thiz_ptr, args);

    // wasm array values for rest parameters
    wasm_value_t wasm_array_data = { 0 }, wasm_array_len = { 0 };
    wasm_array_obj_t wasm_arr_ref;

    WamrAttachment* attachment = (WamrAttachment*)wasm_runtime_get_function_attachment(exec_env);
    FeatureManagerWamr* manager = attachment->manager;
    FeatureInstance *instance = manager->getFeatureInstance((wasm_obj_t)thiz_ptr);
    JSContext* js_ctx = (JSContext*)ft_context_get_data(instance->prototype()->ft_ctx);
    auto description = instance->prototype()->description;

    /* deal with interface real instance (include member vatable) */
    wasm_value_t val = { 0 };
    if (description->dynamic)
    {
        wasm_obj_t obj_ref = (wasm_obj_t)thiz_ptr;
        /* every interface class have a field and name is instance, it's index in the class obj(because the index 0 is obj this) is 1 */
        wasm_struct_obj_get_field((wasm_struct_obj_t)obj_ref, 1, false, &val);
        instance = (FeatureInstance *)val.gc_obj;
    }

    Member* member = manager->getFeatureMember(attachment->description, attachment->index);
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
        for (int i = 0; i < fixed_argc; i++) {
            //feature_value_t currArg = argv[i];
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
            if (!FeatureFFIWamr::convertValueToHost(instance, param, ffi_arg_values[extra_argc + i], exec_env, args++)) {
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
            for (int i = argc; i < fixed_argc; i++) {
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
        feature_value_t tmp_p;
        // instance->prototype()->ft_ctx =  dyntype_get_context()->js_ctx;
        if (is_promise) {
            ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(method.return_type);
            // create promise
            PromiseType* promise_type = (PromiseType*)complex_type;
            // create promise and add to instance
            pid = ((FeatureInstanceWamr*)instance)->addPromise(promise_type->resolveTypes[0], promise_type->resolveTypes[1]);
            feature_value_t promise = ((FeatureInstanceWamr*)instance)->getPromise(pid);
            ((FeatureInstanceWamr*)instance)->addPromise_wamr(promise);
            // pass pid to native function
            ffi_arg_values[2] = &pid;
            // dup and return promise object.
            //ret_promise = feature_dup_value(instance->prototype()->ctx, promiseData->promise);
            feature_value_t tmp_p = feature_dup_value(js_ctx, promise);
            ret_promise = dynamic_dup_value(js_ctx, tmp_p);
        }

        // invoke method
        NativeFunc callback = description->dynamic ? instance->getVirtualFunction(method.func.vtable_idx) : method.func.callback;
        FEATURE_CHECK_NE(callback, nullptr);
        ffi_call(&cif, callback, ffi_ret_value, ffi_arg_values);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!is_promise && method.return_type != FT_VOID) {
            //process return value
            if (!FeatureFFIWamr::convertValueToGuest(instance, method.return_type, ffi_ret_value, exec_env, wasm_ret_value)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                feature_free_value(js_ctx, tmp_p);
                // wasm_ret_value = FEATURE_EXCEPTION;
                got_error = true;
            }
            switch (wasm_ret_value.kind) {
                case WASM_I32:
                {
                    native_raw_return_type(double, wasm_ret_ptr);
                    native_raw_set_return(wasm_ret_value.of.i32);
                }
                break;
                case WASM_F64:
                {
                    native_raw_return_type(double, wasm_ret_ptr);
                    native_raw_set_return(wasm_ret_value.of.f64);
                }
                break;
                case WASM_ANYREF:
                {
                    wasm_struct_obj_t obj = nullptr;
                    if (FT_IS_PRIMITIVE(method.return_type))
                    {
                        native_raw_return_type(void *, wasm_ret_ptr);
                        const char *str = (char *)wasm_ret_value.of.foreign;
                        obj = create_wasm_string(exec_env, str);
                        native_raw_set_return(obj);
                    }
                    /* if return type is complex, and then is array or struct type.*/
                    else if (FT_IS_COMPLEX(method.return_type))
                    {
                        ComplexTypeHeader *complex_type = (ComplexTypeHeader *)FT_GET_COMPLEX(method.return_type);
                        switch (complex_type->type)
                        {
                        case COMPLEX_STRUCT_MAP:
                        {
                            native_raw_return_type(void *, wasm_ret_ptr);
                            ObjectMapType &obj_map_type = *(ObjectMapType *)complex_type;
                            auto member_count = countMember(obj_map_type.members);
                            ts_value_t obj_arr[member_count];
                            /* call fill_struct_data api to fill data in obj array as above */
                            fill_struct_data(obj_map_type, wasm_ret_value.of.foreign, obj_arr, member_count);
                            /* call create_wasm_class_struct api from feature_wamr_utils.h */
                            obj = create_wasm_class_struct(exec_env, obj_arr, member_count);
                            native_raw_set_return(obj);
                        }
                        break;
                        case COMPLEX_ARRAY:
                        {
                            native_raw_return_type(void *, wasm_ret_ptr);
                            FtArray *array = (FtArray *)wasm_ret_value.of.foreign;
                            uint32_t len = array->_size;
                            obj = create_wasm_array_with_string(exec_env, array->_element, len);
                            native_raw_set_return(obj);
                        }
                        break;
                        case COMPLEX_INTERFACE:
                        {
                            native_raw_return_type(void *, wasm_ret_ptr);
                            native_raw_set_return((void *)wasm_ret_value.of.foreign);
                        }
                        break;
                        }
                    }
                }
                break;
                default:
                    break;
            }
        } else if (is_promise) {
            //把promise返回给ts层
            native_raw_return_type(void*, wasm_ret_ptr);
            wasm_anyref_obj_t p_obj = wasm_anyref_obj_new(exec_env, ret_promise);
            native_raw_set_return(p_obj);
            //feature_free_value(js_ctx, tmp_p);
        }
    } while (0);

    // free ffi call resources
    for (int i = 0; i < fixed_argc; i++) {
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

    // if error occurred, throw internal error
    // if (got_error) {
    //     FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native method failed !");
    // }
}

FeatureManagerWamr::FeatureManagerWamr(FeatureRegistry* registry)
  : registry_(registry)
  , ft_ctx_(nullptr)
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

    if (!registry_) {
        FEATURE_LOG_ERROR("feature register not exists!");
        return false;
    }

    for (const auto& pair : registry_->getRegisteredFeatures()) {
        auto name = pair.first;
        auto description = pair.second.first;
        auto proto = pair.second.second;
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
    if (!registry_)
        return;

    for (const auto& pair : registry_->getRegisteredFeatures()) {
        auto proto = pair.second.second;
        auto description = pair.second.first;
        FEATURE_CHECK_NE(description, nullptr);
        if (proto) {
            JSContext* js_ctx = (JSContext*)ft_context_get_data(proto->ft_ctx);
            // clear all feature instance at first, it will free all feature instance and call onDetach for them
            proto->clearAllInstances();
            // call feature's onDestroy
            if (description->native_callbacks && description->native_callbacks->onDestroy) {
                FEATURE_LOG_DEBUG("invoke onDestroy callback...");
                description->native_callbacks->onDestroy(js_ctx, proto);
            }
            auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(proto->ft_proto);
            if (!feature_is_undefined(*js_proto_ptr)) {
                feature_free_value(js_ctx, *js_proto_ptr);
                *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
            }
        }
        // delete prototype
        delete pair.second.second;
    }
    // uninit registery
    delete registry_;
    registry_ = nullptr;

    /* delete nativesymbol */
    if(!nativesymbol_.empty()){
      for(int i = 0; i < nativesymbol_.size(); i++){
        delete nativesymbol_[i];
      }
    }

    if (ft_ctx_) {
        ReleaseFeatureContextQjs(ft_ctx_);
        ft_ctx_ = nullptr;
    }
}

Member* FeatureManagerWamr::getFeatureMember(const FeatureDescription* description, int index)
{
    if (!description) {
        FEATURE_LOG_WARN("can't find native feature!");
        return nullptr;
    }

    return const_cast<Member*>(&(description->members[index]));
}

FeatureInstance* FeatureManagerWamr::getFeatureInstance(wasm_obj_t obj)
{
    auto pos = feature_instance_map_.find(obj);
    if (pos == feature_instance_map_.end())
        return NULL;

    return pos->second;
}

bool FeatureManagerWamr::require(wasm_exec_env_t ctx, wasm_obj_t thiz, const char* name)
{
    FEATURE_LOG_INFO("featureRequire for name: %s", name);
    FeatureRegistry::FeatureRegistryPair *feature_pair = nullptr;
    /* find name if exist, feature_pair new assign value by registeredInterfaceFeatures_*/
    auto pos = registeredInterfaceFeatures_.find(name);
    if (pos != registeredInterfaceFeatures_.end()) {
         feature_pair = &pos->second;
    } else {
        feature_pair = registry_->findFeature(name);
    }
    // FeatureRegistry::FeatureRegistryPair* feature_pair = registry_->findFeature(name);
    if (!feature_pair || !feature_pair->first) {
        FEATURE_LOG_WARN("can't find native feature '%s'!", name);
        return false;
    } 
    auto description = feature_pair->first;

    if (!ft_ctx_)
        ft_ctx_ = CreateFeatureContextQjs(dyntype_get_context()->js_ctx);

    auto& proto = feature_pair->second;

    if (!proto)
    {
        // create proto
        proto = new FeaturePrototype(ft_ctx_, feature_pair->first);
        proto->wamr_env = ctx;

        if (!description->dynamic)
        {
            if (description->native_callbacks->onCreate)
            {
                FEATURE_LOG_DEBUG("invoke onCreate callback...");
                description->native_callbacks->onCreate(ctx, proto);
            }
        }
    }

    // create feature instance for the required object
    auto featureInstance = std::make_unique<FeatureInstanceWamr>(proto, nullptr);
    feature_instance_map_[thiz] = featureInstance.get();

    // insert into instances array, update iid
    int iid = proto->addInstance(std::move(featureInstance));
    proto->instances[iid]->setInstanceId(iid);
    
    // create prototype class instance
    if (description->native_callbacks && description->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        description->native_callbacks->onRequired(ctx, proto->instances[iid].get());
    }

    return true;
}

bool FeatureManagerWamr::makeAttachment(NativeSymbol* symbol, const FeatureDescription* description, int index)
{
    if (symbol->attachment)
        return false;

    WamrAttachment attachment = { this, symbol, description, index};
    symbol_attachment_map_[symbol] = attachment;
    symbol->attachment = &(symbol_attachment_map_[symbol]);
    return true;
}

int FeatureManagerWamr::registerFeature(const FeatureDescription* description)
{ 
   /* register interface api */
    if (description->members->type == MEMBER_METHOD)
    {
        for (size_t i = 0; i < description->member_count; i++)
        {
            Member member = description->members[i];
            FeatureType feature_type =  member.method.return_type;
            if (feature_type != FT_VOID && FT_IS_COMPLEX(feature_type))
            {
                ComplexTypeHeader *complexType = (ComplexTypeHeader *)FT_GET_COMPLEX(feature_type);
                switch (complexType->type)
                {
                case COMPLEX_INTERFACE:
                {
                    InterfaceType *interfaceType = (InterfaceType *)complexType;
                    const FeatureDescription *interfaceDesc = interfaceType->desc;
                    if (interfaceDesc->name)
                    {
                        registeredInterfaceFeatures_[interfaceDesc->name] = std::pair<const FeatureDescription *, FeaturePrototype *>(interfaceDesc, nullptr);
                    }
                    registerFeature(interfaceDesc);
                }
                break;
                default:
                {
                    break;
                }
                }
            }
        }
    }

    /* register class initNative api */
    auto init_symbol = new NativeSymbol();
    nativesymbol_.push_back(init_symbol);
    init_symbol->func_ptr = (void*)init_native;
    char* name = new char[128];
    strcpy(name, description->name);
    strcat(name,"_init_native");
    init_symbol->symbol = name;
    init_symbol->signature = "(rr)";
    // FEATURE_LOG_INFO("register init_native method, name: %s and param:%s", name, init_symbol->signature);
    makeAttachment(init_symbol, description, -1);

    if (!wasm_runtime_register_natives_raw("env", init_symbol, 1)) {
        FEATURE_LOG_ERROR("register method: '%s' failed !", name);
        return false;
    }

    for (int i = 0; i < description->member_count; i++) {
        const Member* const_p = &(description->members[i]);
        Member* modifier = const_cast<Member*>(const_p);
        Member& member = *modifier;

        switch (member.type) {
            case MEMBER_NULL: {
                // not allowed
                FEATURE_CHECK(false && "invalid member type!");
                break;
            }
            case MEMBER_METHOD:
            {
                // register different type
                MemberMethod *method = &member.method;
                auto native_symbol = new NativeSymbol();
                nativesymbol_.push_back(native_symbol);
                native_symbol->func_ptr = (void *)method_call;
                char *name1 = new char[128];
                strcpy(name1, description->name);
                /* special treat for interface */
                if (FT_IS_COMPLEX(method->return_type))
                {
                    ComplexTypeHeader *complexType = (ComplexTypeHeader *)FT_GET_COMPLEX(method->return_type);
                    (complexType->type == COMPLEX_INTERFACE) ? strcat(name1, "__") : strcat(name1, "_");
                }
                else /* method->return_type is PRIMITIVE TYPE, name1 as before */
                {
                    strcat(name1, "_");
                }

                strcat(name1, const_p->name);
                native_symbol->symbol = name1;
                char *param = new char[64];
                memset(param, 0, 64);
                strcpy(param, "(r");
                FeatureType *pars = (FeatureType *)method->parameters;
                while ((*pars) != 0)
                {
                    if (FT_PARAM_REST_END == *pars)
                    {
                        param[strlen(param)] = 'r';
                        break;
                    }
                    char sig = FeatureFFIWamr::getFeatureSignature(*pars);
                    if (sig != 0)
                    {
                        param[strlen(param)] = sig;
                    }
                    pars += 1;
                }
                strcat(param, ")");
                /* if method->return_type is COMPLEX_INTERFACE, it's means the method is createxxx, and return is feature instance ptr
                there use f64 express it's return type */
                if (FT_IS_COMPLEX(method->return_type))
                {
                    ComplexTypeHeader *complexType = (ComplexTypeHeader *)FT_GET_COMPLEX(method->return_type);
                    /* special treat for interface */
                    if (complexType->type == COMPLEX_INTERFACE)
                    {
                        param[strlen(param)] = 'F';
                    }
                    else
                    {
                        /* COMPLEX TYPE, such as FTArray, deal with is as brefore */
                        char retc = FeatureFFIWamr::getFeatureSignature(method->return_type);
                        if (retc != 0)
                            param[strlen(param)] = retc;
                    }
                }
                else
                {
                    /* PRIMITIVE TYPE, deal with is as brefore */
                    char retc = FeatureFFIWamr::getFeatureSignature(method->return_type);
                    if (retc != 0)
                        param[strlen(param)] = retc;
                }
                // FEATURE_LOG_INFO("register method, name: %s, param: %s", name1, param);
                native_symbol->signature = param;
                makeAttachment(native_symbol, description, i);
                if (!wasm_runtime_register_natives_raw("env", native_symbol, 1))
                {
                    FEATURE_LOG_ERROR("register memthod: '%s' failed !", name1);
                    return false;
                }
                break;
            }
            case MEMBER_ACCESSOR: {
                // register accessor_get and accessor_set
                const MemberAccessor& accessor = member.accessor;
                if (accessor.getter.vtable_idx >= 0) {
                    auto native_symbol = new NativeSymbol();
                    nativesymbol_.push_back(native_symbol);
                    native_symbol->func_ptr = (void *)accessor_get;
                    char *buf = new char[128];
                    strcpy(buf, description->name);
                    strcat(buf, "_get_");
                    strcat(buf, member.name);
                    strcat(buf, "_0");
                    native_symbol->symbol = buf;
                    char *signature = new char[64];
                    memset(signature, 0, 64);
                    strcpy(signature, "(r");
                    char type = FeatureFFIWamr::getFeatureSignature(accessor.type);
                    strcat(signature, ")");
                    if (type != 0)
                        signature[strlen(signature)] = type;
                    // FEATURE_LOG_INFO("register getter, name: %s, signature: %s", buf, signature);
                    native_symbol->signature = signature;
                    makeAttachment(native_symbol, description, i);
                    if (!wasm_runtime_register_natives_raw("env", native_symbol, 1)) {
                        FEATURE_LOG_ERROR("register getter: '%s' failed !", buf);
                        return false;
                    }
                }
                if(accessor.setter.vtable_idx >= 0) {
                    auto native_symbol = new NativeSymbol();
                    nativesymbol_.push_back(native_symbol);
                    native_symbol->func_ptr = (void *)accessor_set;
                    char *buf = new char[128];
                    strcpy(buf, description->name);
                    strcat(buf, "_set_");
                    strcat(buf, member.name);
                    strcat(buf, "_0");
                    native_symbol->symbol = buf;
                    char *signature = new char[64];
                    memset(signature, 0, 64);
                    strcpy(signature, "(r");
                    char type = FeatureFFIWamr::getFeatureSignature(accessor.type);
                    if (type != 0)
                        signature[strlen(signature)] = type;
                    strcat(signature, ")");
                    // FEATURE_LOG_INFO("register setter, name: %s, signature: %s", buf, signature);
                    native_symbol->signature = signature;
                    makeAttachment(native_symbol, description, i);
                    if (!wasm_runtime_register_natives_raw("env", native_symbol, 1)) {
                        FEATURE_LOG_ERROR("register setter: '%s' failed !", buf);
                        return false;
                    }
                }
            } break;
        }
    }
    return 0;
}
}

