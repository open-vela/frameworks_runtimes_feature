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
#include "feature_log.h"
#include "feature_manager_wamr.h"
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

#define PUSH_LOCAL_OBJ_REF(obj)                                                                  \
    {                                                                                            \
        wasm_local_obj_ref_t* ref = (wasm_local_obj_ref_t*)malloc(sizeof(wasm_local_obj_ref_t)); \
        wasm_runtime_push_local_object_ref(exec_env, ref);                                       \
        ref->val = (wasm_obj_t)obj;                                                              \
    }

    static void* interface_from_target(uint64_t& target)
    {
        void* param = *((void**)(&target));
        return param;
    }

    static uint64_t target_from_interface(FeatureInstance* interf)
    {
        return (uint64_t)interf;
    }

    static inline void fill_struct_data(ObjectMapType& obj_type, uint64_t ptr, ts_value_t obj_arr[], uint32_t count)
    {
        if (count <= 0) {
            FEATURE_LOG_ERROR("need fill struct is null!");
        }
        ts_value_t obj_field;
        for (uint32_t i = 0; i < count; i++) {
            // fill it
            auto member = obj_type.members[i];
            void* member_ptr = (void*)((char*)ptr + member.offset);
            FeatureType ftype = member.type;
            TRY_GET_REAL_TYPE(ftype);
            if (FT_IS_PRIMITIVE(ftype)) {
                switch (ftype) {
                case FT_BOOLEAN: {
                    obj_field.of.i32 = *(int32_t*)member_ptr;
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
                    obj_field.of.f64 = *(int64_t*)member_ptr;
                    obj_field.type = TS_NUMBER;
                    obj_arr[i] = obj_field;
                } break;
                case FT_STRING: {
                    char** title_ptr = (char**)member_ptr;
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
            switch (ftype) {
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
            case FT_STRING:
                // wasm string signature is 'r'
                return 'r';
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return 0;
            }
            }
        } else if (FT_IS_COMPLEX(ftype)) {
            ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
            switch (complexType->type) {
            case COMPLEX_OPTIONAL: {
                OptionalType* optionalType = (OptionalType*)complexType;
                char res = getFeatureSignature(optionalType->type);
                if (res == 0) {
                    FEATURE_LOG_WARN("unsupported type detected !");
                    return 0;
                }
                return res;
            } break;
            default: {
                return 'r';
            }
            }
        }
        return 0;
    }

    bool convertConstToGuest(wasm_exec_env_t exec_env, FeatureType ftype, const AppendData& const_data, uint64_t& value)
    {
        if (!FT_IS_PRIMITIVE(ftype)) {
            FEATURE_LOG_ERROR("complex is not supported for const!");
            return false;
        }

        switch (ftype) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void not supported !");
            return false;
        }
        case FT_INT8:
        case FT_INT16:
        case FT_INT32:
        case FT_INT: {
            set_wasm_var_by_type(int32_t, const_data.i32, value);
            break;
        }
        case FT_UINT8:
        case FT_UINT16:
        case FT_UINT32: {
            set_wasm_var_by_type(uint32_t, const_data.u32, value);
            break;
        }
        case FT_INT64: {
            set_wasm_var_by_type(int64_t, const_data.i64, value);
            break;
        }
        case FT_UINT64: {
            set_wasm_var_by_type(uint64_t, const_data.u64, value);
            break;
        }
        case FT_FLOAT: {
            set_wasm_var_by_type(float, const_data.f32, value);
            break;
        }
        case FT_DOUBLE: {
            set_wasm_var_by_type(double, const_data.f64, value);
        } break;
        case FT_BOOLEAN: {
            set_wasm_var_by_type(uint32_t, const_data.u32, value);
        } break;
        case FT_STRING: {
            const char* str = (char*)const_data.str;
            wasm_stringref_obj_t obj = create_wasm_string(exec_env, str);
            set_wasm_var_by_type(void*, obj, value);
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
        wasm_exec_env_t exec_env, uint64_t& value)
    {
        FEATURE_CHECK_NE(ptr, nullptr);
        if (FT_IS_PRIMITIVE(ftype)) {
            switch (ftype) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            }
            case FT_INT: {
                set_wasm_var_by_type(int64_t, *((int32_t*)ptr), value);
            } break;
            case FT_INT8: {
                set_wasm_var_by_type(int64_t, *((int8_t*)ptr), value);
            } break;
            case FT_UINT8: {
                set_wasm_var_by_type(uint64_t, *((uint8_t*)ptr), value);
            } break;
            case FT_INT16: {
                set_wasm_var_by_type(int64_t, *((int16_t*)ptr), value);
            } break;
            case FT_UINT16: {
                set_wasm_var_by_type(uint64_t, *((uint16_t*)ptr), value);
            } break;
            case FT_INT32: {
                set_wasm_var_by_type(int64_t, *((int32_t*)ptr), value);
            } break;
            case FT_UINT32: {
                set_wasm_var_by_type(uint64_t, *((uint32_t*)ptr), value);
            } break;
            case FT_INT64: {
                set_wasm_var_by_type(int64_t, *((int64_t*)ptr), value);
            } break;
            case FT_UINT64: {
                set_wasm_var_by_type(uint64_t, *((uint64_t*)ptr), value);
            } break;
            case FT_FLOAT: {
                set_wasm_var_by_type(float, *((float*)ptr), value);
            } break;
            case FT_DOUBLE: {
                set_wasm_var_by_type(double, *((double*)ptr), value);
            } break;
            case FT_BOOLEAN: {
                set_wasm_var_by_type(uint64_t, *((bool*)ptr), value);
            } break;
            case FT_STRING: {
                const char* str = *(char**)ptr;
                wasm_stringref_obj_t obj = create_wasm_string(exec_env, str);
                PUSH_LOCAL_OBJ_REF(obj);
                set_wasm_var_by_type(void*, obj, value);
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
                void* tmp_ptr = *(void**)ptr;
                ObjectMapType& obj_map_type = *(ObjectMapType*)complex_type;
                auto member_count = countMember(obj_map_type.members);
                ts_value_t obj_arr[member_count];
                /* call fill_struct_data api to fill data in obj array as above */
                fill_struct_data(obj_map_type, (uintptr_t)tmp_ptr, obj_arr, member_count);
                /* call createWasmStruct api from feature_wamr_utils.h */
                wasm_struct_obj_t obj = create_wasm_struct(exec_env, obj_arr, member_count);
                PUSH_LOCAL_OBJ_REF(obj);
                set_wasm_var_by_type(void*, obj, value);
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* opt_type = (OptionalType*)complex_type;
                bool ret = convertValueToGuest(instance, opt_type->type, ptr, exec_env, value);
                if (!ret) {
                    FEATURE_LOG_ERROR("convert optional to guest failed !");
                    return false;
                }
            } break;
            case COMPLEX_ARRAY: {
                FtArray* array = *(FtArray**)ptr;
                uint32_t len = array->_size;
                wasm_struct_obj_t obj = create_wasm_array_with_string(exec_env, (void**)(array->_element), len);
                PUSH_LOCAL_OBJ_REF(obj);
                set_wasm_var_by_type(void*, obj, value);
            } break;
            case COMPLEX_INTERFACE: {
                InterfaceType* interface_type = (InterfaceType*)complex_type;
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

    bool convertValueToHost(FeatureInstance* instance, FeatureType ftype, void*& ptr,
        wasm_exec_env_t exec_env, uint64_t value)
    {
        FEATURE_CHECK_NE(ptr, nullptr);

        if (FT_IS_PRIMITIVE(ftype)) {
            switch (ftype) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            }
            case FT_BOOLEAN: {
                *(bool*)ptr = (bool)get_wasm_args_by_type(double, value);
            } break;
            case FT_INT:
            case FT_INT8:
            case FT_UINT8:
            case FT_INT16:
            case FT_UINT16:
            case FT_INT32:
            case FT_UINT32: {
                *(int32_t*)ptr = (int32_t)get_wasm_args_by_type(double, value);
            } break;
            case FT_INT64: {
                *(int64_t*)ptr = (int64_t)get_wasm_args_by_type(double, value);
            } break;
            case FT_UINT64: {
                *(uint64_t*)ptr = (uint64_t)get_wasm_args_by_type(double, value);
            } break;
            case FT_FLOAT: {
                *(float*)ptr = (float)get_wasm_args_by_type(double, value);
            } break;
            case FT_DOUBLE: {
                *(float64*)ptr = (float64)get_wasm_args_by_type(double, value);
            } break;
            case FT_STRING: {
                void* str = get_wasm_args_by_type(void*, value);
                /* get cstring from wasm string (stringref path) */
                uint32_t str_len = 0;
                if (wasm_obj_is_stringref_obj((wasm_obj_t)str)) {
                    str_len = wasm_string_get_length((wasm_stringref_obj_t)str);
                }
                char* alloc_ptr = str_len > 0 ? (char*)FeatureMalloc(str_len + 1, FT_STRING) : nullptr;
                if (alloc_ptr) {
                    wasm_string_to_cstring((wasm_stringref_obj_t)str, alloc_ptr, str_len + 1);
                }
                *(void**)ptr = alloc_ptr;
            } break;
            case FT_ANY_REF: {
                // copy value
                void* param = get_wasm_args_by_type(void*, value);
                JSValue* js_value = (JSValue*)wasm_anyref_obj_get_value((wasm_anyref_obj_t)param);
                ft_value_t* f_val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
                qjs_val_t* q_val = (qjs_val_t*)f_val;
                q_val->js_val = *js_value;
                *(ft_value_t**)ptr = f_val;
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
                wasm_value_t val = { 0 };
                ObjectMapType& objMapType = *(ObjectMapType*)complex_type;
                ObjectMember* member = (ObjectMember*)objMapType.members;
                int member_count = 0;
                while (member->name) {
                    member_count++;
                    member++;
                }

                if (!*(void**)ptr) {
                    *(void**)ptr = FeatureMalloc(complex_type->size, ftype);
                }
                void* inner_ptr = *(void**)ptr;
                wasm_struct_obj_t wasm_obj = get_wasm_args_by_type(wasm_struct_obj_t, value);
                for (int i = 0; i < member_count; i++) {
                    // fill it
                    member = &objMapType.members[i];
                    wasm_struct_obj_get_field(wasm_obj, i + 1, false, &val);
                    void* member_ptr = (void*)((char*)inner_ptr + member->offset);

                    bool ret = convertValueToHost(instance, member->type, member_ptr, exec_env, *((uint64_t*)&val));

                    // feature_free_value(ctx, propValue);
                    if (!ret) {
                        printf("get property value for key: %s failed !",
                            member->name);
                        return false;
                    }
                }
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* opt_type = (OptionalType*)complex_type;
                bool ret = convertValueToHost(instance, opt_type->type, ptr, exec_env, value);
                if (!ret) {
                    FEATURE_LOG_ERROR("convert optional type failed !");
                    return false;
                }
            } break;
            case COMPLEX_CALLBACK: {
                // save into instance
                CallbackType* callbackType = (CallbackType*)complex_type;
                auto callback_manager = static_cast<FeatureInstanceWamr*>(instance);
                wasm_obj_t cb_value = *((wasm_obj_t*)(&value));
                FtCallbackId id = callback_manager->addCallback(cb_value, callbackType);
                *(FtCallbackId*)ptr = id; // write callback id to pointer.
            } break;
            case COMPLEX_ARRAY: {
                uint32_t len;
                wasm_value_t value1 = { 0 };
                ArrayType& arrayType = *(ArrayType*)complex_type;
                auto element_type = arrayType.element_type;
                wasm_struct_obj_t array_val = get_wasm_args_by_type(wasm_struct_obj_t, value);
                wasm_array_obj_t arr_ref = get_array_ref(array_val);
                len = get_array_length(array_val);
                // FtArray must be a pointer
                if (!*(void**)ptr) {
                    // malloc FtArray struct
                    *(void**)ptr = FeatureMalloc(sizeof(FtArray), ftype);
                }
                FtArray* arrayData = *(FtArray**)ptr;
                arrayData->_size = len;
                if (len) {
                    // we support reference and primitive types
                    size_t element_size = getValueSize(element_type);
                    auto size = element_size * len;
                    FEATURE_CHECK_NE(size, 0);
                    arrayData->_element = malloc(size);
                    memset(arrayData->_element, 0, size);
                    for (size_t i = 0; i < len; i++) {
                        // fill it
                        wasm_array_obj_get_elem(arr_ref, i, false, &value1);
                        void* element_ptr = ((char*)arrayData->_element + element_size * i);
                        if (!convertValueToHost(instance, element_type, element_ptr, exec_env, *((uint64_t*)&value1))) {
                            FEATURE_LOG_ERROR("convert array element failed ");
                            // feature_free_value(ctx, elementValue);
                            break;
                        }
                        // feature_free_value(ctx, elementValue);
                    }
                }
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

}
} // namespace ferry
