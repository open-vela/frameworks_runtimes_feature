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
        if (!convertValueToTarget(instance, feature_type, ffi_ret_value, ctx, ret_val)) {
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
        if (!convertValueToNative(instance, accessor->type, arg_value_input, ctx, val)) {
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
