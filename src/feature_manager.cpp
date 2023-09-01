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

#include "feature_manager.h"
#include "feature_registry.h"
#include "feature_ffi.h"
#include "feature_framework.h"
#include "feature_instance_qjs.h"
#include "feature_context_qjs.h"
#include "feature_log.h"
#include "feature_utils.h"
#if defined(CONFIG_QUICKAPP)
#include "aiotjs.h"
#endif
#include "feature.h"
#include <assert.h>
#include <ffi.h>
#include <memory>
#include <rapidjson/error/en.h>
#include <string.h>
#include <string>
#include <vector>

using namespace FEATURE;
#if defined(CONFIG_QUICKAPP)
using namespace AIOTJS;
#endif
namespace ferry {

static thread_local feature_classid_t class_id; // prototype class id
static thread_local feature_classdef_t class_def; // prototype class defination, contains finalizer

// some static functions used by FeatureManager
static bool createFeaturePrototype(context_ref ctx, FeatureUnit* unit);
static bool createJsInstanceClass(context_ref ctx, const char* class_name);
static context_ref getContext(feature_runtime_ref rt);

FeatureManager::FeatureManager(FeatureRegistry* registry)
    : registry_(registry)
{
}

FeatureInstance* getInstance(feature_value_t val)
{
    auto instance = static_cast<FeatureInstance*>(feature_get_opaque(val, class_id));
    return instance;
}

static context_ref getContext(feature_runtime_ref rt)
{
#if defined(CONFIG_QUICKAPP)
//how to get context in nuttx? need check yaozong
    auto qrt = static_cast<AIOTJS::RuntimeContext*>(JS_GetRuntimeOpaque(rt));
    FEATURE_CHECK_NE(qrt, nullptr);
    return qrt->env.ctx;
#else
    auto ctx = static_cast<context_ref>(JS_GetRuntimeOpaque(rt));
    return ctx;
#endif
}

static void __feature_finalizer(feature_runtime_ref rt, feature_value_t val)
{
    auto instance = getInstance(val);
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_INFO("instance or prototype is null...");
        return;
    }
    auto proto = instance->prototype();

    feature_set_opaque(val, nullptr);
    // get proto pointer, it may not be deleted at this time

    //遍历proto->weak_ref_list链表，将其中的js_value设置为JSE_UNDEFINED
    WeakRef* node;
    WeakRef* node_temp;
    weakref_list_for_every_entry_safe(&proto->weak_ref_list, node, node_temp, WeakRef, link)
    {
        node->js_value = FEATURE_VALUE_UNDEFINED;
    }

    auto iid = instance->instanceId();
    // invoke callback
    if (proto->description->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto->description->native_callbacks->onDetached(getContext(rt), instance);
    }
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

static void __feature_mark(feature_runtime_ref rt, feature_value_t val, feature_mark_func mark_func)
{
    // TODO: for now，FeatureInstance saves callbacks only, mark it directly.
    // but it maybe insufficient, instance may manage other resource type, change it according to implementation.
    FeatureInstance* instance = getInstance(val);
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_INFO("instance or prototype is null...");
        return;
    }

    auto proto = instance->prototype();
    // mark callbacks
    for (auto& pair : instance->callbacks) {
        feature_mark_value(rt, pair.second.cb, mark_func);
    }
    // mark promies
    for(auto& pair : instance->promises) {
        feature_mark_value(rt, pair.second->promise, mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[0], mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[1], mark_func);
    }
    // should mark prototype object.
    auto js_proto = FT_VAL_GET_JS_VAL(proto->ft_proto);
    feature_mark_value(rt, js_proto, mark_func);
}

static bool createJsInstanceClass(context_ref ctx, const char* class_name)
{
    FEATURE_CHECK_NE(class_name, nullptr);
    FEATURE_LOG_DEBUG("create PrototypeClass: %s.", class_name);
    // FEATURE_CHECK_EQ(featurePrototype->class_id, 0);
    JS_NewClassID(&class_id);
    FEATURE_CHECK_NE(class_id, 0); // it must not 0 now
    // fill the class_def structure
    class_def = { .class_name = class_name, .finalizer = __feature_finalizer, .gc_mark = __feature_mark };
    // create native feature prototype class defination
    JS_NewClass(feature_get_runtime(static_cast<feature_context_ref>(ctx)), class_id, &class_def);
    return true;
}

static FeaturePrototype* createFeaturePrototype(context_ref ctx, FeatureDescription* description)
{
    FEATURE_CHECK_NE(description, nullptr);
    FEATURE_LOG_DEBUG("create FeaturePrototype for description: %s.", description->name);
    if (!createJsInstanceClass(ctx, "FeatureInstanceObject")) {
        FEATURE_LOG_ERROR("create js Instance class for feature %s.", description->name);
        return nullptr;
    }
    auto ft_ctx = CreateFeatureContext(ctx);
    return new FeaturePrototype(ft_ctx, description);
}

static ferry::FeaturePromiseData* FeatureCreatePromise(FeatureInstanceHandle handle, FeatureType resolve_type, FeatureType reject_type)
{
    ferry::FeaturePromiseData* data = (ferry::FeaturePromiseData*)malloc(sizeof(ferry::FeaturePromiseData));

    data->promise = FEATURE_VALUE_UNDEFINED;
    data->resolveFuncs[0] = FEATURE_VALUE_UNDEFINED;
    data->resolveFuncs[1] = FEATURE_VALUE_UNDEFINED;
    data->resolveTypes[0] = resolve_type;
    data->resolveTypes[1] = reject_type;

    auto ft_ctx = GetFeatureContext(handle);
    JSContext* js_ctx = (JSContext*)ft_context_get_data(ft_ctx);
    feature_value_t promise = feature_promise_capability(js_ctx, data->resolveFuncs);
    if (feature_is_exception(promise)) {
        feature_free_value(js_ctx, data->resolveFuncs[0]);
        feature_free_value(js_ctx, data->resolveFuncs[1]);
        feature_free_value(js_ctx, promise);
        free(data);
        return nullptr;
    }
    data->promise = promise;
    return data;
}

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
    feature_value_t method_ret_value = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = ferry::getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->prototype()->description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    const auto& method = member->method;
    auto currParam = method.parameters;
    FEATURE::FeaturePromiseHandle promiseHandle = -1;
    //feature_value_t promise_obj = FEATURE_VALUE_UNDEFINED;
    // count size
    bool has_rest_param = false;
    int optional_count = 0;
    int method_param_count = getParamCount(currParam, &has_rest_param, &optional_count);
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_param && optional_count, true);
    // variadic parameters type
    ffi_type variadicParameters_type;
    ffi_type* variadicParameters_type_element[3];
    // variadic parameter param
    FtVariadicParameters variadicParameters;
    qjs_val_t* qjs_val_array = nullptr;
    memset(&variadicParameters, 0, sizeof(variadicParameters));
    // check argument count match.
    // FEATURE_LOG_DEBUG("required param count: %d, received param count: %d", method_param_count, argc);
    // beacuse we support rest parameters, so argc is greater or equal to method_param_count.
    if (has_rest_param) {
        FEATURE_CHECK_GE(argc, method_param_count);
        variadicParameters.variadic_count = argc - method_param_count;
    } else if (optional_count) {
        // for optional parameters, argc + optional must grater or equal to method_param_count
        FEATURE_CHECK_GE(argc + optional_count, method_param_count);
    } else {
        // for method which do not have rest or optional parameters, argc equals to method_param_count.
        FEATURE_CHECK_EQ(argc, method_param_count);
    }
    // if has rest parameter, we will pack all variadic parameters together as a param pack
    // use packed_argc instead of argc for ffi call.
    int32_t packed_argc = has_rest_param ? argc - variadicParameters.variadic_count + 1 : argc;
    // if return value is a promise
    bool isPromise = FT_IS_PROMISE(method.return_type);
    int external_count = isPromise ? 3 : 2;
    ffi_type** ffi_params = new ffi_type*[packed_argc + optional_count + external_count + 1]; // FeaturInstance, data, maybe return promise, empty placeholder
    ffi_type* ffi_ret = nullptr;
    memset(ffi_params, 0, sizeof(ffi_type*) * (packed_argc + optional_count + external_count + 1));

    void** ffi_arg_values = new void*[packed_argc + optional_count + external_count]; // FeaturInstance, data, maybe return promise
    memset(ffi_arg_values, 0, sizeof(void*) * (packed_argc + optional_count + external_count));
    void* ffi_ret_value = nullptr;

    // prepare first two param
    ffi_params[0] = &ffi_type_pointer; // FeatureContext
    ffi_arg_values[0] = &instance;
    ffi_params[1] = &ffi_type_sint64; // data
    ffi_arg_values[1] = (void*)&method.data;
    if (isPromise) {
        ffi_params[2] = &ffi_type_sint32;
    }

    do {
        for (int i = 0; i < method_param_count && i < argc; i++) {
            feature_value_t currArg = argv[i];
            auto param = currParam[i];
            if (FT_IS_PROMISE(param)) {
                FEATURE_LOG_ERROR("do not support promise as input param !");
                got_error = true;
                break;
            }
            if (!ferry::FeatureFFI::createTypeDeclaration(param, ffi_params[external_count + i])) {
                FEATURE_LOG_ERROR("prepareType for type failed !");
                got_error = true;
                break;
            }
            if (!ferry::FeatureFFI::convertValueToHost(instance, param, ffi_arg_values[external_count + i], ctx, currArg)) {
                FEATURE_LOG_ERROR("convert argument %d failed !", i);
                got_error = true;
                break;
            }
        }
        if (got_error)
            break;

        // process rest parameters
        if (has_rest_param) {
            // prepare variadicParameters type
            variadicParameters_type.size = 0;
            variadicParameters_type.type = FFI_TYPE_STRUCT;
            variadicParameters_type.elements = variadicParameters_type_element;
            variadicParameters_type_element[0] = &ffi_type_sint32;
            variadicParameters_type_element[1] = &ffi_type_pointer;
            variadicParameters_type_element[2] = nullptr;
            // prepare variadicParameters struct
            variadicParameters.variadic_args = new ft_value_t[variadicParameters.variadic_count];
            qjs_val_array = new qjs_val_t[variadicParameters.variadic_count];
            // pass param
            ffi_params[method_param_count + external_count] = &variadicParameters_type;
            ffi_arg_values[method_param_count + external_count] = &variadicParameters;
            for (int i = 0; i + method_param_count < argc; i++) {
                // just passthrough guest param pointers
                qjs_val_array [i].js_val = argv[i + method_param_count];
                variadicParameters.variadic_args[i] = *((ft_value_t*)(qjs_val_array + i));
            }
        } else if (optional_count) {
            for (int i = argc; i < method_param_count; i++) {
                auto param = currParam[i];
                FEATURE_CHECK_EQ(FT_IS_COMPLEX(param), true);
                OptionalType* optionalType = (OptionalType*)FT_GET_COMPLEX(param);
                FEATURE_CHECK_EQ(optionalType->header.type, COMPLEX_OPTIONAL);
                if (!ferry::FeatureFFI::createTypeDeclaration(param, ffi_params[external_count + i])) {
                    FEATURE_LOG_ERROR("prepareType for type failed !");
                    got_error = true;
                    break;
                }
                ffi_arg_values[external_count + i] = &optionalType->fval;
            }
        }

        if (got_error)
            break;

        // prepeare return type
        if (!ferry::FeatureFFI::createTypeDeclaration(method.return_type, ffi_ret)) {
            FEATURE_LOG_ERROR("prepareType for complex type failed !");
            got_error = true;
            break;
        }
        // create return value pointer inneed.
        if (!isPromise && method.return_type != FT_VOID) {
            if (!ferry::FeatureFFI::createHostValue(method.return_type, ffi_ret_value, true)) {
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
            ret = ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, method_param_count + external_count, packed_argc + external_count, ffi_ret, ffi_params);
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
        if (isPromise) {
            ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(method.return_type);
            // create promise
            PromiseType* promiseType = (PromiseType*)complexType;
            // create promise and add to instance
            auto promiseData = FeatureCreatePromise(instance, promiseType->resolveTypes[0], promiseType->resolveTypes[1]);
            FEATURE_CHECK_NE(promiseData, nullptr);
            promiseHandle = instance->addPromise(promiseData);
            // pass promiseHandle to native function
            ffi_arg_values[2] = &promiseHandle;
            // dup and return promise object.
            method_ret_value = feature_dup_value(ctx, promiseData->promise);
        }
        // invoke method
        ffi_call(&cif, method.callback, ffi_ret_value, ffi_arg_values);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!isPromise && method.return_type != FT_VOID) {
            // process return value
            if (!ferry::FeatureFFI::convertValueToGuest(instance, method.return_type, ffi_ret_value, ctx, method_ret_value)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                feature_free_value(ctx, method_ret_value);
                method_ret_value = FEATURE_EXCEPTION;
                got_error = true;
            }
        }
    } while (0);

    // free ffi call resources
    for (int i = 0; i < method_param_count; i++) {
        // free type
        if (ffi_params[i + external_count]) {
            ferry::FeatureFFI::freeTypeDeclaration(ffi_params[i + external_count]);
        }
        // free value
        if (ffi_arg_values[i + external_count]) {
            FreeFeatureValue(ffi_arg_values[i + external_count]);
        }
    }
    ferry::FeatureFFI::freeTypeDeclaration(ffi_ret);
    if (ffi_ret_value) {
        FreeFeatureValue(ffi_ret_value);
    }
    delete[] ffi_arg_values;
    delete[] ffi_params;
    if (variadicParameters.variadic_args) {
        delete[] variadicParameters.variadic_args;
    }
    if (qjs_val_array) {
        delete[] qjs_val_array;
    }

    // if error occurred, throw internal error
    if (got_error) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native method failed !");
    }

    return method_ret_value;
}

static feature_value_t accessor_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    // get info from this_val
    feature_value_t method_ret_value = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = ferry::getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->prototype()->description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = &member->accessor;
    FEATURE_CHECK_NE(accessor->type, FT_VOID);
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type* ffi_params[2] = { &ffi_type_pointer, &ffi_type_sint64 };
    ffi_type* ffi_ret = nullptr;
    void* arg_values[2] = { &instance, &accessor->data };
    void* ret_value = nullptr;
    do {
        if (!ferry::FeatureFFI::createTypeDeclaration(accessor->type, ffi_ret)) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        if (!ferry::FeatureFFI::createHostValue(accessor->type, ret_value, true)) {
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
        if (!ferry::FeatureFFI::convertValueToGuest(instance, accessor->type, ret_value, ctx, method_ret_value)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            feature_free_value(ctx, method_ret_value);
            method_ret_value = FEATURE_EXCEPTION;
        }
    } while (0);
    // free resources
    ferry::FeatureFFI::freeTypeDeclaration(ffi_ret);
    FreeFeatureValue(ret_value);

    return method_ret_value;
}

static feature_value_t accessor_set(feature_context_ref ctx, feature_value_t this_val, feature_value_t val, int magic)
{
    // get info from this_val
    int index = magic;
    FeatureInstance* instance = ferry::getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->prototype()->description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = &member->accessor;
    FEATURE_CHECK_NE(accessor->type, FT_VOID);
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type* ffi_params[3] = { &ffi_type_pointer, &ffi_type_sint64, nullptr };
    void* arg_value_input = nullptr;
    void* arg_values[3] = { &instance, &accessor->data, nullptr };
    do {
        // prepare third param type declaration, create by accessor type
        if (!ferry::FeatureFFI::createTypeDeclaration(accessor->type, ffi_params[2])) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        // fill third param using guest value and accesor type
        if (!ferry::FeatureFFI::convertValueToHost(instance, accessor->type, arg_value_input, ctx, val)) {
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
    ferry::FeatureFFI::freeTypeDeclaration(ffi_params[2]);
    FreeFeatureValue(arg_value_input);
    return FEATURE_VALUE_UNDEFINED;
}

static feature_value_t const_variable_initialize(context_ref ctx, FeaturePrototype* prototype, const MemberConst& memberConst)
{
    feature_value_t val = FEATURE_VALUE_UNDEFINED;
    FEATURE_CHECK_NE(memberConst.type, FT_VOID);
    // invoke callback to get constant value
    if (memberConst.callback) {
        // create type using featureType description
        ffi_type* ret_type = nullptr;
        void* ret_value = nullptr;
        ffi_type* param_types[2] = { &ffi_type_pointer, &ffi_type_sint64 };
        void* arg_values[2] = { &prototype, (void*)&memberConst.data };
        if (!ferry::FeatureFFI::createTypeDeclaration(memberConst.type, ret_type)) {
            FEATURE_LOG_ERROR("create type failed !");
            FeatureFFI::freeTypeDeclaration(ret_type);
            return val;
        }
        if (!ferry::FeatureFFI::createHostValue(memberConst.type, ret_value, true)) {
            FEATURE_LOG_ERROR("create return value failed !");
            FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(ret_value);
            return val;
        }
        // prepare and call
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, ret_type, param_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(ret_value);
            return val;
        }
        // invoke
        ffi_call(&cif, memberConst.callback, ret_value, arg_values);
        // process return value
        if (!ferry::FeatureFFI::convertValueToGuest(nullptr, memberConst.type, ret_value, ctx, val)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            feature_free_value(ctx, val);
            val = FEATURE_VALUE_UNDEFINED;
        }
        FeatureFFI::freeTypeDeclaration(ret_type);
        FreeFeatureValue(ret_value);
        if (FT_IS_REFERENCE(ret_value)) {
            free(ret_value);
        }
    } else {
        // check type
        if (!ferry::FeatureFFI::convertValueToGuest(nullptr, memberConst.type, (void*)&memberConst.data, ctx, val)) {
            FEATURE_LOG_ERROR("can not convert const value to guest!");
            feature_free_value(ctx, val);
            val = FEATURE_VALUE_UNDEFINED;
        }
    }
    return val;
}

int initialize_prototype(context_ref ctx, FeatureUnit* unit, feature_value_t proto)
{
    assert(unit != nullptr);
    for (int i = 0; i < unit->description->member_count; i++) {
        const Member& member = unit->description->members[i];
        switch (member.type) {
        case MEMBER_NULL: {
            // not allowed
            FEATURE_CHECK(false && "invalid member type!");
        } break;
        case MEMBER_METHOD: {
            // register different type
            const MemberMethod& method = member.method;
            feature_value_t methodCallObj = JS_NewCFunctionMagic(static_cast<feature_context_ref>(ctx), method_call, member.name, getParamCount(method.parameters), JS_CFUNC_generic_magic, i);
            feature_define_object_property(ctx, proto, member.name, methodCallObj, FEATURE_PROP_ENUMERABLE);
        } break;
        case MEMBER_ACCESSOR: {
            // create getter and setter
            const MemberAccessor& accessor = member.accessor;
            feature_atom_t prop_name = feature_atom(static_cast<feature_context_ref>(ctx), member.name);
            feature_value_t funcs[2] = { FEATURE_VALUE_UNDEFINED, FEATURE_VALUE_UNDEFINED };

            char buf[128];
            JSCFunctionType type;
            if (accessor.getter) {
                type.getter_magic = accessor_get;
                sprintf(buf, "get %s", member.name);
                funcs[0] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 0, JS_CFUNC_getter_magic, i);
            }
            if (accessor.setter) {
                type.setter_magic = accessor_set;
                sprintf(buf, "set %s", member.name);
                funcs[1] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 1, JS_CFUNC_setter_magic, i);
            }
            JS_DefinePropertyGetSet(static_cast<feature_context_ref>(ctx), proto, prop_name, funcs[0], funcs[1], FEATURE_PROP_CONFIGURABLE);
            feature_free_atom(static_cast<feature_context_ref>(ctx), prop_name);
        } break;
        case MEMBER_CONST: {
            // handle member const, maybe initialized or directly constantant
            const MemberConst& constMember = member.value;
            feature_value_t constantVal = const_variable_initialize(ctx, unit->proto, constMember);
            feature_define_object_property(ctx, proto, member.name, constantVal, FEATURE_PROP_ENUMERABLE);

        } break;
        }
    }
    return 0;
}

bool WeakRefInit(context_ref js_ctx, feature_value_t feature_object)
{
    // 根据cid获取FeaturePrototype
    FeatureInstance* instance = getInstance(feature_object);
    if (instance == nullptr) {
        FEATURE_LOG_ERROR("WeakRefInit() get FeatureInstance failed");
        return false;
    }
    auto proto = instance->prototype();

    // 创建WeakRef节点添加到proto->weak_ref_list链表中
    WeakRef* node = &instance->weak_self_;
    weakref_list_initialize(&node->link);
    weakref_list_add_tail(&node->link, &proto->weak_ref_list);

    node->js_value = feature_object;
    proto->weak_ref_count++;

    return true;
}

bool WeakRefFree(context_ref js_ctx, feature_value_t feature_object)
{
    // 获取feature_object的cid
    int ret = -1;
    FeatureInstance* instance = getInstance(feature_object);
    if (instance == nullptr) {
        FEATURE_LOG_ERROR("WeakRefFree() get FeatureInstance failed");
        return false;
    }
    auto proto = instance->prototype();

    // 遍历proto->weak_ref_list链表，将其中所有js_value为feature_object的节点删除
    WeakRef* node;
    WeakRef* node_temp;
    weakref_list_for_every_entry_safe(&proto->weak_ref_list, node, node_temp, WeakRef, link)
    {
        ret = feature_is_same_value(static_cast<feature_context_ref>(js_ctx), node->js_value, feature_object);
        if (ret == 1) {
            weakref_list_delete(&node->link);
            proto->weak_ref_count--;
        }
    }

    return true;
}

feature_value_t FeatureManager::featureRequire(context_ref ctx, const char* name)
{
    FEATURE_LOG_DEBUG("featureRequire for '%s'", name);
    FeatureUnit* unit = registry_->findFeature(name);
    if (!unit || !unit->description) {
        FEATURE_LOG_WARN("can't find native feature '%s', fallback to original JS module load!", name);
        return FEATURE_VALUE_UNDEFINED;
    }

    if (!unit->proto) {
        // create proto
        unit->proto = createFeaturePrototype(ctx, unit->description);
        if (!unit->proto) {
            FEATURE_LOG_ERROR("createFeaturePrototype failed !");
            return JS_UNDEFINED;
        }
        auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(unit->proto->ft_proto);
        *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
        required_features_.push_back(name);
    }

    auto proto = unit->proto;
    // create prototype class instance
    auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(proto->ft_proto);
    if (feature_is_undefined(*js_proto_ptr)) {
        feature_value_t js_proto_obj = feature_object(static_cast<feature_context_ref>(ctx));
        if (feature_is_exception(js_proto_obj)) {
            feature_dump_error(static_cast<feature_context_ref>(ctx));
            return FEATURE_VALUE_UNDEFINED;
        }
        // TODO: initialize js_proto using description
        initialize_prototype(ctx, unit, js_proto_obj);
        *js_proto_ptr = js_proto_obj;
        if (proto->description->native_callbacks->onCreate) {
            FEATURE_LOG_DEBUG("invoke onCreate callback...");
            proto->description->native_callbacks->onCreate(ctx, proto);
        }
    }

    // create feature instance for the required object
    auto featureInstance = std::make_unique<FeatureInstanceQjs>(proto);
    // create object with proto and set opaque refers to FeatureInstance
    feature_value_t feature_object = JS_NewObjectProtoClass(static_cast<feature_context_ref>(ctx), *js_proto_ptr, class_id);
    feature_set_opaque(feature_object, featureInstance.get());

    // setup featureInstance WeakRef, refers to feature_object
    // proto->instances[iid]->js_self = WreakRef(feature_object);

    WeakRefInit(ctx, feature_object);
    // insert into instances array, update iid
    int iid = proto->addInstance(std::move(featureInstance));
    proto->instances[iid]->setInstanceId(iid);
    if (proto->description->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        proto->description->native_callbacks->onRequired(ctx, proto->instances[iid].get());
    }

    return feature_object;
}

void FeatureManager::featureRelease()
{
    for (int i = 0; i < required_features_.size(); ++i) {
        FeatureUnit* unit = registry_->findFeature(required_features_[i].data());
        if (!unit)
            continue;

        auto proto = unit->proto;
        auto description = unit->description;
        JSContext* js_ctx = (JSContext*)ft_context_get_data(proto->ft_ctx);
        // call onDestroy
        if (description->native_callbacks->onDestroy) {
            FEATURE_LOG_DEBUG("invoke onDestroy callback...");
            description->native_callbacks->onDestroy(js_ctx, proto);
        }
        auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(proto->ft_proto);
        if (!feature_is_undefined(*js_proto_ptr)) {
            feature_free_value(js_ctx, *js_proto_ptr);
            *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
        }
        ReleaseFeatureContext(proto->ft_ctx);
    }
}

}
