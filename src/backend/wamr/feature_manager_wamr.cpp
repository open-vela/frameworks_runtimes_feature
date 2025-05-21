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
#include "feature_context.h"
#include "feature_context_wamr.h"
#include "feature_exports.h"
// clang-format off
#include "value_translator_wamr.h"
#include "feature_ffi_templates.h"
// clang-format on
#include "feature_instance_wamr.h"
#include "feature_log.h"
#include "feature_prototype_wamr.h"
#include "feature_registry.h"
#include "feature_utils.h"
#include "value_translator_wamr.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#define FEATURE_ENV_NAME "wamr"

typedef struct DynTypeContext {
    JSRuntime* js_rt;
    JSContext* js_ctx;
    JSValue* js_undefined;
    JSValue* js_null;
    dyntype_callback_dispatcher_t cb_dispatcher;
    JSClassID extref_class_id;
    JSValue* extref_class;
} DynTypeContext;

namespace feature_framework {

static inline FeatureManagerWamr* manager_from_instance(FeatureInstance* instance)
{
    return static_cast<FeatureManagerWamr*>(instance->prototype()->featureManager());
}

static inline FeatureInstance* instance_from_target(wasm_obj_t target)
{
    wasm_value_t val = { 0 };
    // every instance class have the first field to hold native instance
    wasm_struct_obj_get_field((wasm_struct_obj_t)target, 1, false, &val);
    return (FeatureInstance*)(val.gc_obj);
}

static inline void set_instance_to_target(wasm_obj_t target, FeatureInstance* instance)
{
    wasm_value_t val = { 0 };
    val.gc_obj = (wasm_obj_t)instance;
    // every instance class have the first field to hold native instance
    wasm_struct_obj_set_field((wasm_struct_obj_t)target, 1, &val);
}

static void module_object_finalizer(wasm_obj_t obj, void* data)
{
    auto instance = (FeatureInstanceWamr*)instance_from_target(obj);
    FEATURE_LOG_INFO("target obj: %p, instance: %p", obj, instance);
    instance->release();
}

static void reportArgsError(FeatureInstance* instance, RetCode ret_code, std::string& message, void* argv, int argc)
{
    FEATURE_CHECK_NE(instance, nullptr);
    if (ret_code == RET_OK) {
        return;
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
        manager->argsErrorCb()(manager->argsErrorData(), &error_info);
        return;
    }
    FEATURE_LOG_ERROR("error: %s", error_info.error_msg);
}

static void init_native(wasm_exec_env_t exec_env, uint64_t* args)
{
    native_raw_get_arg(void*, thiz_ptr, args);
    native_raw_get_arg(void*, str, args);

    wasm_stringref_obj_t str_ref = (wasm_stringref_obj_t)str;

    /* get cstring from wasm string (stringref args) */
    uint32_t str_len = 0;
    if (wasm_obj_is_stringref_obj((wasm_obj_t)str)) {
        str_len = wasm_string_get_length(str_ref);
    }
    char* class_name = str_len > 0 ? (char*)malloc(str_len + 1) : nullptr;
    if (class_name != nullptr) {
        wasm_string_to_cstring(str_ref, class_name, str_len + 1);
    }

    auto manager = (FeatureManagerWamr*)wasm_runtime_get_function_attachment(exec_env);
    manager->require(exec_env, (wasm_obj_t)thiz_ptr, class_name);
    free(class_name);

    // set object destructor func
    wasm_obj_set_gc_finalizer(exec_env, (wasm_obj_t)thiz_ptr, (wasm_obj_finalizer_t)module_object_finalizer, nullptr);
}

static void const_get(wasm_exec_env_t exec_env, uint64_t* args)
{
    uint64_t* ret_ptr = args;
    native_raw_get_arg(void*, thiz_ptr, args); // pop this pointer
    *ret_ptr = 0;

    auto instance = (FeatureInstanceWamr*)instance_from_target((wasm_obj_t)thiz_ptr);
    FEATURE_CHECK_NE(instance, nullptr);
    auto member = (const Member*)wasm_runtime_get_function_attachment(exec_env);

    uint64_t ret_val = 0;
    wasm_local_obj_ref_t* obj_ref_head = wasm_runtime_get_cur_local_obj_ref(exec_env);
    if (constGet(exec_env, member->value, ret_val)) {
        *ret_ptr = ret_val;
    }
    /* pop native createD obj local ref ptr */
    pop_local_obj_ref_to_head(exec_env, obj_ref_head);
}

static void accessor_get(wasm_exec_env_t exec_env, uint64_t* args)
{
    uint64_t* ret_ptr = args;
    native_raw_get_arg(void*, thiz_ptr, args); // pop this pointer
    *ret_ptr = 0;

    auto instance = (FeatureInstanceWamr*)instance_from_target((wasm_obj_t)thiz_ptr);
    FEATURE_CHECK_NE(instance, nullptr);
    auto member = (const Member*)wasm_runtime_get_function_attachment(exec_env);

    uint64_t ret_val = 0;
    wasm_local_obj_ref_t* obj_ref_head = wasm_runtime_get_cur_local_obj_ref(exec_env);
    RetCode ret_code = accessorGet(instance, exec_env, member, ret_val);
    if (ret_code == RET_OK) {
        *ret_ptr = ret_val;
    }
    /* pop native createD obj local ref ptr */
    pop_local_obj_ref_to_head(exec_env, obj_ref_head);
}

static void accessor_set(wasm_exec_env_t exec_env, uint64_t* args)
{
    native_raw_get_arg(void*, thiz_ptr, args); // pop this pointer
    auto instance = (FeatureInstanceWamr*)instance_from_target((wasm_obj_t)thiz_ptr);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    auto member = (const Member*)wasm_runtime_get_function_attachment(exec_env);

    if (!accessorSet(instance, exec_env, member, *args)) {
        FEATURE_LOG_ERROR("accessor set failed!");
    }

    RetCode ret_code = accessorSet(instance, exec_env, member, *args);
    if (ret_code != RET_OK) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", property:" << member->name;
        std::string msg = oss.str();
        reportArgsError(instance, ret_code, msg, (void*)args, 1);
    }
}

static void method_call(wasm_exec_env_t exec_env, uint64_t* args)
{
    uint64_t* ret_ptr = args;
    native_raw_get_arg(void*, thiz_ptr, args);
    *ret_ptr = 0;
    auto instance = (FeatureInstanceWamr*)instance_from_target((wasm_obj_t)thiz_ptr);
    FEATURE_CHECK_NE(instance, nullptr);
    auto manager = manager_from_instance(instance);
    auto description = instance->prototype()->description();
    auto member = (const Member*)wasm_runtime_get_function_attachment(exec_env);
    auto js_ctx = (JSContext*)ft_context_get_data(manager->getFeatureContext());
    auto param_types = member->method->parameters;

    bool has_rest_params = false;
    int opt_argc = 0;
    size_t fixed_argc = getParamCount(param_types, &has_rest_params, &opt_argc);
    size_t argc = fixed_argc;
    FEATURE_CHECK_NE(has_rest_params && opt_argc, true);

    if (has_rest_params) {
        uint64_t* vari_argv = args + fixed_argc;
        native_raw_get_arg(wasm_obj_t, obj, vari_argv);
        assert(wasm_obj_is_struct_obj(obj));
        int32_t vari_argc = get_array_length((wasm_struct_obj_t)obj);
        argc += vari_argc;
    }

    uint64_t ret_val = 0;
    wasm_local_obj_ref_t* obj_ref_head = wasm_runtime_get_cur_local_obj_ref(exec_env);
    RetCode ret_code = methodCall(instance, exec_env, js_ctx, member, argc, args, ret_val);
    /* pop native createD obj local ref ptr */
    pop_local_obj_ref_to_head(exec_env, obj_ref_head);
    if (ret_code != RET_OK) {
        std::ostringstream oss;
        oss << "feature: " << description->name << ", method:" << member->name;
        std::string msg = oss.str();
        reportArgsError(instance, ret_code, msg, (void*)args, argc);
        return;
    }
    *ret_ptr = ret_val;
}

FeatureManager* CreateWamrFeatureManager(FeatureRegistry* registry, FeatureManagerCreateInfo* pinfo)
{
    return new FeatureManagerWamr(registry);
}

FeatureManagerWamr::FeatureManagerWamr(FeatureRegistry* registry)
    : FeatureManager(registry)
    , wamr_env_(nullptr)
{
    ft_context_ref ft_ctx = CreateFeatureContextQjs(dyntype_get_context()->js_ctx);
    setFeatureContext(ft_ctx);
}

bool FeatureManagerWamr::init()
{
    /* Register APIs required by ts2wasm */
    NativeSymbol* native_symbols;
    char* module_name;
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

    registrySetFeatureRegisteredCB([this](const FeatureDescription* description) { return registerFeature(description); });

    return true;
}

void FeatureManagerWamr::uninit()
{
    if (!getFeatureRegistry())
        return;

    JSContext* js_ctx = (JSContext*)ft_context_get_data(getFeatureContext());
    auto release_prototype = [js_ctx](const FeatureDescription* description, FeaturePrototype* proto) {
        FEATURE_CHECK_NE(description, nullptr);
        FEATURE_CHECK_NE(proto, nullptr);
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
    for (auto& feature_pair : getFeaturePrototypes()) {
        const FeatureDescription* pDesc = getFeatureRegistry()->findFeature(feature_pair.first.c_str());
        release_prototype(pDesc, feature_pair.second);
    }
    // uninit registery
    delete getFeatureRegistry();

    /* delete native symbols */
    if (!native_symbols_.empty()) {
        for (size_t i = 0; i < native_symbols_.size(); i++) {
            delete native_symbols_[i];
        }
        native_symbols_.clear();
    }

    /* delete native strings */
    if (!native_strings_.empty()) {
        for (size_t i = 0; i < native_strings_.size(); i++) {
            delete[] native_strings_[i];
        }
        native_strings_.clear();
    }

    if (getFeatureContext()) {
        ReleaseFeatureContextQjs(getFeatureContext());
        setFeatureContext(nullptr);
    }
}

bool FeatureManagerWamr::require(wasm_exec_env_t ctx, wasm_obj_t thiz, const char* name)
{
    FEATURE_LOG_INFO("require feature for name: %s", name);
    const FeatureDescription* pDesc = getFeatureRegistry()->findFeature(name);
    if (!pDesc) {
        FEATURE_LOG_WARN("can't find native feature '%s'!", name);
        return false;
    }

    wamr_env_ = ctx;
    FEATURE_CHECK_NE(getFeatureContext(), nullptr);
    auto& proto = getFeaturePrototypes()[name];

    if (!proto) {
        // create proto
        proto = new FeaturePrototypeWamr(pDesc);
        if (!pDesc->dynamic && pDesc->native_callbacks->onCreate) {
            FEATURE_LOG_DEBUG("invoke onCreate callback...");
            pDesc->native_callbacks->onCreate(ctx, proto);
        }
        proto->setFeatureManager(this);
        setPackageName(getFeatureRegistry()->getFeaturePackageName());
        setEnvName(FEATURE_ENV_NAME);
    }

    // create feature instance for the required object
    FeatureObjectUniquePtr<FeatureInstanceWamr> instance(new FeatureInstanceWamr(proto));
    auto instance_ptr = instance.get();
    set_instance_to_target(thiz, instance_ptr);

    // insert into instances array, update iid
    int iid = proto->addInstance(std::move(instance));
    instance_ptr->setInstanceId(iid);

    // create prototype class instance
    if (pDesc->native_callbacks && pDesc->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        pDesc->native_callbacks->onRequired(ctx, instance_ptr);
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
    FEATURE_LOG_DEBUG("register native symbol, name: %s, signature:%s", name, sig);
    if (!wasm_runtime_register_natives_raw("env", symbol, 1)) {
        FEATURE_LOG_ERROR("register native symbol: '%s' failed !", name);
        delete symbol;
        return false;
    }
    native_symbols_.push_back(symbol);

    return true;
}

static char getFeatureSignature(FeatureType ftype)
{
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (ftype) {
        case FT_VOID:
            return 0;
        case FT_BOOLEAN:
            return 'i';
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
            return 'F';
        case FT_STRING:
            // wasm string signature is 'r'
            return 'r';
        default: {
            FEATURE_LOG_WARN("unsupported type detected !");
            return 0;
        }
        }
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complexType->type) {
        case COMPLEX_OPTIONAL: {
            OptionalType* optionalType = (OptionalType*)complexType;
            char res = getFeatureSignature(optionalType->type);
            if (res == 0) {
                FEATURE_LOG_WARN("unsupported type detected !");
                return 0;
            }
            return res;
        } break;
        default: {
            return 'r';
        }
        }
    }
    return 0;
}

bool FeatureManagerWamr::registerFeature(const FeatureDescription* description)
{
    FEATURE_CHECK_NE(description, nullptr);
    auto name = description->name;
    /* only several specific features can be registered into wamr*/
    if (strcmp(name, "ATest") != 0 && strcmp(name, "Simple") != 0 && strcmp(name, "struct_test") != 0 && strcmp(name, "promise_test") != 0 && strcmp(name, "interface_test") != 0 && strcmp(name, "system.messageChannel") != 0) {
        FEATURE_LOG_WARN("Feature '%s' is not for wamr!", name);
        return true;
    }
    /* register interface api */
    if (!description->dynamic && description->member_count > 0) {
        for (int i = 0; i < description->member_count; i++) {
            const Member& member = description->members[i];
            if (member.type != MEMBER_METHOD)
                continue;
            FeatureType feature_type = member.method->return_type;
            if (!FT_IS_COMPLEX(feature_type))
                continue;
            ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(feature_type);
            if (complex_type->type != COMPLEX_INTERFACE)
                continue;
            InterfaceType* interface_type = (InterfaceType*)complex_type;
            const FeatureDescription* desc = interface_type->desc;
            registerFeature(desc);
        }
    }

    /* register class initNative api */
    std::string class_name(description->name);
    std::replace(class_name.begin(), class_name.end(), '.', '_');
    char* init_name = new char[128];
    native_strings_.push_back(init_name);
    strcpy(init_name, class_name.c_str());
    strcat(init_name, "_init_native");
    if (!registerSymbol((void*)init_native, init_name, "(rr)", this)) {
        return false;
    }

    for (int i = 0; i < description->member_count; i++) {
        const Member* member = &(description->members[i]);
        switch (member->type) {
        case MEMBER_NULL: {
            // not allowed
            FEATURE_CHECK(false, "invalid member type!");
            break;
        }
        case MEMBER_METHOD: {
            // register different type
            auto method = member->method;
            char* method_name = new char[128];
            native_strings_.push_back(method_name);
            strcpy(method_name, class_name.c_str());
            /* special treat for interface */
            if (FT_IS_COMPLEX(method->return_type)) {
                ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(method->return_type);
                (complexType->type == COMPLEX_INTERFACE) ? strcat(method_name, "__") : strcat(method_name, "_");
            } else {
                /* method->return_type is PRIMITIVE TYPE, method_name as before */
                strcat(method_name, "_");
            }

            strcat(method_name, member->name);
            char* signature = new char[64];
            native_strings_.push_back(signature);
            memset(signature, 0, 64);
            strcpy(signature, "(r");
            const FeatureType* ftype = (FeatureType*)method->parameters;
            while ((*ftype) != 0) {
                if (FT_PARAM_REST_END == *ftype) {
                    signature[strlen(signature)] = 'r';
                    break;
                }
                char sig = getFeatureSignature(*ftype);
                if (sig != 0) {
                    signature[strlen(signature)] = sig;
                }
                ftype += 1;
            }
            strcat(signature, ")");
            /* if method->return_type is COMPLEX_INTERFACE, it's means the method is createxxx, and return is feature instance ptr
            there use f64 express it's return type */
            if (FT_IS_COMPLEX(method->return_type)) {
                ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(method->return_type);
                /* special treat for interface */
                if (complexType->type == COMPLEX_INTERFACE) {
                    signature[strlen(signature)] = 'F';
                } else {
                    /* COMPLEX TYPE, such as FTArray, deal with is as brefore */
                    char retc = getFeatureSignature(method->return_type);
                    if (retc != 0)
                        signature[strlen(signature)] = retc;
                }
            } else {
                /* PRIMITIVE TYPE, deal with is as brefore */
                char retc = getFeatureSignature(method->return_type);
                if (retc != 0)
                    signature[strlen(signature)] = retc;
            }
            if (!registerSymbol((void*)method_call, method_name, signature, (void*)(member))) {
                return false;
            }
            break;
        }
        case MEMBER_ACCESSOR: {
            // register accessor_get and accessor_set
            const MemberAccessor* accessor = member->accessor;
            if (accessor->getter_stub) {
                char* getter_name = new char[128];
                native_strings_.push_back(getter_name);
                strcpy(getter_name, class_name.c_str());
                strcat(getter_name, "_get_");
                strcat(getter_name, member->name);
                strcat(getter_name, "_0");
                char* signature = new char[64];
                native_strings_.push_back(signature);
                memset(signature, 0, 64);
                strcpy(signature, "(r");
                char type = getFeatureSignature(accessor->type);
                strcat(signature, ")");
                if (type != 0)
                    signature[strlen(signature)] = type;
                if (!registerSymbol((void*)accessor_get, getter_name, signature, (void*)(member))) {
                    return false;
                }
            }
            if (accessor->setter_stub) {
                char* setter_name = new char[128];
                native_strings_.push_back(setter_name);
                strcpy(setter_name, class_name.c_str());
                strcat(setter_name, "_set_");
                strcat(setter_name, member->name);
                strcat(setter_name, "_0");
                char* signature = new char[64];
                native_strings_.push_back(signature);
                memset(signature, 0, 64);
                strcpy(signature, "(r");
                char sig = getFeatureSignature(accessor->type);
                if (sig != 0)
                    signature[strlen(signature)] = sig;
                strcat(signature, ")");
                if (!registerSymbol((void*)accessor_set, setter_name, signature, (void*)(member))) {
                    return false;
                }
            }
            break;
        }
        case MEMBER_CONST: {
            // handle member const
            const MemberConst* member_const = member->value;
            char* const_name = new char[128];
            native_strings_.push_back(const_name);
            strcpy(const_name, class_name.c_str());
            strcat(const_name, "_const_");
            strcat(const_name, member->name);
            char* signature = new char[64];
            native_strings_.push_back(signature);
            memset(signature, 0, 64);
            strcpy(signature, "(r");
            char sig = getFeatureSignature(member_const->type);
            strcat(signature, ")");
            if (sig != 0)
                signature[strlen(signature)] = sig;
            if (!registerSymbol((void*)const_get, const_name, signature, (void*)(member))) {
                return false;
            }
            break;
        }
        case MEMBER_EVENT: {
            // not supported for wamr now
        } break;
        }
    }
    return true;
}
}
