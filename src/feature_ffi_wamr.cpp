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

#include "feature_ffi_wamr.h"
#include "feature_context_qjs.h"
#include "feature_instance_wamr.h"
#include "feature_manager_wamr.h"
#include "feature_log.h"
#include "feature_prototype.h"
#include "feature_utils.h"
#include "feature_wamr_utils.h"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <ffi.h>
#include <functional>
#include <stdlib.h>

using namespace FEATURE;
using namespace ferry;

namespace ferry {

namespace FeatureFFIWamr {

static inline void fill_struct_data(ObjectMapType &obj_type, uint64_t ptr, ts_value_t obj_arr[], uint32_t count)
{
    if (count <= 0) {
        FEATURE_LOG_ERROR("need fill struct is null!");
    }
    ts_value_t obj_field;
    for (uint32_t i = 0; i < count; i++) {
        // fill it
        auto member = obj_type.members[i];
        void *member_ptr = (void *)((char *)ptr + member.offset);
        FeatureType ftype = member.type;
        TRY_GET_REAL_TYPE(ftype);
        if (FT_IS_PRIMITIVE(ftype)) {
            switch (FT_GET_VALUE(ftype)) {
                case FT_BOOLEAN: {
                    obj_field.of.i32 = *(int32_t *)member_ptr;
                    obj_field.type = TS_BOOLEAN;
                    obj_arr[i] = obj_field;
                } break;
                case FT_INT:
                case FT_INT8:
                case FT_UINT8:
                case FT_INT16:
                case FT_UINT16:
                case FT_INT32:
                case FT_UINT32:
                case FT_INT64:
                case FT_UINT64:
                case FT_FLOAT:
                case FT_DOUBLE: {
                    obj_field.of.f64 = *(int64_t *)member_ptr;
                    obj_field.type = TS_NUMBER;
                    obj_arr[i] = obj_field;
                } break;
                case FT_CHAR: {
                    char **title_ptr = (char **)member_ptr;
                    obj_field.of.ref = *title_ptr;
                    obj_field.type = TS_STRING;
                    obj_arr[i] = obj_field;
                } break;
                default:
                    break;
            }
        }
    }
}

char getFeatureSignature(FeatureType ftype)
{
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (FT_GET_VALUE(ftype)) {
            case FT_VOID:
                return 0;
            case FT_BOOLEAN:
                return 'i';
            case FT_INT:
            case FT_INT8:
            case FT_UINT8:
            case FT_INT16:
            case FT_UINT16:
            case FT_INT32:
            case FT_UINT32:
            case FT_INT64:
            case FT_UINT64:
            case FT_FLOAT:
            case FT_DOUBLE:
                return 'F';
            case FT_CHAR:
            // wasm string signature is 'r'
                return 'r';
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return 0;
            }
        }
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader *complexType = (ComplexTypeHeader *)FT_GET_COMPLEX(ftype);
        switch (complexType->type) {
            case COMPLEX_OPTIONAL: {
                OptionalType *optionalType = (OptionalType *)complexType;
                char res = getFeatureSignature(optionalType->type);
                if (res == 0) {
                    FEATURE_LOG_WARN("unsupported type detected !");
                    return 0;
                }
                return res;
            }
            break;
            default: {
                return 'r';
            }
        }
    }
    return 0;
}

bool convertConstToGuest(wasm_exec_env_t exec_env, FeatureType ftype, AppendData& const_data, uint64_t* value)
{
    if (!FT_IS_PRIMITIVE(ftype)) {
        FEATURE_LOG_ERROR("complex is not supported for const!");
        return false;
    }

    switch (FT_GET_VALUE(ftype)) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void not supported !");
            return false;
        }
        case FT_INT8:
        case FT_INT16:
        case FT_INT32:
        case FT_INT: {
            native_raw_return_type(int32_t, value);
            native_raw_set_return(const_data.i32);
            break;
        }
        case FT_UINT8:
        case FT_UINT16:
        case FT_UINT32: {
            native_raw_return_type(uint32_t, value);
            native_raw_set_return(const_data.u32);
            break;
        }
        case FT_INT64: {
            native_raw_return_type(int64_t, value);
            native_raw_set_return(const_data.i64);
            break;
        }
        case FT_UINT64: {
            native_raw_return_type(uint64_t, value);
            native_raw_set_return(const_data.u64);
            break;
        }
        case FT_FLOAT: {
            native_raw_return_type(float, value);
            native_raw_set_return(const_data.f32);
            break;
        }
        case FT_DOUBLE: {
            native_raw_return_type(double, value);
            native_raw_set_return(const_data.f64);
        } break;
        case FT_BOOLEAN: {
            native_raw_return_type(uint32_t, value);
            native_raw_set_return(const_data.u32);
        } break;
        case FT_CHAR: {
            native_raw_return_type(void *, value);
            const char *str = (char *)const_data.str;
            printf("return str is %s\n", str);
            wasm_stringref_obj_t obj = create_wasm_string(exec_env, str);
            native_raw_set_return(obj);
            break;
        }
        default: {
            FEATURE_LOG_WARN("unsupported type detected !");
            return false;
        }
    }

    return true;
}

bool convertValueToGuest(FeatureInstance* instance, FeatureType ftype, void* ptr,
    wasm_exec_env_t exec_env,  uint64_t* value)
{
    FEATURE_CHECK_NE(ptr, nullptr);
    if (FT_IS_REFERENCE(ftype)) {
        ptr = *(void**)ptr;
    }
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (FT_GET_VALUE(ftype)) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            }
            case FT_INT: {
                native_raw_return_type(int64_t, value);
                native_raw_set_return(*((int32_t*)ptr));
            } break;
            case FT_INT8: {
                native_raw_return_type(int64_t, value);
                native_raw_set_return(*((int8_t*)ptr));
            } break;
            case FT_UINT8: {
                native_raw_return_type(uint64_t, value);
                native_raw_set_return(*((uint8_t*)ptr));
            } break;
            case FT_INT16: {
                native_raw_return_type(int64_t, value);
                native_raw_set_return(*((int16_t*)ptr));
            } break;
            case FT_UINT16: {
                native_raw_return_type(uint64_t, value);
                native_raw_set_return(*((uint16_t*)ptr));
            } break;
            case FT_INT32: {
                native_raw_return_type(int64_t, value);
                native_raw_set_return(*((int32_t*)ptr));
            } break;
            case FT_UINT32: {
                native_raw_return_type(uint64_t, value);
                native_raw_set_return(*((uint32_t*)ptr));
            } break;
            case FT_INT64: {
                native_raw_return_type(int64_t, value);
                native_raw_set_return(*((int64_t*)ptr));
            } break;
            case FT_UINT64: {
                native_raw_return_type(uint64_t, value);
                native_raw_set_return(*((uint64_t*)ptr));
            } break;
            case FT_FLOAT: {
                native_raw_return_type(float, value);
                native_raw_set_return(*((float*)ptr));
            } break;
            case FT_DOUBLE: {
                native_raw_return_type(double, value);
                native_raw_set_return(*((double*)ptr));
            } break;
            case FT_BOOLEAN: {
                native_raw_return_type(uint64_t, value);
                native_raw_set_return(*((bool*)ptr));
            } break;
            case FT_CHAR: {
                native_raw_return_type(void *, value);
                const char *str = (char *)ptr;
                printf("return str is %s\n", str);
                wasm_stringref_obj_t obj = create_wasm_string(exec_env, str);
                native_raw_set_return(obj);
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader *complex_type = (ComplexTypeHeader *)FT_GET_COMPLEX(ftype);
        switch (complex_type->type) {
            case COMPLEX_STRUCT_MAP: {
                native_raw_return_type(void *, value);
                ObjectMapType &obj_map_type = *(ObjectMapType *)complex_type;
                auto member_count = countMember(obj_map_type.members);
                ts_value_t obj_arr[member_count];
                /* call fill_struct_data api to fill data in obj array as above */
                fill_struct_data(obj_map_type, (uintptr_t)ptr, obj_arr, member_count);
                /* call createWasmStruct api from feature_wamr_utils.h */
                wasm_struct_obj_t obj = create_wasm_struct(exec_env, obj_arr, member_count);
                native_raw_set_return(obj);
            }
            break;
            case COMPLEX_ARRAY: {
                native_raw_return_type(void *, value);
                FtArray *array = (FtArray *)ptr;
                uint32_t len = array->_size;
                wasm_struct_obj_t obj = create_wasm_array_with_string(exec_env, array->_element, len);
                native_raw_set_return(obj);
            }
            break;
            case COMPLEX_INTERFACE: {
                InterfaceType* interface_type = (InterfaceType*)complex_type;
                FEATURE_CHECK_NE(interface_type->desc, nullptr);
                FEATURE_CHECK_NE(ptr, nullptr);
                auto interface_ptr = static_cast<FeatureInstance*>(ptr);
                auto parent = interface_ptr->parent();
                FEATURE_CHECK_NE(parent, nullptr);
                FeatureManagerWamr* manager = (FeatureManagerWamr*)(parent->prototype()->getFeatureManager());
                FEATURE_CHECK_NE(manager, nullptr);
                ptr = manager->createTargetInterface(interface_ptr, interface_type->desc);

                native_raw_return_type(void *, value);
                native_raw_set_return(ptr);
            } break;
        }
    }
    return true;
}

bool convertValueToHost(FeatureInstance* instance, FeatureType ftype, void*& ptr,
	wasm_exec_env_t exec_env, uint64_t* value)
{
    // special step: get real type of complex type
    TRY_GET_REAL_TYPE(ftype);
    if (!ptr) {
        if (!createHostValue(ftype, ptr)) {
            FEATURE_LOG_ERROR("create host value failed !");
            return false;
        }
    }
    if (FT_IS_REFERENCE(ftype)) {
        void*& value_ptr = *(void**)ptr;
        if (!convertValueToHost(instance, FT_REMOVE_REFERENCE(ftype), value_ptr, exec_env, value)) {
            FEATURE_LOG_ERROR("convert value to host failed !");
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
            case FT_INT:
            case FT_INT8:
            case FT_UINT8:
            case FT_INT16:
            case FT_UINT16:
            case FT_INT32:
            case FT_UINT32: {
                native_raw_get_arg(double, num_val, value);
                *(int32_t*)ptr = (int32_t)num_val;
            }
            break;
            case FT_INT64: {
                native_raw_get_arg(double, num_val, value);
                *(int64_t*)ptr = (int64_t)num_val;
            }
            break;
            case FT_UINT64: {
                native_raw_get_arg(double, num_val, value);
                *(u_int64_t*)ptr = (u_int64_t)num_val;
            }
            break;
            case FT_FLOAT: {
                native_raw_get_arg(double, num_val, value);
                *(float*)ptr = (float)num_val;
            }
            break;
            case FT_DOUBLE: {
                native_raw_get_arg(double, num_val, value);
                *(float64*)ptr = (float64)num_val;
            }
            break;
            case FT_CHAR: {
                native_raw_get_arg(void *, str, value);
                /* get cstring from wasm string (stringref path) */
                uint32_t str_len = 0, len = 0;
                if (wasm_obj_is_stringref_obj((wasm_obj_t)str)) {
                    str_len = wasm_string_get_length((wasm_stringref_obj_t)str);
                }
                char *buffer = str_len > 0 ? (char *)malloc(str_len + 1) : nullptr;
                if (buffer != nullptr) {
                    len = wasm_string_to_cstring((wasm_stringref_obj_t)str, buffer, str_len + 1);
                }
                char* alloc_ptr = (char*)FeatureMalloc(strlen(buffer) + 1, FT_CHAR);
                strcpy(alloc_ptr, buffer);
                ptr = alloc_ptr;
                // feature_free_cstring(ctx, str);
            }
            break;
            case FT_ANY: {
                // copy value
                native_raw_get_arg(void *, param, value);
                JSValue *js_value = (JSValue *)wasm_anyref_obj_get_value((wasm_anyref_obj_t)param);
                ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY);
                qjs_val_t* q_val = (qjs_val_t*)f_val;
                q_val->js_val = *js_value;
                ptr = f_val;
            }
            break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complexType->type) {
            case COMPLEX_STRUCT_MAP: {
                wasm_value_t val = {0};
                ObjectMapType &objMapType = *(ObjectMapType *)complexType;
                ObjectMember *member = (ObjectMember *)objMapType.members;
                int member_count = 0;
                while (member->name) {
                    member_count++;
                    member++;
                }

                native_raw_get_arg(wasm_struct_obj_t, wasm_obj, value);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    auto member = &objMapType.members[i];
                    wasm_struct_obj_get_field(wasm_obj, i + 1, false, &val);
                    void *member_ptr = (void *)((char *)ptr + member->offset);
                    bool ret = convertValueToHost(instance, member->type, member_ptr, exec_env, (uint64_t *)&val);
                    // feature_free_value(ctx, propValue);
                    if (!ret) {
                        printf("get property value for key: %s failed !",
                            member->name);
                        return false;
                    }
                }
            } break;
            case COMPLEX_CALLBACK: {
                // save into instance
                CallbackType *callbackType = (CallbackType *)complexType;
                FtCallbackId id = ((FeatureInstanceWamr *)instance)->addCallback(value, callbackType);
                *(FtCallbackId *)ptr = id; // write callback id to pointer.
            } break;
            case COMPLEX_ARRAY: {
                uint32_t len;
                wasm_value_t value1 = {0};
                ArrayType &arrayType = *(ArrayType *)complexType;
                auto element_type = arrayType.element_type;
                native_raw_get_arg(wasm_struct_obj_t, arrayValue, value);
                wasm_array_obj_t arr_ref = get_array_ref(arrayValue);
                len = get_array_length(arrayValue);
                FtArray *arrayData = (FtArray *)ptr;
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
                        wasm_array_obj_get_elem(arr_ref, i, false, &value1);
                        void *element_ptr = ((char *)arrayData->_element + element_size * i);
                        if (!convertValueToHost(instance, element_type, element_ptr, exec_env, (uint64_t *)&value1)) {
                            FEATURE_LOG_ERROR("convert array element failed ");
                            // feature_free_value(ctx, elementValue);
                        break;
                        }
                        // feature_free_value(ctx, elementValue);
                    }
                }
            } break;
            case COMPLEX_INTERFACE: {
                native_raw_get_arg(void*, param, value);
                ptr = param;
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

