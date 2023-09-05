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
#include "feature_framework.h"

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
    bool createTypeDeclaration(FEATURE::FeatureType featureType, ffi_type*& type);

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
    bool createHostValue(FEATURE::FeatureType featureType, void*& ptr, bool createPtrOnly = false);

    /**
     * @brief exact variadic parameter using va_list
     *
     * @param ap
     * @param featureType
     * @return void*
     */
    void* exactVariadicParameter(va_list& ap, FEATURE::FeatureType featureType);
};
#endif
