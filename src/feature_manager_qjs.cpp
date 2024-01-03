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

#include "feature_manager_qjs.h"
#include "feature.h"
#include "feature_context.h"
#include "feature_context_qjs.h"
#include "value_translator_qjs.h"
#include "feature_ffi_template.h"
#include "feature_ffi_qjs.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_registry.h"
#include "feature_utils.h"
#include "feature_prototype_qjs.h"

#include <assert.h>
#include <ffi.h>
#include <memory>
#include <rapidjson/error/en.h>
#include <string.h>
#include <string>
#include <vector>

#define FEATURE_ENV_NAME "quickjs"

namespace ferry {

static inline FeatureInstance* getInstance(feature_value_t val)
{
    auto class_id = FeatureManagerQjs::jsClassId();
    if (class_id == 0)
        return NULL;

    void* ptr = feature_get_opaque(val, class_id);
    return static_cast<FeatureInstance*>(ptr);
}

static void __feature_finalizer(feature_runtime_ref rt, feature_value_t val)
{
    auto instance = getInstance(val);
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_INFO("instance or prototype is null, skip resource free...");
        return;
    }
    auto proto = instance->prototype();

    feature_set_opaque(val, nullptr);
    // get proto pointer, it may not be deleted at this time
    auto iid = instance->instanceId();
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

static inline FeaturePrototypeQjs* getPrototype(FeatureInstance* instance)
{
    return static_cast<FeaturePrototypeQjs*>(instance->prototype());
}

static void __feature_mark(feature_runtime_ref rt, feature_value_t val, feature_mark_func mark_func)
{
    FeatureInstance* instance = getInstance(val);
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_INFO("instance or prototype is null, skip mark it ...");
        return;
    }

    auto proto = getPrototype(instance);
    FeatureInstanceQjs* instance_qjs = (FeatureInstanceQjs*)instance;
    // mark all instance values
    instance_qjs->markValues(rt, mark_func);

    // should mark feature prototype object.
    auto js_proto = FT_VAL_GET_JS_VAL(proto->ft_proto());
    feature_mark_value(rt, js_proto, mark_func);
}

#if 0
static feature_value_t new_method_call(feature_context_ref ctx, feature_value_t this_val,
    int argc, feature_value_t* argv, int magic)
{
    int index = magic;
    FeatureInstanceQjs* instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    Member* member = const_cast<Member*>(&description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    bool ret = methodCall(instance, ctx, ctx, member, argc, argv, ret_val);
    if (!ret) {
        ret_val = FEATURE_EXCEPTION;
    }
    return ret_val;
}

static feature_value_t new_accessor_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    int index = magic;
    FeatureInstanceQjs* instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    Member* member = const_cast<Member*>(&description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    bool ret = accessorGet(instance, ctx, member, ret_val);
    if (!ret) {
        ret_val = FEATURE_EXCEPTION;
    }
    return ret_val;
}

static feature_value_t new_accessor_set(feature_context_ref ctx, feature_value_t this_val, feature_value_t val, int magic)
{
    int index = magic;
    FeatureInstanceQjs* instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    Member* member = const_cast<Member*>(&description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    bool ret = accessorSet(instance, ctx, member, val);
    if (!ret) {
        ret_val = FEATURE_EXCEPTION;
    }
    return ret_val;
}

static feature_value_t new_const_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    int index = magic;
    FeatureInstanceQjs* instance = (FeatureInstanceQjs*)getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    Member* member = const_cast<Member*>(&description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_CONST);
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    bool ret = constGet(instance, ctx, member, ret_val);
    if (!ret) {
        ret_val = FEATURE_EXCEPTION;
    }

    // for const value, redefine the property with result value
    JS_DefinePropertyValueStr(static_cast<feature_context_ref>(ctx), this_val, member->name,
        feature_dup_value(ctx, ret_val), FEATURE_PROP_CONFIGURABLE);
    return ret_val;
}
#endif

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
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    auto description = instance->prototype()->description();
    Member* member = const_cast<Member*>(&description->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    const auto& method = member->method;
    auto param_types = method.parameters;
    FtPromiseId pid = -1;
    // feature_value_t promise_obj = FEATURE_VALUE_UNDEFINED;
    //  count size
    bool has_rest_param = false;
    int optional_argc = 0;
    int fixed_argc = getParamCount(param_types, &has_rest_param, &optional_argc);
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_param && optional_argc, true);
    // variadic parameters type
    ffi_type vari_args_type;
    ffi_type* vari_args_elem_types[3];
    // variadic parameter param
    FtVariParams vari_params;
    memset(&vari_params, 0, sizeof(vari_params));
    // check argument count match.
    // FEATURE_LOG_DEBUG("required param count: %d, received param count: %d", fixed_argc, argc);
    // beacuse we support rest parameters, so argc is greater or equal to fixed_argc.
    if (has_rest_param) {
        if (argc < fixed_argc) {
            FEATURE_LOG_ERROR("rest args error, fixed: %d, total: %d!", fixed_argc, argc);
        }
        FEATURE_CHECK_GE(argc, fixed_argc);
        vari_params.vari_count = argc - fixed_argc;
    } else if (optional_argc) {
        // for optional parameters, argc + optional must grater or equal to fixed_argc
        if (argc + optional_argc < fixed_argc) {
            FEATURE_LOG_ERROR("optional args error, optional: %d, fixed: %d, total: %d!",
                optional_argc, fixed_argc, argc);
        }
        FEATURE_CHECK_GE(argc + optional_argc, fixed_argc);
    } else {
        // for method which do not have rest or optional parameters, argc equals to fixed_argc.
        if (argc != fixed_argc) {
            FEATURE_LOG_ERROR("fixed args error, fixed: %d, total: %d!", fixed_argc, argc);
        }
        FEATURE_CHECK_EQ(argc, fixed_argc);
    }
    // if has rest parameter, we will pack all variadic parameters together as a param pack
    // use packed_argc instead of argc for ffi call.
    int32_t packed_argc = has_rest_param ? argc - vari_params.vari_count + 1 : argc;
    // if return value is a promise
    bool is_promise = FT_IS_PROMISE(method.return_type);
    int extra_argc = is_promise ? 3 : 2;
    ffi_type** ffi_arg_types = new ffi_type*[packed_argc + optional_argc + extra_argc + 1]; // FeaturInstance, data, maybe return promise, empty placeholder
    memset(ffi_arg_types, 0, sizeof(ffi_type*) * (packed_argc + optional_argc + extra_argc + 1));
    void** ffi_arg_values = new void*[packed_argc + optional_argc + extra_argc]; // FeaturInstance, data, maybe return promise
    memset(ffi_arg_values, 0, sizeof(void*) * (packed_argc + optional_argc + extra_argc));
    ffi_type* ffi_ret_type = nullptr;
    void* ffi_ret_value = nullptr;

    // prepare first two param
    ffi_arg_types[0] = &ffi_type_pointer; // FeatureContext
    ffi_arg_types[1] = &ffi_type_sint64; // data
    if (is_promise) {
        ffi_arg_types[2] = &ffi_type_sint32;
    }
    ffi_arg_values[0] = &instance;
    ffi_arg_values[1] = (void*)&method.data;

    do {
        for (int i = 0; i < fixed_argc && i < argc; i++) {
            feature_value_t currArg = argv[i];
            auto param_type = param_types[i];
            if (FT_IS_PROMISE(param_type)) {
                FEATURE_LOG_ERROR("do not support promise as input param !");
                got_error = true;
                break;
            }
            if (!createTypeDeclaration(param_type, ffi_arg_types[extra_argc + i])) {
                FEATURE_LOG_ERROR("prepareType for type failed !");
                got_error = true;
                break;
            }
            if (!FeatureFFIQjs::convertValueToHost(instance, param_type, ffi_arg_values[extra_argc + i], ctx, currArg)) {
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
            vari_args_type.size = 0;
            vari_args_type.type = FFI_TYPE_STRUCT;
            vari_args_type.elements = vari_args_elem_types;
            vari_args_elem_types[0] = &ffi_type_sint32;
            vari_args_elem_types[1] = &ffi_type_pointer;
            vari_args_elem_types[2] = nullptr;
            // prepare vari_params struct
            vari_params.vari_args = new ft_value_t[vari_params.vari_count];
            // pass param
            ffi_arg_types[fixed_argc + extra_argc] = &vari_args_type;
            ffi_arg_values[fixed_argc + extra_argc] = &vari_params;
            for (int i = 0; i + fixed_argc < argc; i++) {
                // just passthrough guest param pointers
                auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(vari_params.vari_args[i]);
                *js_val_ptr = argv[i + fixed_argc];
            }
        } else if (optional_argc) {
            for (int i = argc; i < fixed_argc; i++) {
                auto param_type = param_types[i];
                FEATURE_CHECK_EQ(FT_IS_COMPLEX(param_type), true);
                OptionalType* optionalType = (OptionalType*)FT_GET_COMPLEX(param_type);
                FEATURE_CHECK_EQ(optionalType->header.type, COMPLEX_OPTIONAL);
                if (!createTypeDeclaration(param_type, ffi_arg_types[extra_argc + i])) {
                    FEATURE_LOG_ERROR("prepareType for type failed !");
                    got_error = true;
                    break;
                }
                ffi_arg_values[extra_argc + i] = &optionalType->fval;
                FeatureDupValue(ffi_arg_values[extra_argc + i]);
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
        if (is_promise) {
            ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(method.return_type);
            // create promise
            PromiseType* promise_type = (PromiseType*)complex_type;
            // create promise and add to instance
            pid = ((FeatureInstanceQjs*)instance)->addPromise(promise_type->resolveTypes[0], promise_type->resolveTypes[1]);
            feature_value_t promise = ((FeatureInstanceQjs*)instance)->getPromise(pid);
            // pass pid to native function
            ffi_arg_values[2] = &pid;
            // dup and return promise object.
            ret_val = feature_dup_value(ctx, promise);
        }
        // invoke method
        NativeFunc callback = description->dynamic ? instance->getVirtualFunction(method.func.vtable_idx) : method.func.callback;
        FEATURE_CHECK_NE(callback, nullptr);
        ffi_call(&cif, callback, ffi_ret_value, ffi_arg_values);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!is_promise && method.return_type != FT_VOID) {
            // process return value
            if ((FT_IS_COMPLEX(method.return_type)) && (((ComplexTypeHeader*)(FT_GET_COMPLEX(method.return_type)))->type == COMPLEX_STRUCT_MAP)
                && (*(void**)ffi_ret_value == nullptr)) {
                FEATURE_LOG_ERROR("struct ffi_ret_value is null !");
                feature_free_value(ctx, ret_val);
                ret_val = FEATURE_UNDEFINED;
                got_error = true;
            } else if (!FeatureFFIQjs::convertValueToGuest(instance, method.return_type, ffi_ret_value, ctx, ret_val)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                feature_free_value(ctx, ret_val);
                ret_val = FEATURE_EXCEPTION;
                got_error = true;
            }
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
    if (ffi_ret_value) {
        FeatureFreeValue(ffi_ret_value);
    }
    delete[] ffi_arg_values;
    delete[] ffi_arg_types;
    if (vari_params.vari_args) {
        delete[] vari_params.vari_args;
    }

    // if error occurred, throw internal error
    if (got_error) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native method failed !");
    }

    return ret_val;
}

static feature_value_t accessor_get(feature_context_ref ctx, feature_value_t this_val, int magic)
{
    void* data_ptr = nullptr;
    NativeFunc callback = nullptr;
    FeatureType feature_type = 0;
    // get info from this_val
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    int index = magic;
    FeatureInstance* instance = getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->prototype()->description()->members[index]);
    FEATURE_CHECK_EQ(member->type == MEMBER_ACCESSOR || member->type == MEMBER_CONST, true);
    bool is_dynamic = instance->prototype()->description()->dynamic;
    if (member->type == MEMBER_ACCESSOR) {
        MemberAccessor* accessor = &member->accessor;
        data_ptr = &accessor->data;
        callback = is_dynamic ? instance->getVirtualFunction(accessor->getter.vtable_idx) : accessor->getter.callback;
        feature_type = accessor->type;
    } else if (member->type == MEMBER_CONST) {
        MemberConst* memberConst = &member->value;
        data_ptr = &memberConst->data;
        callback = is_dynamic ? instance->getVirtualFunction(memberConst->func.vtable_idx) : memberConst->func.callback;
        feature_type = memberConst->type;
    }
    FEATURE_CHECK_NE(feature_type, FT_VOID);
    FEATURE_CHECK_NE(callback, nullptr);
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type* ffi_arg_types[2] = { &ffi_type_pointer, &ffi_type_sint64 };
    ffi_type* ffi_ret_type = nullptr;
    void* ffi_arg_values[2] = { &instance, data_ptr };
    void* ffi_ret_value = nullptr;
    do {
        if (!createTypeDeclaration(feature_type, ffi_ret_type)) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        if (!createHostValue(feature_type, ffi_ret_value, true)) {
            FEATURE_LOG_ERROR("create return value failed !");
            break;
        }

        // prepare and call method
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, ffi_ret_type, ffi_arg_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            break;
        }
        // invoke
        ffi_call(&cif, callback, ffi_ret_value, ffi_arg_values);
        // process return value
        if (!FeatureFFIQjs::convertValueToGuest(instance, feature_type, ffi_ret_value, ctx, ret_val)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            feature_free_value(ctx, ret_val);
            ret_val = FEATURE_EXCEPTION;
        }
    } while (0);
    // free resources
    freeTypeDeclaration(ffi_ret_type);
    FeatureFreeValue(ffi_ret_value);

    if (member->type == MEMBER_CONST) {
        // for const value, redefine the property with result value
        JS_DefinePropertyValueStr(static_cast<feature_context_ref>(ctx), this_val, member->name,
            feature_dup_value(ctx, ret_val), FEATURE_PROP_CONFIGURABLE);
    }

    return ret_val;
}

static feature_value_t accessor_set(feature_context_ref ctx, feature_value_t this_val, feature_value_t val, int magic)
{
    // get info from this_val
    int index = magic;
    FeatureInstance* instance = getInstance(this_val);
    FEATURE_CHECK_NE(instance, nullptr);
    Member* member = const_cast<Member*>(&instance->prototype()->description()->members[index]);
    FEATURE_CHECK_EQ(member->type, MEMBER_ACCESSOR);
    MemberAccessor* accessor = &member->accessor;
    FEATURE_CHECK_NE(accessor->type, FT_VOID);
    bool is_dynamic = instance->prototype()->description()->dynamic;
    NativeFunc callback = is_dynamic ? instance->getVirtualFunction(accessor->setter.vtable_idx) : accessor->setter.callback;
    FEATURE_CHECK_NE(callback, nullptr);
    // handle parameter
    // 1. FeatureInstance pointer
    // 2. data
    ffi_type* ffi_arg_types[3] = { &ffi_type_pointer, &ffi_type_sint64, nullptr };
    void* arg_value_input = nullptr;
    void* ffi_arg_values[3] = { &instance, &accessor->data, nullptr };
    do {
        // prepare third param type declaration, create by accessor type
        if (!createTypeDeclaration(accessor->type, ffi_arg_types[2])) {
            FEATURE_LOG_ERROR("createTypeDeclaration for ret type failed !");
            break;
        }
        // fill third param using guest value and accesor type
        if (!FeatureFFIQjs::convertValueToHost(instance, accessor->type, arg_value_input, ctx, val)) {
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
    return FEATURE_VALUE_UNDEFINED;
}

static feature_value_t const_variable_initialize(context_ref ctx, FeaturePrototype* prototype, const MemberConst& member_const)
{
    feature_value_t ret_val = FEATURE_VALUE_UNDEFINED;
    FEATURE_CHECK_NE(member_const.type, FT_VOID);
    // we do not handle interface intializer here, handle it as getter function.
    FEATURE_CHECK_EQ(prototype->description()->dynamic && member_const.func.vtable_idx != -1, false);
    // invoke callback to get constant value
    if (member_const.func.callback) {
        // create type using featureType description
        ffi_type* ffi_ret_type = nullptr;
        void* ffi_ret_value = nullptr;
        ffi_type* ffi_arg_types[2] = { &ffi_type_pointer, &ffi_type_sint64 };
        void* ffi_arg_values[2] = { &prototype, (void*)&member_const.data };
        if (!createTypeDeclaration(member_const.type, ffi_ret_type)) {
            FEATURE_LOG_ERROR("create type failed !");
            freeTypeDeclaration(ffi_ret_type);
            return ret_val;
        }
        if (!createHostValue(member_const.type, ffi_ret_value, true)) {
            FEATURE_LOG_ERROR("create return value failed !");
            freeTypeDeclaration(ffi_ret_type);
            FeatureFreeValue(ffi_ret_value);
            return ret_val;
        }
        // prepare and call
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, ffi_ret_type, ffi_arg_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            freeTypeDeclaration(ffi_ret_type);
            FeatureFreeValue(ffi_ret_value);
            return ret_val;
        }
        // invoke
        ffi_call(&cif, member_const.func.callback, ffi_ret_value, ffi_arg_values);
        // process return value
        if (!FeatureFFIQjs::convertValueToGuest(nullptr, member_const.type, ffi_ret_value, ctx, ret_val)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            feature_free_value(ctx, ret_val);
            ret_val = FEATURE_VALUE_UNDEFINED;
        }
        freeTypeDeclaration(ffi_ret_type);
        FeatureFreeValue(ffi_ret_value);
        if (FT_IS_REFERENCE(ffi_ret_value)) {
            free(ffi_ret_value);
        }
    } else {
        // check type
        if (!FeatureFFIQjs::convertValueToGuest(nullptr, member_const.type, (void*)&member_const.data, ctx, ret_val)) {
            FEATURE_LOG_ERROR("can not convert const value to guest!");
            feature_free_value(ctx, ret_val);
            ret_val = FEATURE_VALUE_UNDEFINED;
        }
    }
    return ret_val;
}

static int initialize_prototype(context_ref ctx, const FeatureDescription* description, FeaturePrototype* prototype, feature_value_t js_proto)
{
    FEATURE_CHECK(description != nullptr && prototype != nullptr);
    for (int i = 0; i < description->member_count; i++) {
        const Member& member = description->members[i];
        switch (member.type) {
        case MEMBER_NULL: {
            // not allowed
            FEATURE_CHECK(false && "invalid member type!");
        } break;
        case MEMBER_METHOD: {
            // register different type
            const MemberMethod& method = member.method;
            feature_value_t methodCallObj = JS_NewCFunctionMagic(static_cast<feature_context_ref>(ctx), method_call, member.name, getParamCount(method.parameters), JS_CFUNC_generic_magic, i);
            feature_define_object_property(ctx, js_proto, member.name, methodCallObj, FEATURE_PROP_ENUMERABLE);
        } break;
        case MEMBER_ACCESSOR: {
            // create getter and setter
            const MemberAccessor& accessor = member.accessor;
            feature_atom_t prop_name = feature_atom(static_cast<feature_context_ref>(ctx), member.name);
            feature_value_t funcs[2] = { FEATURE_VALUE_UNDEFINED, FEATURE_VALUE_UNDEFINED };

            char buf[128];
            JSCFunctionType type;
            if (accessor.getter.callback) {
                type.getter_magic = accessor_get;
                sprintf(buf, "get %s", member.name);
                funcs[0] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 0, JS_CFUNC_getter_magic, i);
            }
            if (accessor.setter.callback) {
                type.setter_magic = accessor_set;
                sprintf(buf, "set %s", member.name);
                funcs[1] = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 1, JS_CFUNC_setter_magic, i);
            }
            JS_DefinePropertyGetSet(static_cast<feature_context_ref>(ctx), js_proto, prop_name, funcs[0], funcs[1], FEATURE_PROP_CONFIGURABLE);
            feature_free_atom(static_cast<feature_context_ref>(ctx), prop_name);
        } break;
        case MEMBER_CONST: {
            // handle member const
            const MemberConst& constMember = member.value;
            // if not interface or constant defined value, use const_variable_initialize
            if (!description->dynamic || constMember.func.vtable_idx == -1) {
                feature_value_t constantVal = const_variable_initialize(ctx, prototype, constMember);
                feature_define_object_property(ctx, js_proto, member.name, constantVal, FEATURE_PROP_ENUMERABLE);
            } else {
                // add a getter function for interface initializer sitution
                char buf[128];
                JSCFunctionType type;
                type.getter_magic = accessor_get;
                sprintf(buf, "get %s", member.name);
                feature_value_t const_member_getter = JS_NewCFunction2(static_cast<feature_context_ref>(ctx), type.generic, buf, 0, JS_CFUNC_getter_magic, i);
                feature_atom_t prop_name = feature_atom(static_cast<feature_context_ref>(ctx), member.name);
                JS_DefinePropertyGetSet(static_cast<feature_context_ref>(ctx), js_proto, prop_name, const_member_getter, FEATURE_VALUE_UNDEFINED, FEATURE_PROP_CONFIGURABLE);
                feature_free_atom(static_cast<feature_context_ref>(ctx), prop_name);
            }

        } break;
        }
    }
    return 0;
}

// static members
feature_classid_t FeatureManagerQjs::js_class_id_ = 0;
feature_classdef_t FeatureManagerQjs::js_class_def_ = {
    .class_name = "FeatureInstanceObject",
    .finalizer = __feature_finalizer,
    .gc_mark = __feature_mark
};
uv_mutex_t FeatureManagerQjs::js_class_mutex_ = PTHREAD_MUTEX_INITIALIZER;

// static methods
bool FeatureManagerQjs::ensureJsClass(feature_context_ref ctx)
{
    auto rt = JS_GetRuntime(ctx);
    // here we do twice judgement for js_class_id. the one outside the mutex scope is for fast
    // judgement, the other inside the mutex scope is to prevent thread racing coditions.
    if (js_class_id_ != 0 && JS_IsRegisteredClass(rt, js_class_id_)) {
        FEATURE_LOG_DEBUG("class_id already registered.");
        return true;
    }

    uv_mutex_lock(&js_class_mutex_);
    if (js_class_id_ != 0 && JS_IsRegisteredClass(rt, js_class_id_)) {
        FEATURE_LOG_DEBUG("class_id already registered.");
        uv_mutex_unlock(&js_class_mutex_);
        return true;
    }
    FEATURE_LOG_INFO("last class_id: %d.", js_class_id_);

    js_class_id_ = JS_NewClassID(&js_class_id_);
    if (js_class_id_ == 0) {
        FEATURE_LOG_ERROR("create js class_id failed.");
        uv_mutex_unlock(&js_class_mutex_);
        return false;
    }

    FEATURE_LOG_INFO("created class_id: %d.", js_class_id_);
    JS_NewClass(rt, js_class_id_, &js_class_def_);
    uv_mutex_unlock(&js_class_mutex_);
    return true;
}

FeatureManagerQjs::FeatureManagerQjs(FeatureRegistry* registry)
    : FeatureManager(registry)
{
}

FeatureManagerQjs::~FeatureManagerQjs()
{
}

bool FeatureManagerQjs::ensureJsPrototype(FeaturePrototypeQjs* prototype)
{
    auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(prototype->ft_proto());
    if (!feature_is_undefined(*js_proto_ptr))
        return true;

    auto ctx = (feature_context_ref)ft_context_get_data(getFeatureContext());
    feature_value_t js_proto = feature_object(ctx);
    if (feature_is_exception(js_proto)) {
        feature_dump_error(ctx);
        return false;
    }

    initialize_prototype(ctx, prototype->description(), prototype, js_proto);
    // TODO: initialize js_proto using description
    if (prototype->description()->native_callbacks && prototype->description()->native_callbacks->onCreate) {
        FEATURE_LOG_DEBUG("invoke onCreate callback...");
        prototype->description()->native_callbacks->onCreate(ctx, prototype);
    }

    *js_proto_ptr = js_proto;
    return true;
}

feature_value_t FeatureManagerQjs::createJsInstance(FeaturePrototypeQjs* prototype, FeatureInstance* instance)
{
    ft_context_ref ft_ctx = getFeatureContext();
    auto ctx = (feature_context_ref)ft_context_get_data(ft_ctx);
    FEATURE_CHECK_NE(ctx, nullptr);
    // ensure js feature prototype is created
    if (!ensureJsPrototype(prototype))
        return FEATURE_VALUE_UNDEFINED;

    if (!ensureJsClass(ctx)) {
        FEATURE_LOG_ERROR("invalid js class_id: %d", js_class_id_);
        return FEATURE_VALUE_UNDEFINED;
    }

    // create instance with prototype and set opaque refers to FeatureInstance
    FEATURE_LOG_INFO("created js instance with class_id: %d.", js_class_id_);
    auto js_proto = FT_VAL_GET_JS_VAL(prototype->ft_proto());
    feature_value_t js_instance = JS_NewObjectProtoClass(ctx, js_proto, js_class_id_);
    feature_set_opaque(js_instance, instance);
    // setup instance WeakRef, refers to js_instance
    ((FeatureInstanceQjs*)instance)->initWeakRef(js_instance);
    return js_instance;
}

feature_value_t FeatureManagerQjs::featureRequire(context_ref ctx, feature_value_t vm_object, const char* name)
{
    FEATURE_LOG_DEBUG("featureRequire for '%s'", name);
    auto feature_pair = getFeatureRegistry()->findFeature(name);
    if (!feature_pair || !feature_pair->first->description) {
        FEATURE_LOG_DEBUG("can't find native feature '%s', fallback to original JS module load!", name);
        return FEATURE_VALUE_UNDEFINED;
    }
    const FeatureDescription* description = feature_pair->first;

    if (!getFeatureContext()) {
        ft_context_ref ft_ctx = CreateFeatureContextQjs(ctx);
        setFeatureContext(ft_ctx);
    }

    auto& prototype = feature_pair->second;
    if (!prototype) {
        // create proto
        prototype = new FeaturePrototypeQjs(description);
        prototype->setFeatureManager(this);
        setPackageName(getFeatureRegistry()->getFeaturePackageName());
        setEnvName(FEATURE_ENV_NAME);
    }

    // create feature instance for the required object
    auto instance = std::make_unique<FeatureInstanceQjs>(prototype);
    auto instance_ptr = instance.get();
    // save vm_object into instance
    instance->setVmObject(vm_object);

    // insert into instances array, update iid
    int iid = prototype->addInstance(std::move(instance));
    instance_ptr->setInstanceId(iid);

    auto js_instance = createJsInstance((FeaturePrototypeQjs*)prototype, instance_ptr);
    if (description->native_callbacks && description->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        description->native_callbacks->onRequired(ctx, instance_ptr);
    }
    return js_instance;
}

feature_value_t FeatureManagerQjs::createTargetInterface(FeatureInstance* interf) {
    FEATURE_CHECK_NE(interf, nullptr);
    FeaturePrototypeQjs* proto = static_cast<FeaturePrototypeQjs*>(interf->prototype());
    FEATURE_CHECK_NE(proto, nullptr);
    auto unique_interf= std::unique_ptr<FeatureInstance>(interf);
    int iid = proto->addInstance(std::move(unique_interf));
    interf->setInstanceId(iid);

    auto js_interface = createJsInstance(proto, interf);
    return js_interface;
}

void FeatureManagerQjs::uninit()
{
    auto ft_ctx = getFeatureContext();
    if (!ft_ctx) {
        FEATURE_LOG_INFO("ft_ctx is missing");
        return;
    }

    JSContext* js_ctx = (JSContext*)ft_context_get_data(ft_ctx);
    auto free_prototype = [js_ctx](FeaturePrototypeQjs* prototype) {
        auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(prototype->ft_proto());
        if (!feature_is_undefined(*js_proto_ptr)) {
            feature_free_value(js_ctx, *js_proto_ptr);
            *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
        }
    };

    for (const auto& pair : getFeatureRegistry()->getRegisteredFeatures()) {
        auto proto = static_cast<FeaturePrototypeQjs*>(pair.second.second);
        auto description = pair.second.first;
        FEATURE_CHECK_NE(description, nullptr);
        if (!proto)
            continue;

        for (auto& proto_pair : proto->children()) {
            // clear all interface instances belongs to this instance.
            auto child_proto = static_cast<FeaturePrototypeQjs*>(proto_pair.second.get());
            child_proto->clearAllInstances();
            FEATURE_LOG_INFO("free interface prototype '%s'", proto_pair.first);
            free_prototype(child_proto);
        }

        // clear all feature instance at first, it will free all feature instance and call onDetach for them
        proto->clearAllInstances();
        // call feature's onDestroy
        if (description->native_callbacks && description->native_callbacks->onDestroy) {
            FEATURE_LOG_DEBUG("invoke onDestroy callback...");
            description->native_callbacks->onDestroy(js_ctx, proto);
        }
        FEATURE_LOG_INFO("free feature prototype '%s'", description->name);
        free_prototype(proto);
        delete pair.second.second;
    }

    // uninit registery
    delete getFeatureRegistry();

    if (getFeatureContext()) {
        ReleaseFeatureContextQjs(getFeatureContext());
        setFeatureContext(nullptr);
    }
}

feature_value_t FeatureManagerQjs::findFeature(feature_context_ref ctx, const char* name)
{
    FEATURE_LOG_DEBUG("findFeature for '%s'", name);
    auto feature_pair = getFeatureRegistry()->findFeature(name);
    if (!feature_pair || !feature_pair->first) {
        FEATURE_LOG_WARN("can't find description for native feature '%s'!", name);
        return FEATURE_VALUE_UNDEFINED;
    }

    if (!getFeatureContext()) {
        ft_context_ref ft_ctx = CreateFeatureContextQjs(ctx);
        setFeatureContext(ft_ctx);
    }

    // create proto
    const FeatureDescription* description = feature_pair->first;
    auto& prototype = feature_pair->second;
    if (!prototype) {
        prototype = new FeaturePrototypeQjs(description);
        prototype->setFeatureManager(this);
        setPackageName(getFeatureRegistry()->getFeaturePackageName());
        setEnvName(FEATURE_ENV_NAME);
    }

    if (!ensureJsPrototype((FeaturePrototypeQjs*)prototype)) {
        FEATURE_LOG_ERROR("ensure js prototype failed !");
        return FEATURE_VALUE_UNDEFINED;
    }

    auto js_proto = FT_VAL_GET_JS_VAL(((FeaturePrototypeQjs*)prototype)->ft_proto());
    return feature_dup_value(ctx, js_proto);
}

feature_value_t FeatureManagerQjs::createFeature(feature_context_ref ctx, feature_value_t proto, feature_value_t vm_object)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(getFeatureContext());
    for (const auto& pair : getFeatureRegistry()->getRegisteredFeatures()) {
        auto prototype = static_cast<FeaturePrototypeQjs*>(pair.second.second);
        if (!prototype)
            continue;

        auto js_proto = FT_VAL_GET_JS_VAL(prototype->ft_proto());
        if (!feature_is_same_value(js_ctx, js_proto, proto))
            continue;

        // create feature instance for the required object
        auto instance = std::make_unique<FeatureInstanceQjs>(prototype);
        // save vm_object into instance
        instance->setVmObject(vm_object);
        auto instance_ptr = instance.get();
        // insert into instances array, update iid
        int iid = prototype->addInstance(std::move(instance));
        instance_ptr->setInstanceId(iid);

        // create prototype class instance
        auto description = pair.second.first;
        auto js_instance = createJsInstance(prototype, instance_ptr);
        if (description->native_callbacks && description->native_callbacks->onRequired) {
            FEATURE_LOG_DEBUG("invoke onRequired callback...");
            description->native_callbacks->onRequired(ctx, instance_ptr);
        }
        return js_instance;
    }

    return FEATURE_VALUE_UNDEFINED;
}

}
