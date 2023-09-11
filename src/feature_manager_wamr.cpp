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
#include "feature_instance_wamr.h"
#include "feature_ffi_wamr.h"
#include "feature_framework.h"
#include "feature_log.h"
#include "feature_registry.h"
#include "feature_utils.h"

#include "dyntype.h"

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
dyntype_dup_value(feature_context_ref ctx, feature_value_t value);
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

extern "C" wasm_struct_obj_t create_wasm_string(wasm_exec_env_t exec_env, const char *value);
extern "C" wasm_struct_obj_t create_wasm_array_with_string(wasm_exec_env_t exec_env, void *ptr, uint32_t arrlen);

static void module_object_finalizer(wasm_obj_t obj, void *data)
{
    FeatureManagerWamr* manager = (FeatureManagerWamr*)data;
    FeatureInstance *instance = manager->getFeatureInstance(obj);
    printf("module object finalizer:%p, featureinstance:%p\n",obj,instance);

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

    printf("class name:%s\n",p_src);
    WarmAttachment* attachment = (WarmAttachment*)wasm_runtime_get_function_attachment(exec_env);
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
    WarmAttachment* attachment = (WarmAttachment*)wasm_runtime_get_function_attachment(exec_env);
    FeatureManagerWamr* manager = attachment->manager;
    FeatureInstance *instance = manager->getFeatureInstance((wasm_obj_t)thiz_ptr);
    Member* member = manager->getUnitMember(attachment->unit, attachment->index);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor *accessor = &member->accessor;
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
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
        ffi_call(&cif, accessor->getter, ret_value, arg_values);
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
                break;
            }
            case WASM_ANYREF:
            {
                native_raw_return_type(void *, tmp_args);
                const char *str = (char *)method_ret_value.of.foreign;
                wasm_struct_obj_t obj = create_wasm_string(exec_env, str);
                native_raw_set_return(obj);
                break;
            }
            default:
                break;
        }

    } while (0);

    // free resources
    freeTypeDeclaration(ffi_ret);
    FreeFeatureValue(ret_value);
}

static void accessor_set(wasm_exec_env_t exec_env, uint64_t *args)
{
    native_raw_get_arg(void *, thiz_ptr, args); // pop this pointer
    wasm_val_t method_ret_value;
    WarmAttachment* attachment = (WarmAttachment*)wasm_runtime_get_function_attachment(exec_env);
    FeatureManagerWamr* manager = attachment->manager;
    FeatureInstance *instance = manager->getFeatureInstance((wasm_obj_t)thiz_ptr);
    Member* member = manager->getUnitMember(attachment->unit, attachment->index);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor *accessor = &member->accessor;
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type *ffi_params[3] = {&ffi_type_pointer, &ffi_type_sint64, nullptr};
    void *arg_value_input = nullptr;
    void *arg_values[3] = {&instance, &accessor->data, nullptr};
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
        ffi_call(&cif, accessor->setter, arg_values[2], arg_values);
    } while (0);

    // free resources
    freeTypeDeclaration(ffi_params[2]);
    FreeFeatureValue(arg_value_input);
}

static void method_call(wasm_exec_env_t exec_env, uint64_t *args)
{
    bool got_error = false;
    wasm_val_t method_ret_value;
    feature_value_t* promise_ret_value;

    uint64_t *tmp_args = args;
    native_raw_get_arg(void*, thiz_ptr, args);
    size_t argc = 0;

    wasm_value_t wasm_array_data = { 0 }, wasm_array_len = { 0 };
    wasm_struct_obj_t arr_struct_ref;
    wasm_array_obj_t arr_ref;
    feature_value_t **js_value = NULL;

    WarmAttachment* attachment = (WarmAttachment*)wasm_runtime_get_function_attachment(exec_env);
    FeatureManagerWamr* manager = attachment->manager;
    FeatureInstance *instance = manager->getFeatureInstance((wasm_obj_t)thiz_ptr);
    JSContext* js_ctx = (JSContext*)ft_context_get_data(instance->prototype()->ft_ctx);
    Member* member = manager->getUnitMember(attachment->unit, attachment->index);
    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    const auto& method = member->method;
    auto currParam = method.parameters;
    FEATURE::FeaturePromiseHandle promiseHandle = -1;
    feature_value_t promise_obj = FEATURE_VALUE_UNDEFINED;
    // count size
    bool has_rest_param = false;
    int optional_count = 0;
    int method_param_count = getParamCount(const_cast<FeatureType*>(currParam), &has_rest_param, &optional_count);
    argc = method_param_count;
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_param && optional_count, true);
    int32_t variadic_count = 0;
    // check argument count match.
    // FEATURE_LOG_DEBUG("required param count: %d, received param count: %d", method_param_count, argc);
    // beacuse we support rest parameters, so argc is greater or equal to method_param_count.
    if (has_rest_param) {
        FEATURE_CHECK_GE(argc, method_param_count);
    } else if (optional_count) {
        // for optional parameters, argc + optional must grater or equal to method_param_count
        FEATURE_CHECK_GE(argc + optional_count, method_param_count);
    } else {
        // for method which do not have rest or optional parameters, argc equals to method_param_count.
        FEATURE_CHECK_EQ(argc, method_param_count);
    }
    // exact param from feature_value_t, finally call method
    //int paramTotalIdx = 0;
    // prepare and get params
    ffi_type** ffi_params = new ffi_type*[argc + optional_count + 5]; // FeaturInstance, data, maybe return promise, maybe variadic count, empty placeholder
    ffi_type* ffi_ret = nullptr;
    memset(ffi_params, 0, sizeof(ffi_type*) * (argc + optional_count + 5));

    void** ffi_arg_values = new void*[argc + optional_count + 4]; // FeaturInstance, data, maybe return promise, maybe variadic count
    memset(ffi_arg_values, 0, sizeof(void*) * (argc + optional_count + 4));
    void* ffi_ret_value = nullptr;

    feature_value_t** rest_params = nullptr;

    // prepare first two param
    ffi_params[0] = &ffi_type_pointer; // FeatureContext
    ffi_arg_values[0] = &instance;
    ffi_params[1] = &ffi_type_sint64; // data
    ffi_arg_values[1] = (void*)&method.data;
    bool isPromise = FT_IS_PROMISE(method.return_type);
    if (isPromise) {
        ffi_params[2] = &ffi_type_sint32;
    }
    int external_count = isPromise ? 3 : 2;

    do {
        for (int i = 0; i < method_param_count && i < argc; i++) {
            //feature_value_t currArg = argv[i];
            auto param = currParam[i];
            if (FT_IS_PROMISE(param)) {
                FEATURE_LOG_ERROR("do not support promise as input param !");
                got_error = true;
                break;
            }
            if (!createTypeDeclaration(param, ffi_params[external_count + i])) {
                FEATURE_LOG_ERROR("prepareType for type failed !");
                got_error = true;
                break;
            }
            if (!FeatureFFIWamr::convertValueToHost(instance, param, ffi_arg_values[external_count + i], exec_env, args++)) {
                FEATURE_LOG_ERROR("convert argument %d failed !", i);
                got_error = true;
                break;
            }
        }
        if (got_error)
            break;

        // process rest parameters
        if (has_rest_param) {
            native_raw_get_arg(wasm_obj_t, obj_ref, args);
            assert(wasm_obj_is_struct_obj(obj_ref));
            arr_struct_ref = (wasm_struct_obj_t)obj_ref;
            wasm_struct_obj_get_field(arr_struct_ref, 0, false, &wasm_array_data);
            wasm_struct_obj_get_field(arr_struct_ref, 1, false, &wasm_array_len);

            arr_ref = (wasm_array_obj_t)(wasm_array_data.gc_obj);
            variadic_count = wasm_array_len.i32;

            if(variadic_count > 0) {
                ffi_type** ffi_params_new = new ffi_type*[argc + optional_count + 5 + variadic_count]; // FeaturInstance, data, maybe return promise, maybe variadic count, empty placeholder
                //ffi_type* ffi_ret = nullptr;
                memset(ffi_params_new, 0, sizeof(ffi_type*) * (argc + optional_count + 5 + variadic_count));
                memcpy(ffi_params_new, ffi_params, sizeof(ffi_type*) * (argc + optional_count + 5));
                delete[] ffi_params;
                ffi_params = ffi_params_new;

                void** ffi_arg_values_new = new void*[argc + optional_count + 4 + variadic_count]; // FeaturInstance, data, maybe return promise, maybe variadic count
                memset(ffi_arg_values_new, 0, sizeof(void*) * (argc + optional_count + 4  + variadic_count));
                memcpy(ffi_arg_values_new, ffi_arg_values, sizeof(void*) * (argc + optional_count + 4 ));
                delete[] ffi_arg_values;
                ffi_arg_values = ffi_arg_values_new;
                //void* ffi_ret_value = nullptr;

                //variadic_count = argc - method_param_count;
                //rest_params = new feature_value_t*[variadic_count];
                ffi_params[method_param_count + external_count] = &ffi_type_sint32;
                ffi_arg_values[method_param_count + external_count] = &variadic_count;
                js_value = new feature_value_t*[variadic_count];
                for (int i = 0; i < variadic_count; i++) {
                    void *addr = wasm_array_obj_elem_addr(arr_ref, i);
                    wasm_anyref_obj_t anyref = *((wasm_anyref_obj_t *)addr);
                    feature_value_t *tmp = (feature_value_t *)wasm_anyref_obj_get_value(anyref);
                    //just passthrough guest param pointers
                    ffi_params[i + external_count + 1 + method_param_count] = &ffi_type_pointer;
                    js_value[i] = tmp;
                    ffi_arg_values[i + method_param_count + external_count + 1] = &js_value[i];
                }
            }
        } else if (optional_count) {
            for (int i = argc; i < method_param_count; i++) {
                auto param = currParam[i];
                FEATURE_CHECK_EQ(FT_IS_COMPLEX(param), true);
                OptionalType* optionalType = (OptionalType*)FT_GET_COMPLEX(param);
                FEATURE_CHECK_EQ(optionalType->header.type, COMPLEX_OPTIONAL);
                if (!createTypeDeclaration(param, ffi_params[external_count + i])) {
                    FEATURE_LOG_ERROR("prepareType for type failed !");
                    got_error = true;
                    break;
                }
                ffi_arg_values[external_count + i] = &optionalType->fval;
            }
        }
        // if (got_error)
        //     break;

        // prepeare return type
        if (!createTypeDeclaration(method.return_type, ffi_ret)) {
            FEATURE_LOG_ERROR("prepareType for complex type failed !");
            got_error = true;
            break;
        }
        // create return value pointer inneed.
        if (!isPromise && method.return_type != FT_VOID) {
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
            ret = ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, method_param_count + external_count, argc + variadic_count + external_count + 1, ffi_ret, ffi_params);
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
        feature_value_t tmp_p;
        //instance->prototype()->ctx =  dyntype_get_context()->js_ctx;
        if (isPromise) {
            ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(method.return_type);
            // create promise
            PromiseType* promiseType = (PromiseType*)complexType;
            // create promise and add to instance
            promiseHandle = ((FeatureInstanceWamr*)instance)->addPromise(promiseType->resolveTypes[0], promiseType->resolveTypes[1]);
            feature_value_t promise = ((FeatureInstanceWamr*)instance)->getPromise(promiseHandle);
            // pass promiseHandle to native function
            ffi_arg_values[2] = &promiseHandle;
            // dup and return promise object.
            //promise_ret_value = feature_dup_value(instance->prototype()->ctx, promiseData->promise);
            feature_value_t tmp_p = feature_dup_value(js_ctx, promise);
            promise_ret_value = dyntype_dup_value(js_ctx, tmp_p);
        }

        // invoke method
        ffi_call(&cif, method.callback, ffi_ret_value, ffi_arg_values);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!isPromise && method.return_type != FT_VOID) {
            //process return value
            if (!FeatureFFIWamr::convertValueToGuest(instance, method.return_type, ffi_ret_value, exec_env, method_ret_value)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                feature_free_value(js_ctx, tmp_p);
                // method_ret_value = FEATURE_EXCEPTION;
                // got_error = true;
            }
            switch (method_ret_value.kind) {
                case WASM_I32:
                {
                    native_raw_return_type(double, tmp_args);
                    native_raw_set_return(method_ret_value.of.i32);
                } break;
                case WASM_ANYREF:
                {
                    native_raw_return_type(void *, tmp_args);
                    wasm_struct_obj_t obj = nullptr;
                    if (FT_IS_PRIMITIVE(method.return_type))
                    {
                        const char *str = (char *)method_ret_value.of.foreign;
                        obj = create_wasm_string(exec_env, str);
                    } 
                    // if return type is array or struct type and is complex
                    else if (FT_IS_COMPLEX(method.return_type))
                    {
                        ComplexTypeHeader *complexType = (ComplexTypeHeader *)FT_GET_COMPLEX(method.return_type);
                        switch (complexType->type)
                        {
                        case COMPLEX_STRUCT_MAP:
                        {
                            // need to do later.
                        }
                        break;
                        case COMPLEX_ARRAY:
                        {
                            FTArray *array = (FTArray *)method_ret_value.of.foreign;
                            uint32_t len = array->_size;
                            obj = create_wasm_array_with_string(exec_env, array->_element, len);
                        }
                        break;
                        }
                    }
                    native_raw_set_return(obj);
                }
                break;
                default:
                    break;
            }
        } else if (isPromise) {
            //把promise返回给ts层
            native_raw_return_type(void*, tmp_args);
            wasm_anyref_obj_t p_obj = wasm_anyref_obj_new(exec_env, promise_ret_value);
            native_raw_set_return(p_obj);
            //feature_free_value(js_ctx, tmp_p);
        }
    } while (0);

    // free ffi call resources
    for (int i = 0; i < method_param_count; i++) {
        // free type
        if (ffi_params[i + external_count]) {
            freeTypeDeclaration(ffi_params[i + external_count]);
        }
        // free value
        if (ffi_arg_values[i + external_count]) {
            FreeFeatureValue(ffi_arg_values[i + external_count]);
        }
    }

    freeTypeDeclaration(ffi_ret);
    // ffi_ret_value will cause "segmentation fault " when reture string to ts
    // if (ffi_ret_value) {
    //     FreeFeatureValue(ffi_ret_value);
    // }
    delete[] ffi_arg_values;
    delete[] ffi_params;
    if (rest_params) {
        delete[] rest_params;
    }

    if(js_value != NULL)
        delete []js_value;

    // if error occurred, throw internal error
    // if (got_error) {
    //     FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native method failed !");
    // }
}

static bool create_feature_prototype(ft_context_ref ft_ctx, wasm_exec_env_t ctx, FeatureUnit* unit)
{
    FEATURE_CHECK_NE(unit, nullptr);
    FEATURE_LOG_DEBUG("unit: %p, description: %p, description->name: %s", unit, unit->description, unit->description->name);
    FEATURE_CHECK_EQ(unit->proto, nullptr);
    unit->proto = new FeaturePrototype(ft_ctx, unit->description);
    unit->proto->wamr_env = ctx;
    return true;
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
        printf("Register libdyntype APIs failed.\n");
        return false;
    }

    symbol_count = get_lib_console_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        printf("Register stdlib APIs failed.\n");
        return false;
    }

    symbol_count = get_lib_array_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        printf("Register stdlib APIs failed.\n");
        return false;
    }

    symbol_count = get_lib_timer_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        printf("Register stdlib APIs failed.\n");
        return false;
    }

    symbol_count = get_struct_indirect_symbols(&module_name, &native_symbols);
    if (!wasm_runtime_register_natives(module_name, native_symbols, symbol_count)) {
        printf("Register struct-dyn APIs failed.\n");
        return false;
    }

    for (const auto& pair : registry_->getRegisteredFeatures()) {
        FeatureUnit* unit = pair.second;
        if (!unit || !unit->description) {
            FEATURE_LOG_WARN("can't find native feature '%s'!", module_name);
            continue;
        }

        if (strcmp(pair.first.data(), "ATest_1_0") != 0) {
            FEATURE_LOG_WARN("other Features are not for wamr!!!");
            continue;
        }

        registerUnit(unit);
    }

    return true;
}

void FeatureManagerWamr::release()
{
    if (ft_ctx_) {
        ReleaseFeatureContextQjs(ft_ctx_);
        ft_ctx_ = nullptr;
    }
}

Member* FeatureManagerWamr::getUnitMember(FeatureUnit*unit, int index)
{
    if (!unit || !unit->description) {
        FEATURE_LOG_WARN("can't find native feature!");
        return nullptr;
    }

    return const_cast<Member*>(&(unit->description->members[index]));
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
    FEATURE_LOG_DEBUG("featureRequire for name: %s", name);
    FeatureUnit* unit = registry_->findFeature(name);
    if (!unit || !unit->description) {
        FEATURE_LOG_WARN("can't find native feature '%s', fallback to original JS module load!", name);
        return false;
    }

    if (!ft_ctx_)
        ft_ctx_ = CreateFeatureContextQjs(dyntype_get_context()->js_ctx);

    if (!unit->proto) {
        // create proto
        if (!create_feature_prototype(ft_ctx_, ctx, unit)) {
            FEATURE_LOG_ERROR("createFeaturePrototype failed !");
            return false;
        }

        // initialize_prototype_wamr(ctx, &unit);
        if (unit->proto->description->native_callbacks->onCreate) {
            FEATURE_LOG_DEBUG("invoke onCreate callback...");
            unit->proto->description->native_callbacks->onCreate(ctx, unit->proto);
        }
    }

    // create feature instance for the required object
    auto featureInstance = std::make_unique<FeatureInstanceWamr>(unit->proto);
    feature_instance_map_[thiz] = featureInstance.get();

    // insert into instances array, update iid
    int iid = unit->proto->addInstance(std::move(featureInstance));
    unit->proto->instances[iid]->setInstanceId(iid);
    if (unit->proto->description->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        unit->proto->description->native_callbacks->onRequired(ctx, unit->proto->instances[iid].get());
    }

    return true;
}

bool FeatureManagerWamr::makeAttachment(NativeSymbol* symbol, FeatureUnit* unit, int index)
{
    if (symbol->attachment)
        return false;

    WarmAttachment attachment = { this, symbol, unit, index};
    symbol_attachment_map_[symbol] = attachment;
    symbol->attachment = &(symbol_attachment_map_[symbol]);
    return true;
}

int FeatureManagerWamr::registerUnit(FeatureUnit* unit)
{
    // 注册class_initNative函数
    NativeSymbol* init_symbol = new NativeSymbol();
    init_symbol->func_ptr = (void*)init_native;
    char* name = new char[128];
    strcpy(name, unit->description->name);
    strcat(name,"_init_native");
    init_symbol->symbol = name;
    init_symbol->signature = "(rr)";
    makeAttachment(init_symbol, unit, -1);

    if (!wasm_runtime_register_natives_raw("env", init_symbol, 1)) {
        printf("register failed !\n");
        return false;
    }

    for (int i = 0; i < unit->description->member_count; i++) {
        const Member* const_p = &(unit->description->members[i]);
        Member* modifier = const_cast<Member*>(const_p);
        Member& member = *modifier;

        switch (member.type) {
            case MEMBER_NULL: {
                // not allowed
                FEATURE_CHECK(false && "invalid member type!");
                break;
            }
            case MEMBER_METHOD: {
                // register different type
                MemberMethod* method = &member.method;
                NativeSymbol* native_symbol = new NativeSymbol();
                native_symbol->func_ptr = (void*)method_call;
                char* name1 = new char[128];
                strcpy(name1, unit->description->name);
                strcat(name1,"_");
                strcat(name1,const_p->name);
                native_symbol->symbol = name1;
                char* param = new char[64];
                memset(param,0,64);
                strcpy(param,"(r");
                FEATURE::FeatureType* pars = (FEATURE::FeatureType*)method->parameters;
                while((*pars)!=0) {
                    if(FT_PARAM_REST_END == *pars) {
                        param[strlen(param)] = 'r';
                        break;
                    }
                    char sig = FeatureFFIWamr::getFeatureSignature(*pars);
                    if (sig!=0) {
                        param[strlen(param)] = sig;
                    }
                    pars += 1;
                }
                strcat(param,")");
                char retc = FeatureFFIWamr::getFeatureSignature(method->return_type);
                if(retc!=0) {
                    param[strlen(param)] = retc;
                }
                printf("param is %s\n",param);
                native_symbol->signature = param;
                makeAttachment(native_symbol, unit, i);
                if (!wasm_runtime_register_natives_raw("env", native_symbol, 1)) {
                    printf("register failed !\n");
                    return false;
                }
                break;
            }
            case MEMBER_ACCESSOR: {
                // register accessor_get and accessor_set
                MemberAccessor *accessor = &member.accessor;
                if (accessor->getter) {
                    NativeSymbol *native_symbol = new NativeSymbol();
                    native_symbol->func_ptr = (void *)accessor_get;
                    char *buf = new char[128];
                    strcpy(buf, unit->description->name);
                    strcat(buf, "_get_");
                    strcat(buf, member.name);
                    strcat(buf, "_0");
                    printf("buf is %s\n", buf);
                    native_symbol->symbol = buf;
                    char *signature = new char[64];
                    memset(signature, 0, 64);
                    strcpy(signature, "(r");
                    char type = FeatureFFIWamr::getFeatureSignature(accessor->type);
                    strcat(signature, ")");
                    if (type != 0)
                        signature[strlen(signature)] = type;
                    printf("signature is %s\n", signature);
                    native_symbol->signature = signature;
                    makeAttachment(native_symbol, unit, i);
                    if (!wasm_runtime_register_natives_raw("env", native_symbol, 1)) {
                        printf("register failed !\n");
                        return false;
                    }
                }
                if (accessor->setter) {
                    NativeSymbol *native_symbol = new NativeSymbol();
                    native_symbol->func_ptr = (void *)accessor_set;
                    char *buf = new char[128];
                    strcpy(buf, unit->description->name);
                    strcat(buf, "_set_");
                    strcat(buf, member.name);
                    strcat(buf, "_0");
                    printf("buf is %s\n", buf);
                    native_symbol->symbol = buf;
                    char *signature = new char[64];
                    memset(signature, 0, 64);
                    strcpy(signature, "(r");
                    char type = FeatureFFIWamr::getFeatureSignature(accessor->type);
                    if (type != 0)
                        signature[strlen(signature)] = type;
                    strcat(signature, ")");
                    printf("signature is %s\n", signature);
                    native_symbol->signature = signature;
                    makeAttachment(native_symbol, unit, i);
                    if (!wasm_runtime_register_natives_raw("env", native_symbol, 1)) {
                        printf("register failed !\n");
                        return false;
                    }
                }
                break;
            }
        }
    }
    return 0;
}
}

