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
#include "feature_instance_wamr.h"
#include "feature_log.h"
#include "feature_manager_wamr.h"
#include "feature_prototype.h"
#include "feature_utils.h"
#include "feature_wamr_utils.h"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdlib.h>

using namespace feature_framework;

namespace feature_framework {

namespace FeatureFFIWamr {

    static void* interface_from_target(uint64_t& target)
    {
        void* param = *((void**)(&target));
        return param;
    }

    static uint64_t target_from_interface(FeatureInstance* interf)
    {
        return (uint64_t)interf;
    }

    static bool get_struct_field_types(ObjectMapType& obj_type, wasm_value_type_t type_arr[], uint32_t count)
    {
        if (count <= 0) {
            FEATURE_LOG_ERROR("struct field count is invalid!");
            return false;
        }
        for (uint32_t i = 0; i < count; i++) {
            auto member = obj_type.members[i];
            wasm_value_type_t wasm_type = ft_type_to_wasm_type(member.type);
            if (wasm_type == 0)
                return false;
            type_arr[i] = wasm_type;
        }
        return true;
    }

    static wasm_struct_type_t get_struct_type(wasm_exec_env_t exec_env, ObjectMapType& obj_map_type, uint32_t member_count)
    {
        wasm_struct_type_t struct_type = NULL;
        wasm_value_type_t field_types[member_count];
        if (!get_struct_field_types(obj_map_type, field_types, member_count))
            return struct_type;
        find_struct_type(exec_env, field_types, member_count, &struct_type);
        return struct_type;
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

    bool convertValueToGuest(FeatureType ftype, void* ptr, wasm_exec_env_t exec_env, uint64_t& value)
    {
        FEATURE_CHECK_NE(ptr, nullptr);
        if (FT_IS_PRIMITIVE(ftype)) {
            switch (ftype) {
            case FT_VOID: {
                FEATURE_LOG_ERROR("void not supported !");
                return false;
            }
            case FT_INT: {
                set_wasm_var_by_type(wasm_number, *((int32_t*)ptr), value);
            } break;
            case FT_INT8: {
                set_wasm_var_by_type(wasm_number, *((int8_t*)ptr), value);
            } break;
            case FT_UINT8: {
                set_wasm_var_by_type(wasm_number, *((uint8_t*)ptr), value);
            } break;
            case FT_INT16: {
                set_wasm_var_by_type(wasm_number, *((int16_t*)ptr), value);
            } break;
            case FT_UINT16: {
                set_wasm_var_by_type(wasm_number, *((uint16_t*)ptr), value);
            } break;
            case FT_INT32: {
                set_wasm_var_by_type(wasm_number, *((int32_t*)ptr), value);
            } break;
            case FT_UINT32: {
                set_wasm_var_by_type(wasm_number, *((uint32_t*)ptr), value);
            } break;
            case FT_INT64: {
                set_wasm_var_by_type(wasm_number, *((int64_t*)ptr), value);
            } break;
            case FT_UINT64: {
                set_wasm_var_by_type(wasm_number, *((uint64_t*)ptr), value);
            } break;
            case FT_FLOAT: {
                set_wasm_var_by_type(wasm_number, *((float*)ptr), value);
            } break;
            case FT_DOUBLE: {
                set_wasm_var_by_type(wasm_number, *((double*)ptr), value);
            } break;
            case FT_BOOLEAN: {
                set_wasm_var_by_type(uint64_t, *((bool*)ptr), value);
            } break;
            case FT_STRING: {
                wasm_stringref_obj_t obj = NULL;
                const char* str = *(char**)ptr;
                if (!str) {
                    obj = create_wasm_string(exec_env, "");
                } else {
                    obj = create_wasm_string(exec_env, str);
                }
                push_local_obj_ref(exec_env, obj);
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
                void* struct_ptr = *(void**)ptr;
                if (!struct_ptr) {
                    FEATURE_LOG_INFO("struct ptr is null!");
                    return true;
                }
                ObjectMapType& obj_map_type = *(ObjectMapType*)complex_type;
                auto member = obj_map_type.members;
                auto member_count = countMember(member);
                wasm_struct_type_t struct_type = get_struct_type(exec_env, obj_map_type, member_count);
                if (!struct_type) {
                    FEATURE_LOG_ERROR("can not find wasm struct type!");
                    return false;
                }
                wasm_struct_obj_t struct_obj = wasm_struct_obj_new_with_type(exec_env, struct_type);
                for (int i = 0; i < member_count; i++) {
                    void* member_ptr = (void*)((char*)struct_ptr + member->offset);
                    uint64_t feild;
                    bool ret = convertValueToGuest(member->type, member_ptr, exec_env, feild);
                    if (!ret) {
                        FEATURE_LOG_ERROR("convert property name: %s failed !", member->name);
                        return false;
                    }
                    wasm_struct_obj_set_field(struct_obj, i + 1, ((wasm_value_t*)&feild));
                    member++;
                }
                push_local_obj_ref(exec_env, struct_obj);
                set_wasm_var_by_type(void*, struct_obj, value);
            } break;
            case COMPLEX_OPTIONAL: {
                OptionalType* opt_type = (OptionalType*)complex_type;
                bool ret = convertValueToGuest(opt_type->type, ptr, exec_env, value);
                if (!ret) {
                    FEATURE_LOG_ERROR("convert optional to guest failed !");
                    return false;
                }
            } break;
            case COMPLEX_ARRAY: {
                // convert to guest
                FtArray* array = *(FtArray**)ptr;
                if (!array) {
                    FEATURE_LOG_INFO("array ptr is null!");
                    return true;
                }
                auto elem_type = ((ArrayType*)complex_type)->element_type;
                size_t elem_size = sizeof(uintptr_t);
                wasm_struct_obj_t array_struct = create_array_with_type(
                    exec_env, array->_size, ft_type_to_wasm_type(elem_type));
                if (!array_struct) {
                    FEATURE_LOG_ERROR("create array failed !");
                    return false;
                }
                wasm_array_obj_t array_obj = get_array_ref(array_struct);
                for (int32_t i = 0; i < array->_size; i++) {
                    void* elem_ptr = ((char*)array->_element + elem_size * i);
                    // convert element value
                    uint64_t elem = 0;
                    if (!convertValueToGuest(elem_type, elem_ptr, exec_env, elem)) {
                        FEATURE_LOG_ERROR("convert array element to guest failed !");
                        return false;
                    }
                    wasm_array_obj_set_elem(array_obj, i, (wasm_value_t*)&elem);
                }
                push_local_obj_ref(exec_env, array_struct);
                set_wasm_var_by_type(void*, array_struct, value);
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
                *(JSValue*)f_val = *js_value;
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
                ArrayType& array_type = *(ArrayType*)complex_type;
                auto element_type = array_type.element_type;
                wasm_struct_obj_t array_val = get_wasm_args_by_type(wasm_struct_obj_t, value);
                wasm_array_obj_t arr_ref = get_array_ref(array_val);
                len = get_array_length(array_val);
                // FtArray must be a pointer
                if (!*(void**)ptr) {
                    // malloc FtArray struct
                    *(void**)ptr = FeatureMalloc(sizeof(FtArray), ftype);
                }
                FtArray* array = *(FtArray**)ptr;
                array->_size = len;
                if (len) {
                    // we support reference and primitive types
                    size_t elem_size = getValueSize(element_type);
                    auto size = elem_size * len;
                    FEATURE_CHECK_NE(size, 0);
                    array->_element = malloc(size);
                    memset(array->_element, 0, size);
                    for (size_t i = 0; i < len; i++) {
                        // fill it
                        wasm_array_obj_get_elem(arr_ref, i, false, &value1);
                        void* element_ptr = ((char*)array->_element + elem_size * i);
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
} // namespace feature_framework
