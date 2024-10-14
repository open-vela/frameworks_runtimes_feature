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

#include "feature_common.h"
#include "feature_instance.h"

#include <cstdarg>
#include <stdalign.h>

namespace feature_framework {

/**
 * @brief Create a Host Value object
 *
 * @param featureType
 * @param ptr
 * @return true
 * @return false
 */
bool createHostValue(FeatureType featureType, void*& ptr);

/**
 * @brief exact variadic parameter using va_list
 *
 * @param ap
 * @param featureType
 * @return void*
 */
void* extractVariadicParam(va_list& ap, FeatureType featureType);

};
#endif
