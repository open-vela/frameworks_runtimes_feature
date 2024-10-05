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

#ifndef __FEATURE_FFI_QJS_H__
#define __FEATURE_FFI_QJS_H__

#include "feature.h"
#include "feature_ffi.h"

namespace feature_framework {

class FeatureInstance;

/**
 * @brief the feature ffi functions
 *
 */
namespace FeatureFFIQjs {

    /**
     * @brief convert value from guest to host
     *
     * @param instance
     * @param featureType
     * @param ptr the ptr will allocated in this function
     * @param ctx
     * @param value
     * @return true
     * @return false
     */
    bool convertValueToHost(FeatureInstance* instance, FeatureType featureType, void*& ptr, context_ref ctx, feature_value_t value);

    /**
     * @brief convert value from host to guest
     *
     * @param featureType
     * @param ptr
     * @param ctx
     * @param value
     * @return true
     * @return false
     */
    bool convertValueToGuest(FeatureType featureType, void* ptr, context_ref ctx, feature_value_t& value);
}

}
#endif
