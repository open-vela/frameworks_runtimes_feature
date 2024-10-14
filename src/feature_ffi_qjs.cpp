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
#include "feature_context_qjs.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_prototype.h"
#include "feature_utils.h"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdlib.h>

namespace feature_framework {

namespace FeatureFFIQjs {

    static void* interface_from_target(feature_value_t& target)
    {
        auto opaque = feature_get_opaque(target, FeatureManagerQjs::jsClassId());
        FEATURE_LOG_DEBUG("value: %p, get opaque: %p", JS_VALUE_GET_PTR(target), opaque);
        FEATURE_CHECK_NE(opaque, nullptr);
        return opaque;
    }

    static feature_value_t target_from_interface(FeatureInstance* instance)
    {
        return ((FeatureInstanceQjs*)instance)->dupTarget();
    }

    bool convertValueToHost(FeatureInstance* instance, FeatureType featureType, void*& ptr,
        context_ref ctx, feature_value_t value)
    {
        FEATURE_CHECK_NE(ptr, nullptr);
        if (FT_IS_PRIMITIVE(featureType)) {
            switch (featureType) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            } break;
            case FT_INT: {
                if (JS_VALUE_GET_TAG(value) != JS_TAG_INT) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with int !");
                    return false;
                }
                if (!feature_to_int(ctx, (int32_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to int32 failed !");
                    return false;
                }
            } break;
            case FT_INT8: {
                if ((JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with int8 !");
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
                if (!(JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with uint8 !");
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
                FEATURE_LOG_DEBUG("ptr is %d !", *(uint8_t*)ptr);
            } break;
            case FT_INT16: {
                if ((JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with int16 !");
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
                if ((JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with uint16 !");
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
                if ((JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with int32 !");
                    return false;
                }
                if (!feature_to_int(ctx, (int32_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to int32 failed !");
                    return false;
                }
            } break;
            case FT_UINT32: {
                if ((JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with uint32 !");
                    return false;
                }
                if (!feature_to_uint(ctx, (uint32_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to uint32 failed !");
                    return false;
                }
            } break;
            case FT_INT64: {
                if ((JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with int64 !");
                    return false;
                }
                if (!feature_to_int64(ctx, (int64_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to int64 failed !");
                    return false;
                }
            } break;
            case FT_UINT64: {
                if ((JS_VALUE_GET_TAG(value) != JS_TAG_INT)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need number with uint64 !");
                    return false;
                }
                if (!feature_to_uint64(ctx, (uint64_t*)ptr, value)) {
                    FEATURE_LOG_ERROR("convert to uint64 failed !");
                    return false;
                }
            } break;
            case FT_FLOAT: {
                if (JS_VALUE_GET_TAG(value) != JS_TAG_FLOAT64) {
                    FEATURE_LOG_ERROR("arg type mismatch, need float !");
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
                if (JS_VALUE_GET_TAG(value) != JS_TAG_FLOAT64) {
                    FEATURE_LOG_ERROR("arg type mismatch, need double !");
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
            case FT_STRING: {
                if (feature_is_null(value) || feature_is_undefined(value)) {
                    FEATURE_LOG_DEBUG("string arg is null or undefined!");
                    *(const char**)ptr = NULL;
                } else if (!feature_is_string(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need string !");
                    return false;
                } else {
                    const char* str = feature_to_cstring(ctx, value);
                    char* alloc_ptr = (char*)FeatureMalloc(strlen(str) + 1, FT_STRING);
                    strcpy(alloc_ptr, str);
                    *(const char**)ptr = alloc_ptr;
                    feature_free_cstring(ctx, str);
                }
            } break;
            case FT_ANY_REF: {
                if (feature_is_null(value) || feature_is_undefined(value)) {
                    FEATURE_LOG_DEBUG("object is null or undefined!");
                    *(ft_value_t**)ptr = NULL;
                } else {
                    // copy value
                    ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
                    qjs_val_t* q_val = (qjs_val_t*)f_val;
                    q_val->js_val = value;
                    *(ft_value_t**)ptr = f_val;
                }
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
                if (feature_is_undefined(value)) {
                    FEATURE_LOG_WARN("js struct is undefined!");
                    break;
                }
                if (feature_is_null(value)) {
                    FEATURE_LOG_WARN("js struct is null!");
                    break;
                }
                ObjectMapType& objMapType = *(ObjectMapType*)complexType;
                auto member_count = countMember(objMapType.members);
                if (!*(void**)ptr) {
                    *(void**)ptr = FeatureMalloc(complexType->size, featureType);
                }
                void* inner_ptr = *(void**)ptr;
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    auto member = &objMapType.members[i];
                    bool ret;
                    void* member_ptr = (void*)((char*)inner_ptr + member->offset);
                    feature_value_t propValue = feature_get_object_property(ctx, value, member->name);
                    // check propValue is js_undefined or not
                    if (feature_is_undefined(propValue)) {
                        if (FT_IS_COMPLEX(member->type)) {
                            ComplexTypeHeader* cmplx_type = (ComplexTypeHeader*)FT_GET_COMPLEX(member->type);
                            if (cmplx_type->type == COMPLEX_OPTIONAL) {
                                FEATURE_LOG_DEBUG("propValue is undefined will get value with optinalType!");
                                OptionalType* optinalType = (OptionalType*)cmplx_type;
                                ret = convertValueToGuest(optinalType->type, &optinalType->fval, ctx, propValue);
                                if (!ret) {
                                    feature_free_value(ctx, propValue);
                                    propValue = FEATURE_UNDEFINED;
                                    FEATURE_LOG_ERROR("propValue convert optional failed!");
                                    return false;
                                }
                            }
                        } else {
                            if ((member->type != FT_ANY_REF) && (member->type != FT_STRING)) {
                                FEATURE_LOG_ERROR("struct member with type '%d' missing!", member->type);
                                return false;
                            }
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
                if (feature_is_undefined(value)) {
                    FEATURE_LOG_DEBUG("js callback is undefined!");
                    break;
                } else if (feature_is_null(value)) {
                    FEATURE_LOG_DEBUG("js callback is null!");
                    break;
                } else if (!feature_is_object(value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need callback function !");
                    return false;
                }
                // save into instance
                CallbackType* callbackType = (CallbackType*)complexType;
                auto callback_manager = static_cast<FeatureInstanceQjs*>(instance);
                FtCallbackId id = callback_manager->addCallback(value, callbackType);
                if (callback_manager->getCallbacks().size() >= 30) {
                    FEATURE_LOG_ERROR("callback count of instance[%p] is %d, which is larger than 30, feature name is[%s]",
                        instance,
                        callback_manager->getCallbacks().size(),
                        instance->description()->name);
                }
                *(FtCallbackId*)ptr = id; // write callback id to pointer.
            } break;
            case COMPLEX_ARRAY: {
                ArrayType& arrayType = *(ArrayType*)complexType;
                auto element_type = arrayType.element_type;
                if (!feature_is_array(ctx, value)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need array !");
                    return false;
                }
                auto len = feature_get_array_length(ctx, value);
                // FtArray must be a pointer
                if (!*(void**)ptr) {
                    // malloc FtArray struct
                    *(void**)ptr = FeatureMalloc(sizeof(FtArray), featureType);
                }
                FtArray* arrayData = *(FtArray**)ptr;
                arrayData->_size = len;
                arrayData->_element = nullptr;
                if (len) {
                    // we support reference and primitive types
                    size_t element_size = getValueSize(element_type);
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
                            return false;
                        }
                        feature_free_value(ctx, elementValue);
                    }
                }
                FEATURE_LOG_DEBUG("array data: %p", ptr);
            } break;
            case COMPLEX_PROMISE: {
                FEATURE_LOG_ERROR("do not support convert promise to host !");
                return false;
            } break;
            case COMPLEX_INTERFACE: {
                *(void**)ptr = interface_from_target(value);
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
            }
        }
        return true;
    }

    bool convertValueToGuest(FeatureType featureType, void* ptr,
        context_ref ctx, feature_value_t& value)
    {
        if (!ptr) {
            FEATURE_LOG_ERROR("ptr is null and return false!");
            return false;
        }
        if (FT_IS_PRIMITIVE(featureType)) {
            switch (featureType) {
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
            case FT_STRING: {
                if (!*(char**)ptr) {
                    value = feature_string(ctx, "");
                } else {
                    char* str = *(char**)ptr;
                    value = feature_string(ctx, str);
                }
            } break;
            case FT_ANY_REF: {
                if (!*(ft_value_t**)ptr) {
                    value = JS_NULL;
                } else {
                    ft_value_t* f_val = *(ft_value_t**)(ptr);
                    value = FT_VAL_GET_JS_VAL(*f_val);
                    feature_dup_value(ctx, value);
                }
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
                void* struct_ptr = *(void**)ptr;
                if (!struct_ptr) {
                    FEATURE_LOG_WARN("null struct ptr!");
                    value = FEATURE_UNDEFINED;
                    break;
                }
                ObjectMapType& objMapType = *(ObjectMapType*)complexType;
                auto member = objMapType.members;
                auto member_count = countMember(member);
                value = feature_object(ctx);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    void* member_ptr = (void*)((char*)struct_ptr + member->offset);
                    feature_value_t prop;
                    bool ret = convertValueToGuest(member->type, member_ptr, ctx, prop);
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
                bool ret = convertValueToGuest(optinalType->type, ptr, ctx, value);
                if (!ret) {
                    feature_free_value(ctx, value);
                    value = FEATURE_UNDEFINED;
                    FEATURE_LOG_ERROR("convert optional to guest failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                int32_t callback = *(int32_t*)ptr;
                if (callback == 0) {
                    FEATURE_LOG_WARN("zero callback id!");
                    value = FEATURE_UNDEFINED;
                    break;
                }
                // unreachable
                FEATURE_LOG_ERROR("convert callback to guest is unreachable");
            } break;
            case COMPLEX_ARRAY: {
                // convert to guest
                FtArray* arrayData = *(FtArray**)ptr;
                if (!arrayData) {
                    FEATURE_LOG_ERROR("null array ptr !");
                    return false;
                }
                ArrayType* arrayType = (ArrayType*)complexType;
                auto element_type = arrayType->element_type;
                size_t element_size = sizeof(uintptr_t);
                // exact and create js value
                value = feature_array(ctx);
                for (int32_t i = 0; i < arrayData->_size; i++) {
                    void* element_ptr = ((char*)arrayData->_element + element_size * i);
                    // convert element value
                    feature_value_t element_obj = FEATURE_UNDEFINED;
                    if (!convertValueToGuest(element_type, element_ptr, ctx, element_obj)) {
                        FEATURE_LOG_ERROR("convert array element to guest failed !");
                        feature_free_value(ctx, element_obj);
                        feature_free_value(ctx, value);
                        return false;
                    }
                    feature_set_array_idx(static_cast<feature_context_ref>(ctx), value, i, element_obj);
                }
            } break;
            case COMPLEX_PROMISE: {
                FEATURE_LOG_ERROR("do not support convert promise to target !");
                return false;
            } break;
            case COMPLEX_INTERFACE: {
                InterfaceType* interface_type = (InterfaceType*)complexType;
                FEATURE_CHECK_NE(interface_type->desc, nullptr);
                FEATURE_CHECK_NE(ptr, nullptr);
                auto pinstance = *static_cast<FeatureInstance**>(ptr);
                if (!pinstance->isInterface()) {
                    FEATURE_LOG_ERROR("not a native interface!");
                    return false;
                }
                if (!pinstance->isInitialized()) {
                    auto module_proto = pinstance->prototype();
                    auto intf_proto = module_proto->getInterfacePrototype(interface_type->desc);
                    pinstance->setPrototype(intf_proto);
                    pinstance->initialize();
                }
                value = target_from_interface(pinstance);
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
} // namespace feature_framework
