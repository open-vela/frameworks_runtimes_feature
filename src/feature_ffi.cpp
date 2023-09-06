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
#include "feature_log.h"
#include "feature_utils.h"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdlib.h>

using namespace FEATURE;
using namespace ferry;

namespace ferry {

bool createHostValue(FeatureType featureType, void*& ptr, bool createPtrOnly)
{
    // special step: check if it is optional
    TRY_GET_REAL_TYPE(featureType);
    // handle type
    if (FT_IS_REFERENCE(featureType)) {
        if (!ptr) {
            // create raw pointer for interface
            bool isInterface = false;
            IS_INTERFACE_TYPE(featureType, isInterface);
            ptr = FTMalloc(sizeof(uintptr_t), isInterface ? FT_RAWPOINTER : FT_POINTER);
            if (createPtrOnly)
                return true;
            return createHostValue(FT_REMOVE_REFERENCE(featureType), *(void**)ptr);
        }
    }
    if (FT_IS_PRIMITIVE(featureType)) {
        switch (FT_GET_VALUE(featureType)) {
            case FT_VOID: {
                ptr = nullptr;
                return true;
            } break;
            case FT_INT: {
                ptr = FTMalloc(sizeof(int), featureType);
            } break;
            case FT_INT8: {
                ptr = FTMalloc(sizeof(int8_t), featureType);
            } break;
            case FT_UINT8: {
                ptr = FTMalloc(sizeof(uint8_t), featureType);
            } break;
            case FT_INT16: {
                ptr = FTMalloc(sizeof(int16_t), featureType);
            } break;
            case FT_UINT16: {
                ptr = FTMalloc(sizeof(uint16_t), featureType);
            } break;
            case FT_INT32: {
                ptr = FTMalloc(sizeof(int32_t), featureType);
            } break;
            case FT_UINT32: {
                ptr = FTMalloc(sizeof(uint32_t), featureType);
            } break;
            case FT_INT64: {
                ptr = FTMalloc(sizeof(int64_t), featureType);
            } break;
            case FT_UINT64: {
                ptr = FTMalloc(sizeof(uint64_t), featureType);
            } break;
            case FT_DOUBLE: {
                ptr = FTMalloc(sizeof(double), featureType);
            } break;
            case FT_FLOAT: {
                ptr = FTMalloc(sizeof(float), featureType);
            } break;
            case FT_BOOLEAN: {
                ptr = FTMalloc(sizeof(int), featureType);
            } break;
            case FT_CHAR: {
                // skip string space allocation, delay to value copy6
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
            case COMPLEX_STRUCT_MAP: {
                ptr = FTMalloc(complexType->size, featureType);
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* optionalType = (OptionalType*)complexType;
                if (!ptr) {
                    FEATURE_CHECK_EQ(FT_IS_REFERENCE(optionalType->type), true);
                    ptr = FTMalloc(sizeof(uintptr_t), FT_REMOVE_REFERENCE(optionalType->type));
                }
                if (!createHostValue(optionalType->type, ptr)) {
                    FEATURE_LOG_ERROR("create member pointered memory failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                // callback means cid
                ptr = FTMalloc(sizeof(FeatureCallbackId), FT_INT32);
            } break;
            case COMPLEX_ARRAY: {
                // array element not created at this point.
                ptr = FTMalloc(complexType->size, featureType);
            } break;
            case COMPLEX_PROMISE: {
                ptr = FTMalloc(sizeof(FeaturePromiseHandle), FT_INT32);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
        }
    }
    return true;
}

bool createTypeDeclaration(FeatureType featureType, ffi_type*& type)
{
    if (FT_IS_REFERENCE(featureType)) {
        type = &ffi_type_pointer;
        return true;
    }

    if (FT_IS_PRIMITIVE(featureType)) {
        switch (FT_GET_VALUE(featureType)) {
            case FT_VOID: {
                type = &ffi_type_void;
            } break;
            case FT_INT: {
                type = &ffi_type_sint;
            } break;
            case FT_INT8: {
                type = &ffi_type_sint8;
            } break;
            case FT_UINT8: {
                type = &ffi_type_uint8;
            } break;
            case FT_INT16: {
                type = &ffi_type_sint16;
            } break;
            case FT_UINT16: {
                type = &ffi_type_uint16;
            } break;
            case FT_INT32: {
                type = &ffi_type_sint32;
            } break;
            case FT_UINT32: {
                type = &ffi_type_uint32;
            } break;
            case FT_INT64: {
                type = &ffi_type_sint64;
            } break;
            case FT_UINT64: {
                type = &ffi_type_uint64;
            } break;
            case FT_FLOAT: {
                type = &ffi_type_float;
            } break;
            case FT_DOUBLE: {
                type = &ffi_type_double;
            } break;
            case FT_BOOLEAN: {
                type = &ffi_type_sint;
            } break;
            case FT_CHAR: {
                type = &ffi_type_pointer;
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
    } else if (FT_IS_COMPLEX(featureType)) {
        // fill members
        ComplexTypeHeader* complexHeader = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        switch (complexHeader->type) {
            case COMPLEX_STRUCT_MAP: {
                type = new ffi_type();
                ObjectMapType& objMapType = *(ObjectMapType*)complexHeader;
                // fill struct
                type->type = FFI_TYPE_STRUCT;
                type->alignment = 0;
                type->size = 0;
                ObjectMember* member = objMapType.members;
                auto member_count = countMember(member);
                ffi_type** ffi_members = new ffi_type*[member_count + 1];
                int i = 0;
                for (; i < member_count; i++) {
                    // process primitives
                    bool ret = createTypeDeclaration(objMapType.members[i].type, ffi_members[i]);
                    if (!ret) {
                        FEATURE_LOG_ERROR(
                            "prepareStructType for primitive type failed !!!!!!");
                        return false;
                    }
                }
                ffi_members[i] = nullptr;
                // fill members
                type->elements = ffi_members;
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* optionalType = (OptionalType*)complexHeader;
                if (!createTypeDeclaration(optionalType->type, type)) {
                    FEATURE_LOG_ERROR("create optional type declaration failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                type = &ffi_type_sint32;
            } break;
            case COMPLEX_ARRAY: {
                // FTArray
                type = new ffi_type();
                type->type = FFI_TYPE_STRUCT;
                type->alignment = 0;
                type->size = 0;
                type->elements = new ffi_type*[3];
                type->elements[0] = &ffi_type_sint32;
                type->elements[1] = &ffi_type_pointer;
                type->elements[2] = nullptr;
            } break;
            case COMPLEX_PROMISE: {
                type = &ffi_type_sint32;
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            } break;
        }
    }
    return true;
}

void freeTypeDeclaration(ffi_type*& type)
{
    if (!type)
        return;

    auto elem = type->elements;
    if (elem) {
        while (*elem) {
            freeTypeDeclaration(*elem);
            elem++;
        }
        delete[] type->elements;
        delete type;
        type = nullptr;
    }
}

void* exactVariadicParameter(va_list& ap, FeatureType featureType)
{
    void* result = nullptr;
    bool isPtr = FT_IS_REFERENCE(featureType);
    if (isPtr) {
        result = malloc(sizeof(void*));
        *(intptr_t*)result = va_arg(ap, intptr_t);
        return result;
    } else if (FT_IS_PRIMITIVE(featureType)) {
        switch (FT_GET_VALUE(featureType)) {
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
                result = malloc(sizeof(int));
                *(bool*)result = (bool)va_arg(ap, int);
            } break;
            case FT_CHAR: {
                result = malloc(sizeof(uintptr_t));
                *(const char**)result = va_arg(ap, const char*);
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return result;
            }
        }
    } else if (FT_IS_COMPLEX(featureType)) {
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        result = FTMalloc(complexType->size, featureType);
        switch (complexType->type) {
            case COMPLEX_STRUCT_MAP: {
                *(ObjectMapType*)result = va_arg(ap, ObjectMapType);
            } break;
            case COMPLEX_OPTIONAL: {
                FEATURE_CHECK(false && "do not support exact optional type !");
            } break;
            case COMPLEX_CALLBACK: {
                *(FeatureCallbackId*)result = va_arg(ap, FeatureCallbackId);
            } break;
            case COMPLEX_ARRAY: {

            } break;
            case COMPLEX_PROMISE: {
                *(FeaturePromiseHandle*)result = va_arg(ap, FeaturePromiseHandle);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return result;
            }
        }
    }
    return result;
}

} // namespace ferry
