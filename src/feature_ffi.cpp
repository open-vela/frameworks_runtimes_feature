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

#include "value_translator.h"
#include "feature_instance.h"
#include "feature_framework.h"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdlib.h>

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
            ptr = FeatureMalloc(sizeof(uintptr_t), isInterface ? FT_RAWPOINTER : FT_POINTER);
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
        case FT_CHAR: {
            // skip string space allocation, delay to value copy
        } break;
        case FT_ANY: {
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
        case COMPLEX_STRUCT_MAP: {
            ptr = FeatureMalloc(complexType->size, featureType);
        } break;
        case COMPLEX_OPTIONAL: {
            OptionalType* optionalType = (OptionalType*)complexType;
            if (!ptr) {
                FEATURE_CHECK_EQ(FT_IS_REFERENCE(optionalType->type), true);
                ptr = FeatureMalloc(sizeof(uintptr_t), FT_REMOVE_REFERENCE(optionalType->type));
            }
            if (!createHostValue(optionalType->type, ptr)) {
                FEATURE_LOG_ERROR("create member pointered memory failed !");
                return false;
            }
        } break;
        case COMPLEX_CALLBACK: {
            // callback means cid
            ptr = FeatureMalloc(sizeof(FtCallbackId), FT_INT32);
        } break;
        case COMPLEX_ARRAY: {
            // array element not created at this point.
            ptr = FeatureMalloc(complexType->size, featureType);
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
            type = &ffi_type_sint8;
        } break;
        case FT_CHAR: {
            type = &ffi_type_pointer;
        } break;
        case FT_ANY: {
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
        case FT_CHAR: {
            result = malloc(sizeof(uintptr_t));
            *(const char**)result = va_arg(ap, const char*);
        } break;
        case FT_ANY: {
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
            FEATURE_CHECK(false && "do not support exact optional type !");
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


template<typename TNative, typename TCtx, typename TTarget>
static void argToNativePtr(TCtx ctx, const TTarget& target, void* ptr) {
  value_translator::toNative(ctx, target, (TNative*)ptr);
}

template<typename TCtx, typename TTarget>
bool convertValueToNatvie(FeatureInstance* instance, FeatureType ftype,
        TCtx ctx, const TTarget& target, void*& pnative) {
    TRY_GET_REAL_TYPE(ftype);
    if (!pnative) {
        if (!createHostValue(ftype, pnative)) {
            FEATURE_LOG_ERROR("create native value failed !");
            return false;
        }
    }

    if (FT_IS_REFERENCE(ftype)) {
        void*& value_ptr = *(void**)pnative;
        if (!convertValueToNatvie(instance, FT_REMOVE_REFERENCE(ftype), ctx, target, value_ptr)) {
            FEATURE_LOG_ERROR("convert target to native failed !");
            return false;
        }
        return true;
    }

    if (FT_IS_PRIMITIVE(ftype)) {
        switch (FT_GET_VALUE(ftype)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            }
            case FT_BOOLEAN:
                if (!argToNativePtr<bool>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to bool failed !");
                    return false;
                } break;
            case FT_INT:
                if (!argToNativePtr<int32_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int failed !");
                    return false;
                } break;
            case FT_INT8:
                if (!argToNativePtr<int8_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int8_t failed !");
                    return false;
                } break;
            case FT_UINT8:
                if (!argToNativePtr<uint8_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint8_t failed !");
                    return false;
                } break;
            case FT_INT16:
                if (!argToNativePtr<int16_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int16_t failed !");
                    return false;
                } break;
            case FT_UINT16:
                if (!argToNativePtr<uint16_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint16_t failed !");
                    return false;
                } break;
            case FT_INT32:
                if (!argToNativePtr<int32_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int32_t failed !");
                    return false;
                } break;
            case FT_UINT32:
                if (!argToNativePtr<uint32_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint32_t failed !");
                    return false;
                } break;
            case FT_INT64:
                if (!argToNativePtr<int64_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to int64_t failed !");
                    return false;
                } break;
            case FT_UINT64:
                if (!argToNativePtr<uint64_t>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to uint64_t failed !");
                    return false;
                } break;
            case FT_FLOAT:
                if (!argToNativePtr<float>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to float failed !");
                    return false;
                } break;
            case FT_DOUBLE:
                if (!argToNativePtr<double>(ctx, target, pnative)) {
                    FEATURE_LOG_ERROR("convert to double failed !");
                    return false;
                } break;
            case FT_CHAR:
                if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                    FEATURE_LOG_ERROR("string arg is null or undefined!");
                    pnative = NULL;
                } else if (!value_translator::isString(ctx, target)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need string !");
                    return false;
                } else {
                    const char* str = NULL;
                    if (!argToNativePtr<const char*>(ctx, target, &str)) {
                        FEATURE_LOG_ERROR("convert to const char* failed !");
                        return false;
                    }
                    char* alloc_ptr = (char*)FeatureMalloc(strlen(str) + 1, FT_CHAR);
                    strcpy(alloc_ptr, str);
                    value_translator::freeString(ctx, str); // to do by wjf
                    pnative = alloc_ptr;
                } break;
            case FT_ANY:
                if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                    FEATURE_LOG_ERROR("object is null or undefined!");
                    pnative = NULL;
                } else {
                    ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY);
                    if (!argToNativePtr<ft_value_t>(ctx, target, f_val)) {
                        FEATURE_LOG_ERROR("convert to ft_value_t failed !");
                        FeatureFreeValue(f_val);
                        pnative = NULL;
                    }
                    pnative = f_val;
                } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
     } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complex_type->type) {
            case COMPLEX_STRUCT_MAP: {
                if (value_translator::isUndefined(ctx, target)) {
                    FEATURE_LOG_WARN("js struct value missing!");
                    pnative = NULL;
                    break;
                }
                ObjectMapType &objMapType = *(ObjectMapType *)complex_type;
                ObjectMember *members = (ObjectMember *)objMapType.members;
                auto member_count = countMember(members);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    bool ret;
                    TTarget field;
                    auto member = &members[i];
                    if (!value_translator::getObjectField(ctx, target, member->name, &field)) {
                        // check field is js_undefined or not
                        if (FT_IS_COMPLEX(member->type)) {
                            ComplexTypeHeader* cmp_type = (ComplexTypeHeader*)FT_GET_COMPLEX(member->type);
                            if (cmp_type->type == COMPLEX_OPTIONAL) {
                                FEATURE_LOG_DEBUG("field is undefined, we get value with optinalType!");
                                OptionalType* opt_type = (OptionalType*)cmp_type;
                                ret = convertValueToTarget(instance, opt_type->type, ctx, &opt_type->fval, field);  // to do by wjf
                                if (!ret) {
                                    value_translator::freeValue(ctx, field);
                                    FEATURE_LOG_ERROR("propValue convert optional failed!");
                                    return false;
                                }
                            }
                        } else {
                            FEATURE_LOG_DEBUG("COMPLEX_STRUCT_MAP member->type is %d!", member->type);
                        }
                    }

                    void *member_ptr = (void *)((char *)pnative + member->offset);
                    ret = convertValueToNatvie(instance, member->type, ctx, field, member_ptr);
                    value_translator::freeValue(ctx, field);
                    if (!ret) {
                        printf("get property value for key: %s failed !", member->name);
                        return false;
                    }
                }
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* opt_type = (OptionalType*)complex_type;
                bool ret = convertValueToNatvie(instance, opt_type->type, ctx, target, pnative);
                if (!ret) {
                    FEATURE_LOG_ERROR("convert optional type failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                // save into instance
                CallbackType *cb_type = (CallbackType *)complex_type;
                ft_value_t cb_value;
                value_translator::argToNative(ctx, target, &cb_value);  // to do by wjf
                // FtCallbackId id = instance->addCallback(cb_value, cb_type);
                // *(FtCallbackId *)pnative = id; // write callback id to pointer.
            } break;
            case COMPLEX_ARRAY: {
                ArrayType& array_type = *(ArrayType*)complex_type;
                auto elem_type = array_type.element_type;
                if (!value_translator::isArray(ctx, target)) {
                    FEATURE_LOG_ERROR("arg type mismatch, need array !");
                    return false;
                }
                auto asize = value_translator::arraySize(ctx, target);
                FtArray* array_data = (FtArray*)pnative;
                array_data->_size = asize;
                if (asize) {
                    // we support reference and primitive types
                    size_t elem_size = FT_IS_REFERENCE(elem_type) ? sizeof(uintptr_t) : getValueSize(elem_type);
                    auto size = elem_size * asize;
                    FEATURE_CHECK_NE(size, 0);
                    array_data->_element = malloc(size);
                    memset(array_data->_element, 0, size);
                    for (size_t i = 0; i < asize; i++) {
                        // fill it
                        TTarget elem_val = value_translator::arrayGet(ctx, target, i);
                        FEATURE_CHECK_NE(value_translator::isUndefined(elem_val), true);
                        void* elem_ptr = ((char*)array_data->_element + elem_size * i);
                        if (!convertValueToNatvie(instance, elem_type, elem_ptr, ctx, elem_val)) {
                            FEATURE_LOG_ERROR("convert array element failed ");
                            value_translator::freeValue(ctx, elem_val);
                            break;
                        }
                        value_translator::freeValue(ctx, elem_val);
                    }
                }
                FEATURE_LOG_DEBUG("array data: %p", ptr);
            } break;
            case COMPLEX_PROMISE: {
                FEATURE_LOG_ERROR("do not support convert promise to guest !");
                return false;
            } break;
            case COMPLEX_INTERFACE: {  // to do by wjf
                // get interface ptr from js object
                // auto opaque_ptr = feature_get_opaque(target, FeatureManagerQjs::jsClassId());
                // FEATURE_CHECK_NE(opaque_ptr, nullptr);
                // pnative = opaque_ptr;
            } break;
            default: {
                FEATURE_LOG_ERROR("unsupported complex type !");
                return false;
            }
        }
    }
}

} // namespace ferry
