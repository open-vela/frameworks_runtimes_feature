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

#ifndef __FEATURE_CONVERTOR_TEMPLATES_H__
#define __FEATURE_CONVERTOR_TEMPLATES_H__
#include "array_buffer.h"
#include "feature_common.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_instance.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "protobuf/proto_reflection.h"
#include "value_translator.h"
#include <cstdarg>
#include <list>
#include <protobuf-c/protobuf-c.h>
#include <stdalign.h>

namespace feature_framework {

template <typename TNative, typename TCtx, typename TTarget>
bool nativeToTarget(TCtx ctx, void* ptr, TTarget& target)
{
    return value_translator::toTarget(ctx, *((TNative*)ptr), &target);
}

template <typename TCtx, typename TTarget>
bool convertValueToTarget(FeatureType ftype,
    TCtx ctx, void* pnative, TTarget& target)
{
    FEATURE_CHECK_NE(pnative, nullptr);
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (ftype) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void not supported !");
            return false;
        } break;
        case FT_INT: {
            nativeToTarget<int32_t>(ctx, pnative, target);
        } break;
        case FT_INT8: {
            nativeToTarget<int8_t>(ctx, pnative, target);
        } break;
        case FT_UINT8: {
            nativeToTarget<uint8_t>(ctx, pnative, target);
        } break;
        case FT_INT16: {
            nativeToTarget<int16_t>(ctx, pnative, target);
        } break;
        case FT_UINT16: {
            nativeToTarget<uint16_t>(ctx, pnative, target);
        } break;
        case FT_INT32: {
            nativeToTarget<int32_t>(ctx, pnative, target);
        } break;
        case FT_UINT32: {
            nativeToTarget<uint32_t>(ctx, pnative, target);
        } break;
        case FT_INT64: {
            nativeToTarget<int64_t>(ctx, pnative, target);
        } break;
        case FT_UINT64: {
            nativeToTarget<uint64_t>(ctx, pnative, target);
        } break;
        case FT_FLOAT: {
            nativeToTarget<float>(ctx, pnative, target);
        } break;
        case FT_DOUBLE: {
            nativeToTarget<double>(ctx, pnative, target);
        } break;
        case FT_BOOLEAN: {
            nativeToTarget<bool>(ctx, pnative, target);
        } break;
        case FT_STRING: {
            if (!(*(char**)pnative)) {
                const char* empty_str = "";
                nativeToTarget<const char*>(ctx, &empty_str, target);
            } else {
                nativeToTarget<const char*>(ctx, pnative, target);
            }
        } break;
        case FT_ANY_REF: {
            if (!(*(ft_value_t**)pnative)) {
                ft_value_t null_val = value_translator::nullFtVal();
                nativeToTarget<ft_value_t>(ctx, &null_val, target);
            } else {
                nativeToTarget<ft_value_t>(ctx, *(ft_value_t**)pnative, target);
            }
        } break;
        case FT_JSON_OBJ: {
            if (!(*(FtJsonObject*)pnative)) {
                value_translator::toTargetJson(ctx, nullptr, &target);
            } else {
                if (!value_translator::toTargetJson(ctx, *(FtJsonObject*)pnative, &target)) {
                    FEATURE_LOG_WARN("toTargetJson failed, json string: %s", (*(FtJsonObject*)pnative)->str);
                }
            }
        } break;
        case FT_ARRAY_BUFFER: {
            FtArrayBuffer fab = *(FtArrayBuffer*)pnative;
            if (!fab) {
                target = value_translator::nullValue(ctx);
            } else {
                if (!nativeToTarget<FtArrayBuffer>(ctx, pnative, target)) {
                    FEATURE_LOG_WARN("convert arraybuffer failed!");
                    return false;
                }
            }
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
            ObjectMapType& obj_map_type = *(ObjectMapType*)complex_type;
            auto member = obj_map_type.members;
            auto member_count = countMember(member);
            target = value_translator::createStruct(ctx, obj_map_type, member_count);
            void* struct_ptr = *(void**)pnative;
            if (!struct_ptr) {
                value_translator::freeValue(ctx, target);
                target = value_translator::nullValue(ctx);
                FEATURE_LOG_WARN("null struct ptr!");
                break;
            }
            if (value_translator::isNull(ctx, target)) {
                FEATURE_LOG_ERROR("create struct failed!");
                return false;
            }
            for (int i = 0; i < member_count; i++) {
                // fill it
                void* member_ptr = (void*)((char*)struct_ptr + member->offset);
                TTarget field;
                bool ret = convertValueToTarget(member->type, ctx, member_ptr, field);
                if (!ret) {
                    value_translator::freeValue(ctx, field);
                    FEATURE_LOG_ERROR("convert field name: %s failed !", member->name);
                    return false;
                }
                value_translator::setStructField(ctx, target, member->name, i, field);
                member++;
            }
        } break;
        case COMPLEX_OPTIONAL: {
            OptionalType* opt_type = (OptionalType*)complex_type;
            bool ret = convertValueToTarget(opt_type->type, ctx, pnative, target);
            if (!ret) {
                value_translator::freeValue(ctx, target);
                FEATURE_LOG_ERROR("convert optional to guest failed !");
                return false;
            }
        } break;
        case COMPLEX_CALLBACK: {
            bool ret = convertValueToTarget(FT_INT32, ctx, pnative, target);
            if (!ret) {
                value_translator::freeValue(ctx, target);
                FEATURE_LOG_ERROR("convert optional to guest failed !");
                return false;
            }
        } break;
        case COMPLEX_ARRAY: {
            // convert to guest
            FtArray* array = *(FtArray**)pnative;
            if (!array) {
                FEATURE_LOG_WARN("array ptr is null!");
                target = value_translator::nullValue(ctx);
                break;
            }
            auto elem_type = ((ArrayType*)complex_type)->element_type;
            size_t elem_size = FT_IS_REFERENCE(elem_type) ? sizeof(uintptr_t) : getValueSize(elem_type);
            target = value_translator::createArray(ctx, elem_type, array->_size);
            if (value_translator::isNull(ctx, target)) {
                FEATURE_LOG_ERROR("create array failed!");
                return false;
            }
            for (int32_t i = 0; i < array->_size; i++) {
                void* elem_ptr = ((char*)array->_element + elem_size * i);
                // convert element target
                TTarget elem_val;
                if (!convertValueToTarget(elem_type, ctx, elem_ptr, elem_val)) {
                    FEATURE_LOG_ERROR("convert array element to guest failed !");
                    value_translator::freeValue(ctx, elem_val);
                    value_translator::freeValue(ctx, target);
                    return false;
                }
                value_translator::arraySet(ctx, target, i, elem_val);
            }
        } break;
        case COMPLEX_PROMISE: {
            FEATURE_LOG_ERROR("do not support convert promise to target !");
            return false;
        } break;
        case COMPLEX_INTERFACE: {
            InterfaceType* interface_type = (InterfaceType*)complex_type;
            FEATURE_CHECK_NE(interface_type->desc, nullptr);
            FEATURE_CHECK_NE(pnative, nullptr);
            auto pinstance = *static_cast<FeatureInstance**>(pnative);
            if (!pinstance) {
                FEATURE_LOG_ERROR("null interface instance!");
                target = value_translator::nullValue(ctx);
                break;
            }
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
            target = value_translator::targetFromInterface(ctx, pinstance);
        } break;
        case COMPLEX_PROTOBUF: {
            ProtobufMessageType* proto = (ProtobufMessageType*)complex_type;
            FEATURE_CHECK_EQ(proto->desc, (*(ProtobufCMessage**)pnative)->descriptor);
            target = value_translator::newObject(ctx);
            if constexpr (std::is_same_v<JSContext*, std::remove_cv_t<TCtx>>) {
                proto_reflection::fromNative(ctx, target, *(ProtobufCMessage**)(pnative));
            } else {
                FEATURE_LOG_ERROR("protobuf only support JS backend !");
            }
        } break;
        default: {
            FEATURE_LOG_ERROR("unsupported complex type !");
            return false;
        }
        }
    }
    return true;
}

// templeate functions
template <typename TNative, typename TCtx, typename TTarget>
bool argToNativePtr(TCtx ctx, TTarget& target, void* native_ptr)
{
    return value_translator::toNative(ctx, target, (TNative*)native_ptr);
}

template <typename TInstance, typename TCtx, typename TTarget>
bool convertValueToNative(TInstance* instance, FeatureType ftype,
    TCtx ctx, TTarget& target, void*& pnative)
{
    FEATURE_CHECK_NE(pnative, nullptr);
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (ftype) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void not supported !");
            return false;
        }
        case FT_BOOLEAN:
            if (!argToNativePtr<bool>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to bool failed !");
                return false;
            }
            break;
        case FT_INT:
            if (!argToNativePtr<int32_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to int failed !");
                return false;
            }
            break;
        case FT_INT8:
            if (!argToNativePtr<int8_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to int8_t failed !");
                return false;
            }
            break;
        case FT_UINT8:
            if (!argToNativePtr<uint8_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to uint8_t failed !");
                return false;
            }
            break;
        case FT_INT16:
            if (!argToNativePtr<int16_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to int16_t failed !");
                return false;
            }
            break;
        case FT_UINT16:
            if (!argToNativePtr<uint16_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to uint16_t failed !");
                return false;
            }
            break;
        case FT_INT32:
            if (!argToNativePtr<int32_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to int32_t failed !");
                return false;
            }
            break;
        case FT_UINT32:
            if (!argToNativePtr<uint32_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to uint32_t failed !");
                return false;
            }
            break;
        case FT_INT64:
            if (!argToNativePtr<int64_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to int64_t failed !");
                return false;
            }
            break;
        case FT_UINT64:
            if (!argToNativePtr<uint64_t>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to uint64_t failed !");
                return false;
            }
            break;
        case FT_FLOAT:
            if (!argToNativePtr<float>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to float failed !");
                return false;
            }
            break;
        case FT_DOUBLE:
            if (!argToNativePtr<double>(ctx, target, pnative)) {
                FEATURE_LOG_ERROR("convert to double failed !");
                return false;
            }
            break;
        case FT_STRING:
            if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                FEATURE_LOG_ERROR("string arg is null or undefined!");
                *(const char**)pnative = NULL;
            } else if (!value_translator::isString(ctx, target)) {
                FEATURE_LOG_ERROR("arg type mismatch, need string !");
                return false;
            } else {
                char* str = NULL;
                if (!argToNativePtr<char*>(ctx, target, (void*)(&str))) {
                    FEATURE_LOG_ERROR("convert to const char* failed !");
                    return false;
                }
                char* alloc_ptr = (char*)FeatureMalloc(strlen(str) + 1, FT_STRING);
                strcpy(alloc_ptr, str);
                value_translator::freeCString(ctx, str);
                *(const char**)pnative = alloc_ptr;
            }
            break;
        case FT_ANY_REF:
            if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                FEATURE_LOG_ERROR("object is null or undefined!");
                *(ft_value_t**)pnative = NULL;
            } else {
                ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
                if (!argToNativePtr<ft_value_t>(ctx, target, f_val)) {
                    FEATURE_LOG_ERROR("convert to ft_value_t failed !");
                    FeatureFreeValue(f_val);
                    *(ft_value_t**)pnative = NULL;
                }
                *(ft_value_t**)pnative = f_val;
            }
            break;
        case FT_JSON_OBJ:
            if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                FEATURE_LOG_ERROR("jsonobject arg is null or undefined!");
                *(FtJsonObject*)pnative = NULL;
            } else if (!value_translator::isObject(ctx, target)) {
                FEATURE_LOG_ERROR("arg type mismatch, need object !");
                return false;
            } else {
                if (!argToNativePtr<FtJsonObject>(ctx, target, (void*)(pnative))) {
                    FEATURE_LOG_ERROR("convert to FtJSONValue* failed !");
                    return false;
                }
            }
            break;
        case FT_ARRAY_BUFFER:
            *(FtArrayBuffer*)pnative = NULL;
            if (value_translator::isNull(ctx, target) || value_translator::isUndefined(ctx, target)) {
                FEATURE_LOG_ERROR("arraybuffer arg is null or undefined!");
            } else if (!value_translator::isArrayBuffer(ctx, target)) {
                FEATURE_LOG_ERROR("arg type mismatch, need arraybuffer !");
                return false;
            } else {
                ft_value_t f_val;
                argToNativePtr<ft_value_t>(ctx, target, &f_val);
                ArrayBufferCreateParams params { .type = ArrayBufferCreateParams::kTarget, .target = f_val };
                *(FtArrayBuffer*)pnative = (FtArrayBuffer)instance->featureManager()->createArrayBuffer(params);
            }
            break;
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
                FEATURE_LOG_WARN("js struct is undefined!");
                break;
            }
            if (value_translator::isNull(ctx, target)) {
                FEATURE_LOG_WARN("js struct is null!");
                break;
            }
            // check js argv is object or not.
            if (!value_translator::isPlainObject(ctx, target)) {
                FEATURE_LOG_ERROR("arg type mismatch, need struct!");
                return false;
            }

            if (!*(void**)pnative) {
                *(void**)pnative = FeatureMalloc(complex_type->size, ftype);
            }
            void* ptr = *(void**)pnative;
            ObjectMapType& obj_map_type = *(ObjectMapType*)complex_type;
            ObjectMember* members = (ObjectMember*)obj_map_type.members;
            auto member_count = countMember(members);
            for (int i = 0; i < member_count; i++) {
                // fill it
                bool ret;
                TTarget field;
                auto member = &members[i];
                if (!value_translator::getStructField(ctx, target, member->name, i, &field)) {
                    // check field is js_undefined or not
                    if (FT_IS_COMPLEX(member->type)) {
                        ComplexTypeHeader* cmp_type = (ComplexTypeHeader*)FT_GET_COMPLEX(member->type);
                        if (cmp_type->type == COMPLEX_OPTIONAL) {
                            FEATURE_LOG_DEBUG("field is undefined, we get value with optinalType!");
                            OptionalType* opt_type = (OptionalType*)cmp_type;
                            void* member_ptr = (void*)((char*)ptr + member->offset);
                            ret = convertOptional(opt_type, member_ptr);
                            if (!ret) {
                                value_translator::freeValue(ctx, field);
                                FEATURE_LOG_ERROR("propValue convert optional failed!");
                                return false;
                            }
                            continue;
                        }
                    } else {
                        if ((member->type != FT_ANY_REF) && (member->type != FT_STRING) && (member->type != FT_JSON_OBJ)) {
                            FEATURE_LOG_ERROR("struct member with type '%d' missing!", member->type);
                            return false;
                        }
                    }
                }

                void* member_ptr = (void*)((char*)ptr + member->offset);
                ret = convertValueToNative(instance, member->type, ctx, field, member_ptr);
                value_translator::freeValue(ctx, field);
                if (!ret) {
                    FEATURE_LOG_ERROR("get property value for key: %s failed !", member->name);
                    return false;
                }
            }
        } break;
        case COMPLEX_OPTIONAL: {
            OptionalType* opt_type = (OptionalType*)complex_type;
            bool ret = convertValueToNative(instance, opt_type->type, ctx, target, pnative);
            if (!ret) {
                FEATURE_LOG_ERROR("convert optional type failed !");
                return false;
            }
        } break;
        case COMPLEX_CALLBACK: {
            // save into instance
            if (value_translator::isUndefined(ctx, target)) {
                FEATURE_LOG_WARN("js callback is undefined!");
                break;
            } else if (value_translator::isNull(ctx, target)) {
                FEATURE_LOG_WARN("js callback is null!");
                break;
            } else if (!value_translator::isFunction(ctx, target)) {
                FEATURE_LOG_ERROR("arg type mismatch, need callback function!");
                return false;
            }
            CallbackType* callback_type = (CallbackType*)complex_type;
            auto cb_val = value_translator::toCallbackValue(target);
            FtCallbackId id = instance->addCallback(cb_val, callback_type);
            if (instance->getCallbacks().size() >= CONFIG_FEATURE_MAX_CALLBACK_COUNT) {
                FEATURE_LOG_ERROR("callback count of instance[%p] is %d, which is larger than %d, feature name is[%s]",
                    instance,
                    instance->getCallbacks().size(),
                    CONFIG_FEATURE_MAX_CALLBACK_COUNT,
                    instance->description()->name);
            }
            *(FtCallbackId*)pnative = id; // write callback id to pointer.
        } break;
        case COMPLEX_ARRAY: {
            if (value_translator::isUndefined(ctx, target) || value_translator::isNull(ctx, target)) {
                FEATURE_LOG_WARN("js array is null or undefined!");
                *(FtArray**)pnative = NULL;
                break;
            } else if (!value_translator::isArray(ctx, target)) {
                FEATURE_LOG_ERROR("arg type mismatch, need array!");
                return false;
            }
            // FtArray must be a pointer
            if (!*(FtArray**)pnative) {
                // malloc FtArray struct
                *(FtArray**)pnative = (FtArray*)FeatureMalloc(sizeof(FtArray), ftype);
            }
            FtArray* array = *(FtArray**)pnative;
            auto elem_type = ((ArrayType*)complex_type)->element_type;
            auto asize = value_translator::arraySize(ctx, target);
            array->_size = asize;
            if (asize) {
                // we support reference and primitive types
                size_t elem_size = FT_IS_REFERENCE(elem_type) ? sizeof(uintptr_t) : getValueSize(elem_type);
                auto size = elem_size * asize;
                FEATURE_CHECK_NE(size, 0);
                array->_element = malloc(size);
                memset(array->_element, 0, size);
                for (size_t i = 0; i < asize; i++) {
                    // fill it
                    TTarget elem_val = value_translator::arrayGet(ctx, target, i);
                    FEATURE_CHECK_NE(value_translator::isUndefined(ctx, elem_val), true);
                    void* elem_ptr = ((char*)array->_element + elem_size * i);
                    if (!convertValueToNative(instance, elem_type, ctx, elem_val, elem_ptr)) {
                        FEATURE_LOG_ERROR("convert array element failed ");
                        value_translator::freeValue(ctx, elem_val);
                        return false;
                    }
                    value_translator::freeValue(ctx, elem_val);
                }
            }
        } break;
        case COMPLEX_PROMISE: {
            FEATURE_LOG_ERROR("do not support convert promise to guest !");
            return false;
        } break;
        case COMPLEX_INTERFACE: {
            *(void**)pnative = value_translator::interfaceFromTarget(ctx, target);
        } break;
        case COMPLEX_PROTOBUF: {
            ProtobufMessageType* proto = (ProtobufMessageType*)complex_type;
            ProtobufCMessage* out {};
            if constexpr (std::is_same_v<JSContext*, std::remove_cv_t<TCtx>>) {
                proto_reflection::toNative(instance, ctx, target, proto->desc, &out);
            } else {
                FEATURE_LOG_ERROR("protobuf only support JS backend !");
            }
            *(void**)pnative = out;
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
#endif // __FEATURE_CONVERTOR_TEMPLATES_H__
