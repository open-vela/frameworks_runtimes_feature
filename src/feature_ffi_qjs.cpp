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
#include "feature_ffi_qjs.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "feature_instance_qjs.h"
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

namespace FeatureFFIQjs {

bool convertValueToHost(FeatureInstance* instance, FeatureType featureType, void*& ptr,
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
                auto member_count = countMember(objMapType.members);
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
                FeatureCallbackId id = ((FeatureInstanceQjs*)instance)->addCallback(value, callbackType);
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

bool convertValueToGuest(FeatureInstance* instance, FeatureType featureType, void* ptr,
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
                auto member_count = countMember(member);
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
                FeaturePromiseHandle promiseHandle = *(FeaturePromiseHandle*)ptr;
                auto promiseData = ((FeatureInstanceQjs*)instance)->getPromise(promiseHandle);
                if (!promiseData) {
                    FEATURE_LOG_ERROR("get promise with promiseHandle: %" PRId32 " failed !", promiseHandle);
                    return false;
                }
                value = feature_dup_value(ctx, promiseData->promise);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
        }
    }
    return true;
}

}
} // namespace ferry
