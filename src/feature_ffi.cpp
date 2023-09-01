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
#include "feature.h"
#include "feature_framework.h"
#include "feature_instance.h"
#include "feature_context_qjs.h"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <ffi.h>
#include <functional>
#include <stdlib.h>

using namespace FEATURE;
using namespace ferry;

namespace ferry {
namespace FeatureFFI {

#define TRY_GET_REAL_TYPE(featureType)                                                    \
    if (FT_IS_COMPLEX(featureType)) {                                                     \
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType); \
        if (complexType->type == COMPLEX_OPTIONAL) {                                      \
            featureType = ((OptionalType*)complexType)->type;                             \
        }                                                                                 \
    }

    void* FTMalloc(size_t size, FEATURE::FeatureType featureType)
    {
        void* ptr = malloc(size + FT_OBJ_HEADER_SIZE);
        FTObjHeader* objHeader = (FTObjHeader*)ptr;
        objHeader->ref_count = 1;
        objHeader->featureType = featureType;
        ptr = (char*)ptr + FT_OBJ_HEADER_SIZE;
        memset(ptr, 0, size);
        return ptr;
    }

    namespace utils {
        inline int count_member(ObjectMember* member)
        {
            int count = 0;
            while (member->name) {
                count++;
                member++;
            }
            return count;
        }
    } // namespace utils

    int32_t getValueSize(FEATURE::FeatureType featureType)
    {
        if (FT_IS_REFERENCE(featureType)) {
            // alloc pointer pointed memory space
            return sizeof(uintptr_t);
        }
        if (FT_IS_PRIMITIVE(featureType)) {
            switch (FT_GET_VALUE(featureType)) {
            case FT_VOID: {
                return 0;
            } break;
            case FT_INT: {
                return sizeof(int);
            } break;
            case FT_INT8: {
                return sizeof(int8_t);
            } break;
            case FT_UINT8: {
                return sizeof(uint8_t);
            } break;
            case FT_INT16: {
                return sizeof(int16_t);
            } break;
            case FT_UINT16: {
                return sizeof(uint16_t);
            } break;
            case FT_INT32: {
                return sizeof(int32_t);
            } break;
            case FT_UINT32: {
                return sizeof(uint32_t);
            } break;
            case FT_INT64: {
                return sizeof(int64_t);
            } break;
            case FT_UINT64: {
                return sizeof(uint64_t);
            } break;
            case FT_DOUBLE: {
                return sizeof(double);
            } break;
            case FT_FLOAT: {
                return sizeof(float);
            } break;
            case FT_BOOLEAN: {
                return sizeof(int32_t);
            } break;
            case FT_CHAR: {
                // return 0 for string buffer size.
                return 0;
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return 0;
            }
            }
        } else if (FT_IS_COMPLEX(featureType)) {
            // allocate complex type
            ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
            return complexType->size;
        } else {
            return 0;
        }
    }

    bool createHostValue(FEATURE::FeatureType featureType, void*& ptr, bool createPtrOnly)
    {
        // special step: check if it is optional
        TRY_GET_REAL_TYPE(featureType);
        // handle type
        if (FT_IS_REFERENCE(featureType)) {
            if (!ptr) {
                ptr = FTMalloc(sizeof(uintptr_t), FT_POINTER);
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
                ptr = FTMalloc(sizeof(FEATURE::FeatureCallbackId), FT_INT32);
            } break;
            case COMPLEX_ARRAY: {
                // array element not created at this point.
                ptr = FTMalloc(complexType->size, featureType);
            } break;
            case COMPLEX_PROMISE: {
                ptr = FTMalloc(sizeof(FEATURE::FeaturePromiseHandle), FT_INT32);
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
                auto member_count = utils::count_member(member);
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

    bool convertValueToHost(FeatureInstance* instance, FEATURE::FeatureType featureType, void*& ptr,
        context_ref ctx, feature_value_t value)
    {
        // special step: get real type of complex type
        TRY_GET_REAL_TYPE(featureType);
        if (!ptr) {
            if (!createHostValue(featureType, ptr)) {
                FEATURE_LOG_ERROR("create host value failed !");
                return false;
            }
        }
        if (FT_IS_REFERENCE(featureType)) {
            void*& value_ptr = *(void**)ptr;
            if (!convertValueToHost(instance, FT_REMOVE_REFERENCE(featureType), value_ptr, ctx, value)) {
                FEATURE_LOG_ERROR("convert value to host failed !");
                return false;
            }
            return true;
        }
        if (FT_IS_PRIMITIVE(featureType)) {
            switch (FT_GET_VALUE(featureType)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            } break;
            case FT_INT: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                if (!feature_to_int(ctx, (int32_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to int32 failed !");
                    return false;
                }
            } break;
            case FT_INT8: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }

                void* int32_ptr = nullptr;
                int32_ptr = malloc(sizeof(int32_t));
                feature_to_int(ctx, (int32_t*)int32_ptr, value);

                // back to int8
                int8_t d = static_cast<int8_t>(*(int32_t*)int32_ptr);
                (*(int8_t*)ptr) = d;
                free(int32_ptr);
                int32_ptr = nullptr;
                FEATURE_LOG_DEBUG("ptr is %d !", *(int8_t*)ptr);
            } break;
            case FT_UINT8: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                void* uint32_ptr = nullptr;
                uint32_ptr = malloc(sizeof(uint32_t));
                feature_to_uint(ctx, (uint32_t*)uint32_ptr, value);

                // back to uint8
                uint8_t d = static_cast<uint8_t>(*(uint32_t*)uint32_ptr);
                (*(uint8_t*)ptr) = d;
                free(uint32_ptr);
                uint32_ptr = nullptr;
                //打印uint8_t类型的值
                FEATURE_LOG_DEBUG("ptr is %d !", *(uint8_t*)ptr);
            } break;
            case FT_INT16: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                void* int32_ptr = nullptr;
                int32_ptr = malloc(sizeof(int32_t));
                feature_to_int(ctx, (int32_t*)int32_ptr, value);

                // back to int16
                int16_t d = static_cast<int16_t>(*(int32_t*)int32_ptr);
                (*(int16_t*)ptr) = d;
                free(int32_ptr);
                int32_ptr = nullptr;
                FEATURE_LOG_DEBUG("ptr is %d !", *(int16_t*)ptr);
            } break;
            case FT_UINT16: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }

                void* uint32_ptr = nullptr;
                uint32_ptr = malloc(sizeof(uint32_t));
                feature_to_uint(ctx, (uint32_t*)uint32_ptr, value);

                // back to uint16
                uint16_t d = static_cast<uint16_t>(*(uint32_t*)uint32_ptr);
                (*(uint16_t*)ptr) = d;
                free(uint32_ptr);
                uint32_ptr = nullptr;
                FEATURE_LOG_DEBUG("ptr is %d !", *(uint16_t*)ptr);
            } break;
            case FT_INT32: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                if (!feature_to_int(ctx, (int32_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to int32 failed !");
                    return false;
                }
            } break;
            case FT_UINT32: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                if (!feature_to_uint(ctx, (uint32_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to uint32 failed !");
                    return false;
                }
            } break;
            case FT_INT64: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                if (!feature_to_int64(ctx, (int64_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to int64 failed !");
                    return false;
                }
            } break;
            case FT_UINT64: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                if (!feature_to_uint64(ctx, (uint64_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to uint64 failed !");
                    return false;
                }
            } break;
            case FT_FLOAT: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }

                void* double_ptr = nullptr;
                double_ptr = malloc(sizeof(double));
                feature_to_double(ctx, (double*)double_ptr, value);
                // back to float
                float d = static_cast<float>(*(double*)double_ptr);
                (*(float*)ptr) = d;
                free(double_ptr);
                double_ptr = nullptr;
                FEATURE_LOG_DEBUG("ptr is %f !", *(float*)ptr);
            } break;
            case FT_DOUBLE: {
                if (!feature_is_number(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number !");
                    return false;
                }
                if (!feature_to_double(ctx, (double*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to double failed !");
                    return false;
                }
            } break;
            case FT_BOOLEAN: {
                if (!feature_is_boolean(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need boolean !");
                    return false;
                }
                if (!feature_to_boolean(ctx, (bool*)ptr, value)) {
                    FEATURE_LOG_ERROR("arg to boolean failed !");
                }
            } break;
            case FT_CHAR: {
                if (!feature_is_string(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need string !");
                    return false;
                }
                const char* str = feature_to_cstring(ctx, value);
                char* alloc_ptr = (char*)FTMalloc(strlen(str) + 1, FT_CHAR);
                strcpy(alloc_ptr, str);
                ptr = alloc_ptr;
                feature_free_cstring(ctx, str);
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
            }
        } else if (FT_IS_COMPLEX(featureType)) {
            ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
            switch (complexType->type) {
            case COMPLEX_STRUCT_MAP: {
                ObjectMapType& objMapType = *(ObjectMapType*)complexType;
                auto member_count = utils::count_member(objMapType.members);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    auto member = &objMapType.members[i];
                    bool ret;
                    void* member_ptr = (void*)((char*)ptr + member->offset);
                    feature_value_t propValue = feature_get_object_property(ctx, value, member->name);
                    //check propValue is js_undefined or not
                    if (feature_is_undefined(propValue)) {
                        if (FT_IS_COMPLEX(member->type)) {
                            ComplexTypeHeader* complexType1 = (ComplexTypeHeader*)FT_GET_COMPLEX(member->type);
                            if (complexType1->type == COMPLEX_OPTIONAL) {
                                FEATURE_LOG_DEBUG("propValue is undefined will get value with optinalType!");
                                OptionalType* optinalType = (OptionalType*)complexType1;
                                ret = convertValueToGuest(instance, optinalType->type, &optinalType->fval, ctx, propValue);
                                if (!ret) {
                                    feature_free_value(ctx, propValue);
                                    propValue = FEATURE_UNDEFINED;
                                    FEATURE_LOG_ERROR("propValue convert optional failed!");
                                    return false;
                                }
                            }
                        } else {
                            FEATURE_LOG_DEBUG("COMPLEX_STRUCT_MAP member->type is %d!", member->type);
                        }
                    }

                    ret = convertValueToHost(instance, member->type, member_ptr, ctx, propValue);
                    feature_free_value(ctx, propValue);
                    if (!ret) {
                        FEATURE_LOG_ERROR("get property value for key: %s failed !",
                            member->name);
                        return false;
                    }
                }
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* optionalType = (OptionalType*)complexType;
                bool ret = convertValueToHost(instance, optionalType->type, ptr, ctx, value);
                if (!ret) {
                    FEATURE_LOG_ERROR("convert optional type failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                // save into instance
                CallbackType* callbackType = (CallbackType*)complexType;
                ft_value_t ft_val;
                auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(ft_val);
               *js_val_ptr = value;
                FEATURE::FeatureCallbackId id = instance->addCallback(ft_val, callbackType);
                *(FeatureCallbackId*)ptr = id; // write callback id to pointer.
            } break;
            case COMPLEX_ARRAY: {
                ArrayType& arrayType = *(ArrayType*)complexType;
                auto element_type = arrayType.element_type;
                FEATURE_CHECK_EQ(feature_is_array(ctx, value), true);
                auto len = feature_get_array_length(ctx, value);
                FTArray* arrayData = (FTArray*)ptr;
                arrayData->_size = len;
                if (len) {
                    // we support reference and primitive types
                    size_t element_size = FT_IS_REFERENCE(element_type) ? sizeof(uintptr_t) : getValueSize(element_type);
                    auto size = element_size * len;
                    FEATURE_CHECK_NE(size, 0);
                    arrayData->_element = malloc(size);
                    memset(arrayData->_element, 0, size);
                    for (size_t i = 0; i < len; i++) {
                        // fill it
                        feature_value_t elementValue = feature_get_array_idx_safe(ctx, value, i);
                        FEATURE_CHECK_NE(feature_is_undefined(elementValue), true);
                        void* element_ptr = ((char*)arrayData->_element + element_size * i);
                        if (!convertValueToHost(instance, element_type, element_ptr, ctx, elementValue)) {
                            FEATURE_LOG_ERROR("convert array element failed ");
                            feature_free_value(ctx, elementValue);
                            break;
                        }
                        feature_free_value(ctx, elementValue);
                    }
                }
                FEATURE_LOG_DEBUG("array data: %p", ptr);
            } break;
            case COMPLEX_PROMISE: {
                FEATURE_LOG_ERROR("do not support convert promise to guest !");
                return false;
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
            }
        }
        return true;
    }

    bool convertValueToGuest(FeatureInstance* instance, FEATURE::FeatureType featureType, void* ptr,
        context_ref ctx, feature_value_t& value)
    {
        FEATURE_CHECK_NE(ptr, nullptr);
        bool isRef = FT_IS_REFERENCE(featureType);
        if (isRef) {
            ptr = *(void**)ptr;
        }
        if (FT_IS_PRIMITIVE(featureType)) {
            switch (FT_GET_VALUE(featureType)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            } break;
            case FT_INT: {
                value = feature_int(ctx, *((int32_t*)ptr));
            } break;
            case FT_INT8: {
                value = feature_int(ctx, *((int8_t*)ptr));
            } break;
            case FT_UINT8: {
                value = feature_uint(ctx, *((uint8_t*)ptr));
            } break;
            case FT_INT16: {
                value = feature_int(ctx, *((int16_t*)ptr));
            } break;
            case FT_UINT16: {
                value = feature_uint(ctx, *((uint16_t*)ptr));
            } break;
            case FT_INT32: {
                value = feature_int(ctx, *((int32_t*)ptr));
            } break;
            case FT_UINT32: {
                value = feature_uint(ctx, *((uint32_t*)ptr));
            } break;
            case FT_INT64: {
                value = feature_int64(ctx, *((int64_t*)ptr));
            } break;
            case FT_UINT64: {
                value = feature_uint64(ctx, *((uint64_t*)ptr));
            } break;
            case FT_FLOAT: {
                value = feature_double(ctx, *((float*)ptr));
            } break;
            case FT_DOUBLE: {
                value = feature_double(ctx, *((double*)ptr));
            } break;
            case FT_BOOLEAN: {
                value = feature_boolean(ctx, *((bool*)ptr));
            } break;
            case FT_CHAR: {
                value = feature_string(ctx, (const char*)ptr);
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
            }
        } else if (FT_IS_COMPLEX(featureType)) {
            ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
            switch (complexType->type) {
            case COMPLEX_STRUCT_MAP: {
                ObjectMapType& objMapType = *(ObjectMapType*)complexType;
                auto member = objMapType.members;
                auto member_count = utils::count_member(member);
                value = feature_object(ctx);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    void* member_ptr = (void*)((char*)ptr + member->offset);
                    feature_value_t prop;
                    bool ret = convertValueToGuest(instance, member->type, member_ptr, ctx, prop);
                    if (!ret) {
                        feature_free_value(ctx, prop);
                        FEATURE_LOG_ERROR("convert property name: %s failed !", member->name);
                        return false;
                    }
                    feature_set_object_property(ctx, value, member->name, prop);
                    member++;
                }
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* optinalType = (OptionalType*)complexType;
                bool ret = convertValueToGuest(instance, optinalType->type, &optinalType->fval, ctx, value);
                if (!ret) {
                    feature_free_value(ctx, value);
                    value = FEATURE_UNDEFINED;
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
                ArrayType* arrayType = (ArrayType*)complexType;
                FTArray* arrayData = (FTArray*)ptr;
                auto element_type = arrayType->element_type;
                FEATURE_CHECK_EQ(FT_IS_REFERENCE(element_type), true);
                size_t element_size = sizeof(uintptr_t);
                // exact and create js value
                value = feature_array(ctx);
                for (int32_t i = 0; i < arrayData->_size; i++) {
                    void* element_ptr = ((char*)arrayData->_element + element_size * i);
                    // convert element value
                    feature_value_t element_obj = FEATURE_UNDEFINED;
                    if (!convertValueToGuest(instance, element_type, element_ptr, ctx, element_obj)) {
                        FEATURE_LOG_ERROR("convert array element to guest failed !");
                        feature_free_value(ctx, element_obj);
                        feature_free_value(ctx, value);
                        return false;
                    }
                    feature_set_array_idx(static_cast<feature_context_ref>(ctx), value, i, element_obj);
                }
            } break;
            case COMPLEX_PROMISE: {
                // convert to guest means return promise object back.
                //PromiseType* promiseType = (PromiseType*)complexType;
                // get promise data back.
                if (!instance) {
                    FEATURE_LOG_ERROR("convert promise need instance provided !");
                }
                FEATURE_CHECK_NE(instance, nullptr);
                FEATURE::FeaturePromiseHandle promiseHandle = *(FEATURE::FeaturePromiseHandle*)ptr;
                auto promiseData = instance->getPromise(promiseHandle);
                if (!promiseData) {
                    FEATURE_LOG_ERROR("get promise with promiseHandle: %" PRId32 " failed !", promiseHandle);
                    return false;
                }
                auto js_val = FT_VAL_GET_JS_VAL(promiseData->promise);
                value = feature_dup_value(ctx, js_val);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
            }
        }
        return true;
    }

    void* exactVariadicParameter(va_list& ap, FEATURE::FeatureType featureType)
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
                *(FEATURE::FeatureCallbackId*)result = va_arg(ap, FEATURE::FeatureCallbackId);
            } break;
            case COMPLEX_ARRAY: {

            } break;
            case COMPLEX_PROMISE: {
                *(FEATURE::FeaturePromiseHandle*)result = va_arg(ap, FEATURE::FeaturePromiseHandle);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return result;
            }
            }
        }
        return result;
    }
} // namespace FeatureFFI
} // namespace ferry

namespace FEATURE {
void FreeFeatureValue(void* ptr)
{
    if (!ptr)
        return;
    void* header_ptr = ((char*)ptr - FT_OBJ_HEADER_SIZE);
    ferry::FTObjHeader* header = (ferry::FTObjHeader*)header_ptr;
    if (--header->ref_count > 0) {
        // free
        return;
    }
    FeatureType featureType = header->featureType;

    // free pointer refers memory
    if (FT_IS_REFERENCE(featureType)) {
        // we do not support reference reference.
        FreeFeatureValue(*(void**)ptr);
        // ptr space is allocated outside, it's callers responsibility to free it
        free(header);
        return;
    }
    if (FT_IS_COMPLEX(featureType)) {
        ComplexTypeHeader* complexType1 = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        switch (complexType1->type) {
        case COMPLEX_STRUCT_MAP: {
            ObjectMapType& objMapType = *(ObjectMapType*)complexType1;
            auto member_count = ferry::FeatureFFI::utils::count_member(objMapType.members);
            for (int i = 0; i < member_count; i++) {
                ferry::ObjectMember* member = &objMapType.members[i];
                //FEATURE::FeatureType member_type = member->type;
                auto member_type = member->type;
                TRY_GET_REAL_TYPE(member_type);
                if (FT_IS_REFERENCE(member_type)) {
                    void* member_ptr = (void*)((char*)ptr + member->offset);
                    FreeFeatureValue(*(void**)member_ptr);
                }
            }
            // TODO: if we can free the ptr? it may not be allocated by malloc().
            // Maybe we can check the last bit of the pointer to determinte if it's allocated by us.
            free(header);
        } break;
        case COMPLEX_OPTIONAL: {
            FreeFeatureValue(ptr);
        } break;
        case COMPLEX_CALLBACK: {

        } break;
        case COMPLEX_ARRAY: {
            // free array elements and ptr
            ArrayType& arrayType = *(ArrayType*)complexType1;
            auto element_type = arrayType.element_type;
            FTArray* arrayData = (FEATURE::FTArray*)ptr;
            // only support reference as element
            if (FT_IS_REFERENCE(element_type)) {
                size_t element_size = sizeof(uintptr_t);
                for (int32_t i = 0; i < arrayData->_size; i++) {
                    void* element_ptr = (char*)arrayData->_element + element_size * i;
                    if (element_ptr) {
                        // free it.
                        FreeFeatureValue(*(void**)element_ptr);
                    }
                }
            }
            free(arrayData->_element);
            free(header);
        } break;
        case COMPLEX_PROMISE: {

        } break;
        default: {
            FEATURE_LOG_ERROR("unsupported type !");
        } break;
        }
    } else {
        free(header);
    }
}

}
