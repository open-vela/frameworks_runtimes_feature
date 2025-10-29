/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#ifndef __ARRAY_BUFFER_H__
#define __ARRAY_BUFFER_H__

#include "feature_exports.h"

namespace feature_framework {

struct ArrayBufferCreateParams {
    enum {
        kNative, // array buffer created by native code with native memory
        kNativeCopy, // array buffer created by native code with copyed memory
        kTarget, // array buffer created by js code
    };
    int type;
    union {
        // kNative
        struct {
            uint8_t* data;
            size_t size;
            FeatureArrayBufferFreeFunc free_func;
            void* opaque;
        } native;
        // kNativeCopy
        struct {
            uint8_t* data;
            size_t size;
        } copy;
        // kTarget
        ft_value_t target;
    };
};

class ArrayBuffer {

public:
    ArrayBuffer() = default;
    virtual ~ArrayBuffer() = default;

    virtual uint8_t* getData(size_t* psize) = 0;

    virtual void destroy() = 0;
};

}
#endif // __ARRAY_BUFFER_H__