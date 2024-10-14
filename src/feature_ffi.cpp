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

#include "feature_ffi.h"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdlib.h>

namespace feature_framework {

bool createHostValue(FeatureType featureType, void*& ptr)
{
    // check if ptr have been malloc
    if (ptr) {
        FEATURE_LOG_WARN("ptr: %p already exist !", ptr);
        return true;
    }
    auto flags = FT_GET_FLAG(featureType);
    if (flags & TYPE_FLAGS_POINTER) {
        FEATURE_LOG_DEBUG("pointer don't need creating!");
        return true;
    } else if (FT_IS_PRIMITIVE(featureType)) {
        switch (featureType) {
        case FT_VOID: {
            ptr = nullptr;
            return true;
        } break;
        case FT_INT: {
            ptr = FeatureMalloc(sizeof(int), featureType);
        } break;
        case FT_INT8: {
            ptr = FeatureMalloc(sizeof(int8_t), featureType);
        } break;
        case FT_UINT8: {
            ptr = FeatureMalloc(sizeof(uint8_t), featureType);
        } break;
        case FT_INT16: {
            ptr = FeatureMalloc(sizeof(int16_t), featureType);
        } break;
        case FT_UINT16: {
            ptr = FeatureMalloc(sizeof(uint16_t), featureType);
        } break;
        case FT_INT32: {
            ptr = FeatureMalloc(sizeof(int32_t), featureType);
        } break;
        case FT_UINT32: {
            ptr = FeatureMalloc(sizeof(uint32_t), featureType);
        } break;
        case FT_INT64: {
            ptr = FeatureMalloc(sizeof(int64_t), featureType);
        } break;
        case FT_UINT64: {
            ptr = FeatureMalloc(sizeof(uint64_t), featureType);
        } break;
        case FT_DOUBLE: {
            ptr = FeatureMalloc(sizeof(double), featureType);
        } break;
        case FT_FLOAT: {
            ptr = FeatureMalloc(sizeof(float), featureType);
        } break;
        case FT_BOOLEAN: {
            ptr = FeatureMalloc(sizeof(bool), featureType);
        } break;
        case FT_STRING: {
            ptr = FeatureMalloc(sizeof(uintptr_t), FT_STRING);
        } break;
        case FT_ANY_REF: {
            // skip anyref space allocation, delay to value copy
        } break;
        default: {
            FEATURE_LOG_WARN("unsupported type detected !");
            return false;
        }
        }
    } else if (FT_IS_COMPLEX(featureType)) {
        // allocate complex type
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        switch (complexType->type) {
            // map and array is reference
        case COMPLEX_STRUCT_MAP: {
            // ptr = FeatureMalloc(sizeof(uintptr_t), FT_POINTER);
        } break;
        case COMPLEX_OPTIONAL: {
            OptionalType* optionalType = (OptionalType*)complexType;
            if (!createHostValue(optionalType->type, ptr)) {
                FEATURE_LOG_ERROR("create optional type: %d failed !", optionalType->type);
                return false;
            }
        } break;
        case COMPLEX_CALLBACK: {
            // callback means cid
            ptr = FeatureMalloc(sizeof(FtCallbackId), FT_INT32);
        } break;
        case COMPLEX_ARRAY: {
            // malloc array pointer
            // ptr = FeatureMalloc(sizeof(uintptr_t), FT_POINTER);
        } break;
        case COMPLEX_PROMISE: {
            ptr = FeatureMalloc(sizeof(FtPromiseId), FT_INT32);
        } break;
        case COMPLEX_INTERFACE: {
            // interface do not need create
            ptr = nullptr;
        } break;
        default: {
            FEATURE_LOG_ERROR("unsupported complex type !");
            return false;
        }
        }
    }
    return true;
}

void* extractVariadicParam(va_list& ap, FeatureType featureType)
{
    void* result = nullptr;
    bool isPtr = FT_IS_REFERENCE(featureType);
    if (isPtr) {
        result = malloc(sizeof(void*));
        *(intptr_t*)result = va_arg(ap, intptr_t);
        return result;
    }
    if (FT_IS_PRIMITIVE(featureType)) {
        switch (featureType) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void not supported !");
            return result;
        } break;
        case FT_INT: {
            result = malloc(sizeof(int));
            *(int*)result = va_arg(ap, int);
        } break;
        case FT_INT8: {
            void* result_int = nullptr;
            result_int = malloc(sizeof(int));
            (*(int*)result_int) = va_arg(ap, int);
            // back to int8
            int8_t d = static_cast<int8_t>(*(int*)result_int);
            result = malloc(sizeof(int8_t));
            (*(int8_t*)result) = d;
            free(result_int);
            result_int = nullptr;
            FEATURE_LOG_DEBUG("result is %d !", *(int8_t*)result);
        } break;
        case FT_UINT8: {
            void* result_int = nullptr;
            result_int = malloc(sizeof(uint));
            (*(uint*)result_int) = va_arg(ap, uint);
            // back to uint8
            uint8_t d = static_cast<uint8_t>(*(uint*)result_int);
            result = malloc(sizeof(uint8_t));
            (*(uint8_t*)result) = d;
            free(result_int);
            result_int = nullptr;
            FEATURE_LOG_DEBUG("result is %d !", *(uint8_t*)result);
        } break;
        case FT_INT16: {
            void* result_int = nullptr;
            result_int = malloc(sizeof(int));
            (*(int*)result_int) = va_arg(ap, int);
            // back to int16
            int16_t d = static_cast<int16_t>(*(int*)result_int);
            result = malloc(sizeof(int16_t));
            (*(int16_t*)result) = d;
            free(result_int);
            result_int = nullptr;
            FEATURE_LOG_DEBUG("result is %d !", *(int16_t*)result);
        } break;
        case FT_UINT16: {
            void* result_int = nullptr;
            result_int = malloc(sizeof(uint));
            (*(uint*)result_int) = va_arg(ap, uint);
            // back to uint16
            uint16_t d = static_cast<uint16_t>(*(uint*)result_int);
            result = malloc(sizeof(uint16_t));
            (*(uint16_t*)result) = d;
            free(result_int);
            result_int = nullptr;
            FEATURE_LOG_DEBUG("result is %d !", *(uint16_t*)result);
        } break;
        case FT_INT32: {
            result = malloc(sizeof(int32_t));
            *(int32_t*)result = va_arg(ap, int32_t);
        } break;
        case FT_UINT32: {
            result = malloc(sizeof(uint32_t));
            *(uint32_t*)result = va_arg(ap, uint32_t);
        } break;
        case FT_INT64: {
            result = malloc(sizeof(int64_t));
            *(int64_t*)result = va_arg(ap, int64_t);
        } break;
        case FT_UINT64: {
            result = malloc(sizeof(uint64_t));
            *(uint64_t*)result = va_arg(ap, uint64_t);
        } break;
        case FT_FLOAT: {
            void* result_double = nullptr;
            result_double = malloc(sizeof(double));
            (*(double*)result_double) = va_arg(ap, double);
            // back to float
            float d = static_cast<float>(*(double*)result_double);
            result = malloc(sizeof(float));
            (*(float*)result) = d;
            free(result_double);
            result_double = nullptr;
            FEATURE_LOG_DEBUG("result is %f !", *(float*)result);
        } break;
        case FT_DOUBLE: {
            result = malloc(sizeof(double));
            *(double*)result = va_arg(ap, double);
        } break;
        case FT_BOOLEAN: {
            void* result_int = nullptr;
            result_int = malloc(sizeof(int));
            (*(int*)result_int) = va_arg(ap, int);
            // back to bool
            bool d = static_cast<bool>(*(int*)result_int);
            result = malloc(sizeof(bool));
            (*(bool*)result) = d;
            free(result_int);
            result_int = nullptr;
            FEATURE_LOG_DEBUG("result is %d !", *(bool*)result);
        } break;
        case FT_STRING: {
            result = malloc(sizeof(uintptr_t));
            *(const char**)result = va_arg(ap, const char*);
        } break;
        case FT_ANY_REF: {
            result = malloc(sizeof(uintptr_t));
            *(ft_value_t**)result = va_arg(ap, ft_value_t*);
        } break;
        default: {
            FEATURE_LOG_WARN("unsupported type detected !");
            return result;
        }
        }
    } else if (FT_IS_COMPLEX(featureType)) {
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        result = FeatureMalloc(complexType->size, featureType);
        switch (complexType->type) {
        case COMPLEX_STRUCT_MAP: {
            *(ObjectMapType*)result = va_arg(ap, ObjectMapType);
        } break;
        case COMPLEX_OPTIONAL: {
            FEATURE_CHECK(false, "do not support exact optional type !");
        } break;
        case COMPLEX_CALLBACK: {
            *(FtCallbackId*)result = va_arg(ap, FtCallbackId);
        } break;
        case COMPLEX_ARRAY: {

        } break;
        case COMPLEX_PROMISE: {
            *(FtPromiseId*)result = va_arg(ap, FtPromiseId);
        } break;
        default: {
            FEATURE_LOG_ERROR("unsupported complex type !");
            return result;
        }
        }
    }
    return result;
}

} // namespace feature_framework
