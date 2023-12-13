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

#ifndef __FEATURE_FFI_H__
#define __FEATURE_FFI_H__

#include "feature_exports.h"
#include "feature_log.h"
#include "feature_utils.h"

#include "value_translator.h"
#include "feature_instance.h"
#include "feature_common.h"

#include <cstdarg>
#include <ffi.h>
#include <stdalign.h>

namespace ferry {

/**
 * @brief create type declaration using FeatureType
 *
 * @param featureType
 * @param type
 * @return true
 * @return false
 */
bool createTypeDeclaration(FeatureType featureType, ffi_type*& type);

/**
 * @brief free ffi type declaration
 *
 * @param type
 */
void freeTypeDeclaration(ffi_type*& type);

/**
 * @brief Create a Host Value object
 *
 * @param featureType
 * @param ptr
 * @param createPtrOnly
 * @return true
 * @return false
 */
bool createHostValue(FeatureType featureType, void*& ptr, bool createPtrOnly = false);

/**
 * @brief exact variadic parameter using va_list
 *
 * @param ap
 * @param featureType
 * @return void*
 */
void* exactVariadicParameter(va_list& ap, FeatureType featureType);

// templeate functions
template<typename TNative, typename TCtx, typename TTarget>
bool argToNativePtr(TCtx ctx, TTarget& target, void* native_ptr) {
    return value_translator::toNative(ctx, target, (TNative*)native_ptr);
}

template<typename TInstance, typename TCtx, typename TTarget>
bool convertValueToNative(TInstance* instance, FeatureType ftype,
        TCtx ctx, TTarget& target, void*& pnative) {
    TRY_GET_REAL_TYPE(ftype);
    if (!pnative) {
        if (!createHostValue(ftype, pnative)) {
            FEATURE_LOG_ERROR("create native value failed !");
            return false;
        }
    }

    if (FT_IS_REFERENCE(ftype)) {
        void*& value_ptr = *(void**)pnative;
        if (!convertValueToNative(instance, FT_REMOVE_REFERENCE(ftype), ctx, target, value_ptr)) {
            FEATURE_LOG_ERROR("convert target to native failed !");
            return false;
        }
        return true;
    }

    if (FT_IS_PRIMITIVE(ftype)) {
        switch (FT_GET_VALUE(ftype)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            }
            case FT_BOOLEAN:
                if (!argToNativePtr<bool>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to bool failed !");
                    return false;
                } break;
            case FT_INT:
                if (!argToNativePtr<int32_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int failed !");
                    return false;
                } break;
            case FT_INT8:
                if (!argToNativePtr<int8_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int8_t failed !");
                    return false;
                } break;
            case FT_UINT8:
                if (!argToNativePtr<uint8_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint8_t failed !");
                    return false;
                } break;
            case FT_INT16:
                if (!argToNativePtr<int16_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int16_t failed !");
                    return false;
                } break;
            case FT_UINT16:
                if (!argToNativePtr<uint16_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint16_t failed !");
                    return false;
                } break;
            case FT_INT32:
                if (!argToNativePtr<int32_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int32_t failed !");
                    return false;
                } break;
            case FT_UINT32:
                if (!argToNativePtr<uint32_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint32_t failed !");
                    return false;
                } break;
            case FT_INT64:
                if (!argToNativePtr<int64_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int64_t failed !");
                    return false;
                } break;
            case FT_UINT64:
                if (!argToNativePtr<uint64_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint64_t failed !");
                    return false;
                } break;
            case FT_FLOAT:
                if (!argToNativePtr<float>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to float failed !");
                    return false;
                } break;
            case FT_DOUBLE:
                if (!argToNativePtr<double>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to double failed !");
                    return false;
                } break;
            case FT_CHAR:
                if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                    FEATURE_LOG_ERROR("string arg is null or undefined!");
                    pnative = NULL;
                } else if (!value_translator::isString(ctx, target)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need string !");
                    return false;
                } else {
                    const char* str = NULL;
                    if (!argToNativePtr<const char*>(ctx, target, (void*)(&str))) {
                        FEATURE_LOG_ERROR("convert to const char* failed !");
                        return false;
                    }
                    char* alloc_ptr = (char*)FeatureMalloc(strlen(str) + 1, FT_CHAR);
                    strcpy(alloc_ptr, str);
                    value_translator::freeString(ctx, str); // to do by wjf
                    pnative = alloc_ptr;
                } break;
            case FT_ANY:
                if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                    FEATURE_LOG_ERROR("object is null or undefined!");
                    pnative = NULL;
                } else {
                    ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY);
                    if (!argToNativePtr<ft_value_t>(ctx, target, f_val)) {
                        FEATURE_LOG_ERROR("convert to ft_value_t failed !");
                        FeatureFreeValue(f_val);
                        pnative = NULL;
                    }
                    pnative = f_val;
                } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
     } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complex_type->type) {
            case COMPLEX_STRUCT_MAP: {
                if (value_translator::isUndefined(ctx, target)) {
                    FEATURE_LOG_WARN("js struct value missing!");
                    pnative = NULL;
                    break;
                }
                ObjectMapType &objMapType = *(ObjectMapType *)complex_type;
                ObjectMember *members = (ObjectMember *)objMapType.members;
                auto member_count = countMember(members);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    bool ret;
                    TTarget field;
                    auto member = &members[i];
                    if (!value_translator::getObjectField(ctx, target, member->name, &field)) {
                        // check field is js_undefined or not
                        if (FT_IS_COMPLEX(member->type)) {
                            ComplexTypeHeader* cmp_type = (ComplexTypeHeader*)FT_GET_COMPLEX(member->type);
                            if (cmp_type->type == COMPLEX_OPTIONAL) {
                                FEATURE_LOG_DEBUG("field is undefined, we get value with optinalType!");
                                OptionalType* opt_type = (OptionalType*)cmp_type;
                                ret = convertValueToTarget(instance, opt_type->type, ctx, &opt_type->fval, field);  // to do by wjf
                                if (!ret) {
                                    value_translator::freeValue(ctx, field);
                                    FEATURE_LOG_ERROR("propValue convert optional failed!");
                                    return false;
                                }
                            }
                        } else {
                            FEATURE_LOG_DEBUG("COMPLEX_STRUCT_MAP member->type is %d!", member->type);
                        }
                    }

                    void *member_ptr = (void *)((char *)pnative + member->offset);
                    ret = convertValueToNative(instance, member->type, ctx, field, member_ptr);
                    value_translator::freeValue(ctx, field);
                    if (!ret) {
                        printf("get property value for key: %s failed !", member->name);
                        return false;
                    }
                }
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* opt_type = (OptionalType*)complex_type;
                bool ret = convertValueToNative(instance, opt_type->type, ctx, target, pnative);
                if (!ret) {
                    FEATURE_LOG_ERROR("convert optional type failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                // save into instance
                CallbackType *callbackType = (CallbackType *)complex_type;
                FtCallbackId id = instance->addCallback(target, callbackType);
                *(FtCallbackId *)pnative = id; // write callback id to pointer.
            } break;
            case COMPLEX_ARRAY: {
                ArrayType& array_type = *(ArrayType*)complex_type;
                auto elem_type = array_type.element_type;
                if (!value_translator::isArray(ctx, target)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need array !");
                    return false;
                }
                auto asize = value_translator::arraySize(ctx, target);
                FtArray* array_data = (FtArray*)pnative;
                array_data->_size = asize;
                if (asize) {
                    // we support reference and primitive types
                    size_t elem_size = FT_IS_REFERENCE(elem_type) ? sizeof(uintptr_t) : getValueSize(elem_type);
                    auto size = elem_size * asize;
                    FEATURE_CHECK_NE(size, 0);
                    array_data->_element = malloc(size);
                    memset(array_data->_element, 0, size);
                    for (size_t i = 0; i < asize; i++) {
                        // fill it
                        TTarget elem_val = value_translator::arrayGet(ctx, target, i);
                        FEATURE_CHECK_NE(value_translator::isUndefined(ctx, elem_val), true);
                        void* elem_ptr = ((char*)array_data->_element + elem_size * i);
                        if (!convertValueToNative(instance, elem_type, ctx, elem_val, elem_ptr)) {
                            FEATURE_LOG_ERROR("convert array element failed ");
                            value_translator::freeValue(ctx, elem_val);
                            break;
                        }
                        value_translator::freeValue(ctx, elem_val);
                    }
                }
                FEATURE_LOG_DEBUG("array data: %p", ptr);
            } break;
            case COMPLEX_PROMISE: {
                FEATURE_LOG_ERROR("do not support convert promise to guest !");
                return false;
            } break;
            case COMPLEX_INTERFACE: {  // to do by wjf
                pnative = instance->getNativeInterface(target);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
        }
    }
}

template<typename TNative, typename TCtx, typename TTarget>
void nativeToTarget(TCtx ctx, void* ptr, TTarget& target) {
  value_translator::toTarget(ctx, *((TNative*)ptr), &target);
}

template<typename TInstance, typename TCtx, typename TTarget>
bool convertValueToTarget(TInstance* instance, FeatureType ftype,
    TCtx ctx, void* pnative, TTarget& target)
{
    FEATURE_CHECK_NE(pnative, nullptr);
    if (FT_IS_REFERENCE(ftype)) {
            pnative = *(void**)pnative;
    }
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (FT_GET_VALUE(ftype)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            } break;
            case FT_INT: {
                nativeToTarget<int32_t>(ctx, pnative, target);
            } break;
            case FT_INT8: {
                nativeToTarget<int8_t>(ctx, pnative, target);
            } break;
            case FT_UINT8: {
                nativeToTarget<uint8_t>(ctx, pnative, target);
            } break;
            case FT_INT16: {
                nativeToTarget<int16_t>(ctx, pnative, target);
            } break;
            case FT_UINT16: {
                nativeToTarget<uint16_t>(ctx, pnative, target);
            } break;
            case FT_INT32: {
                nativeToTarget<int32_t>(ctx, pnative, target);
            } break;
            case FT_UINT32: {
                nativeToTarget<uint32_t>(ctx, pnative, target);
            } break;
            case FT_INT64: {
                nativeToTarget<int64_t>(ctx, pnative, target);
            } break;
            case FT_UINT64: {
                nativeToTarget<uint64_t>(ctx, pnative, target);
            } break;
            case FT_FLOAT: {
                nativeToTarget<float>(ctx, pnative, target);
            } break;
            case FT_DOUBLE: {
                nativeToTarget<double>(ctx, pnative, target);
            } break;
            case FT_BOOLEAN: {
                nativeToTarget<bool>(ctx, pnative, target);
            } break;
            case FT_CHAR: {
                if (!pnative)
                    nativeToTarget<const char*>(ctx, (void*)(""), target);
                else
                    nativeToTarget<const char*>(ctx, pnative, target);
            } break;
            case FT_ANY: {
                if (!pnative) {
                    ft_value_t null_val = value_translator::nullFtVal();
                    nativeToTarget<ft_value_t>(ctx, &null_val, target);
                } else {
                    nativeToTarget<ft_value_t>(ctx, pnative, target);
                }
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complex_type->type) {
            case COMPLEX_STRUCT_MAP: {
                ObjectMapType& objMapType = *(ObjectMapType*)complex_type;
                auto member = objMapType.members;
                auto member_count = countMember(member);
                target = value_translator::newObject(ctx);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    void* member_ptr = (void*)((char*)pnative + member->offset);
                    TTarget prop;
                    bool ret = convertValueToTarget(instance, member->type, ctx, member_ptr, prop);
                    if (!ret) {
                        value_translator::freeValue(ctx, prop);
                        FEATURE_LOG_ERROR("convert property name: %s failed !", member->name);
                        return false;
                    }
                    value_translator::setObjectField(ctx, target, member->name, prop);
                    member++;
                }
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* opt_type = (OptionalType*)complex_type;
                bool ret = convertValueToTarget(instance, opt_type->type, ctx, pnative, target);
                if (!ret) {
                    value_translator::freeValue(ctx, target);
                    FEATURE_LOG_ERROR("convert optional to guest failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                // unreachable
                FEATURE_LOG_ERROR("convert callback to guest is unreachable");
            } break;
            case COMPLEX_ARRAY: {
                // convert to guest
                ArrayType* array_type = (ArrayType*)complex_type;
                FtArray* array_data = (FtArray*)pnative;
                auto elem_type = array_type->element_type;
                FEATURE_CHECK_EQ(FT_IS_REFERENCE(elem_type), true);
                size_t elem_size = sizeof(uintptr_t);
                // exact and create js target
                target = value_translator::newArray(ctx);
                for (int32_t i = 0; i < array_data->_size; i++) {
                    void* elem_ptr = ((char*)array_data->_element + elem_size * i);
                    // convert element target
                    TTarget elem_val;
                    if (!convertValueToTarget(instance, elem_type, ctx, elem_ptr, elem_val)) {
                        FEATURE_LOG_ERROR("convert array element to guest failed !");
                        value_translator::freeValue(ctx, elem_val);
                        value_translator::freeValue(ctx, target);
                        return false;
                    }
                    value_translator::arraySet(ctx, target, i, elem_val);
                }
            } break;
            case COMPLEX_PROMISE: {
                FEATURE_LOG_ERROR("do not support convert promise to host !");
                return false;
            } break;
            case COMPLEX_INTERFACE: {
                InterfaceType* interface_type = (InterfaceType*)complex_type;
                FEATURE_CHECK_NE(interface_type->desc, nullptr);
                FEATURE_CHECK_NE(pnative, nullptr);
                auto interface_ptr = static_cast<TInstance*>(pnative);
                target = interface_ptr->createTargetInterface(interface_type->desc);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
        }
    }

    return true;
}

template<typename TInstance, typename TCtx, typename TTarget>
bool methodCall(TInstance* instance, TCtx ctx, JSContext* js_ctx,
        Member* member, int argc, TTarget* argv, TTarget& ret_val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type, MEMBER_METHOD);
    auto description = instance->prototype()->description;
    const auto& method = member->method;
    auto param_types = method.parameters;

    bool got_error = false;
    FtPromiseId pid = -1;
    bool has_rest_param = false;
    int optional_argc = 0;
    int fixed_argc = getParamCount(param_types, &has_rest_param, &optional_argc);
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_param && optional_argc, true);

    // variadic parameters type
    ffi_type vari_args_type;
    ffi_type* vari_args_elem_types[3];
    FtVariParams vari_params;
    memset(&vari_params, 0, sizeof(vari_params));

    // beacuse we support rest parameters, so argc is greater or equal to fixed_argc.
    if (has_rest_param) {
        FEATURE_CHECK_GE(argc, fixed_argc);
        vari_params.vari_count = argc - fixed_argc;
    } else if (optional_argc) {
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
    // FeaturInstance, data, maybe return promise, maybe variadic count, empty placeholder
    ffi_type** ffi_arg_types = new ffi_type*[extra_argc + packed_argc + optional_argc + 1];
    memset(ffi_arg_types, 0, sizeof(ffi_type*) * (extra_argc + packed_argc + optional_argc + 1));
    // FeaturInstance, data, maybe return promise, maybe variadic count
    void** ffi_arg_values = new void*[extra_argc + packed_argc + optional_argc];
    memset(ffi_arg_values, 0, sizeof(void*) * (extra_argc + packed_argc + optional_argc));
    ffi_type* ffi_ret_type = nullptr;
    void* ffi_ret_value = nullptr;

    // prepare first two param
    // FeatureContext and data
    ffi_arg_types[0] = &ffi_type_pointer;
    ffi_arg_types[1] = &ffi_type_sint64;
    if (is_promise) {
        ffi_arg_types[2] = &ffi_type_sint32;
    }
    ffi_arg_values[0] = &instance;
    ffi_arg_values[1] = (void*)&method.data;

    do {
        for (int i = 0; i < fixed_argc; i++) {
            TTarget curr_arg = argv[i];
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
            if (!convertValueToNative(instance, param_type, ctx, curr_arg, ffi_arg_values[extra_argc + i])) {
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
                *js_val_ptr = value_translator::getVariArg(ctx, argv[i + fixed_argc]);
            }
        } else if (optional_argc > 0) {
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
        if (is_promise) {
            ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(method.return_type);
            // create promise
            PromiseType* promise_type = (PromiseType*)complex_type;
            // create promise and add to instance
            pid = instance->addPromise(promise_type->resolveTypes[0], promise_type->resolveTypes[1]);
            promise = feature_dup_value(js_ctx, instance->getPromise(pid));
            // pass pid to native function
            ffi_arg_values[2] = &pid;
        }

        // invoke method
        NativeFunc callback = description->dynamic ? instance->getVirtualFunction(method.func.vtable_idx) : method.func.callback;
        FEATURE_CHECK_NE(callback, nullptr);
        ffi_call(&cif, callback, ffi_ret_value, ffi_arg_values);
        // process return value, do not handle promise, it is handled before we invoke ffi_call.
        if (!is_promise && method.return_type != FT_VOID) {
            // process return value
            if (!convertValueToTarget(instance, method.return_type, ctx, ffi_ret_value, ret_val)) {
                FEATURE_LOG_ERROR("can not convert return value to guest!");
                value_translator::freeValue(ctx, ret_val);
                got_error = true;
            }
        } else if (is_promise) {
            value_translator::toTargetPromise(js_ctx, promise, ret_val);
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
        return false;
    }
    return true;
}

template<typename TInstance, typename TCtx, typename TTarget>
bool accessorGet(TInstance* instance, TCtx ctx, Member* member, TTarget& ret_val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type == MEMBER_ACCESSOR, true);

    bool got_error = false;
    MemberAccessor* accessor = &member->accessor;
    void* data_ptr = &accessor->data;
    FeatureType feature_type = accessor->type;
    FEATURE_CHECK_NE(feature_type, FT_VOID);
    bool is_dynamic = instance->prototype()->description->dynamic;
    NativeFunc callback = is_dynamic ?
            instance->getVirtualFunction(accessor->getter.vtable_idx) : accessor->getter.callback;
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
            got_error = true;
            break;
        }
        if (!createHostValue(feature_type, ffi_ret_value, true)) {
            FEATURE_LOG_ERROR("create return value failed !");
            got_error = true;
            break;
        }

        // prepare and call method
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, ffi_ret_type, ffi_arg_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            got_error = true;
            break;
        }
        // invoke
        ffi_call(&cif, callback, ffi_ret_value, ffi_arg_values);
        // process return value
        if (!convertValueToTarget(instance, feature_type, ctx, ffi_ret_value, ret_val)) {
            FEATURE_LOG_ERROR("can not convert return value to guest!");
            value_translator::freeValue(ctx, ret_val);
        }
    } while (0);

    freeTypeDeclaration(ffi_ret_type);
    FeatureFreeValue(ffi_ret_value);

    if (got_error) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native accessorGet failed !");
        return false;
    }
    return true;
}

template<typename TInstance, typename TCtx, typename TTarget>
bool accessorSet(TInstance* instance, TCtx ctx, Member* member, TTarget& val)
{
    FEATURE_CHECK_NE(instance, nullptr);
    FEATURE_CHECK_NE(member, nullptr);
    FEATURE_CHECK_EQ(member->type == MEMBER_ACCESSOR, true);
    MemberAccessor* accessor = &member->accessor;
    FEATURE_CHECK_NE(accessor->type, FT_VOID);
    bool is_dynamic = instance->prototype()->description->dynamic;
    NativeFunc callback = is_dynamic ?
            instance->getVirtualFunction(accessor->setter.vtable_idx) : accessor->setter.callback;
    FEATURE_CHECK_NE(callback, nullptr);
    bool got_error = false;

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
            got_error = true;
            break;
        }
        // fill third param using guest value and accesor type
        if (!convertValueToNative(instance, accessor->type, ctx, val, arg_value_input)) {
            FEATURE_LOG_ERROR("convert value to native failed !");
            got_error = true;
            break;
        }
        ffi_arg_values[2] = arg_value_input;

        // prepare and call method
        ffi_cif cif;
        ffi_status ret = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 3, &ffi_type_void, ffi_arg_types);
        if (ret) {
            FEATURE_LOG_ERROR("ffi_prep_cif failed: %d", ret);
            got_error = true;
            break;
        }
        // invoke
        ffi_call(&cif, callback, ffi_arg_values[2], ffi_arg_values);
    } while (0);
    // free resources
    freeTypeDeclaration(ffi_arg_types[2]);
    FeatureFreeValue(arg_value_input);

    if (got_error) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "invoke native accessorSet failed !");
        return false;
    }
    return true;
}

};
#endif
