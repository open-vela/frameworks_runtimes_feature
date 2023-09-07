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
#include "feature_log.h"
#include "feature_utils.h"
#include "feature_instance_wamr.h"

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

extern "C" int32_t get_string_struct_type(wasm_module_t wasm_module,
                                          wasm_struct_type_t *p_struct_type);

extern "C" int32_t get_string_array_type(wasm_module_t wasm_module,
                                         wasm_array_type_t *p_array_type_t);

extern "C" int get_array_length(wasm_struct_obj_t obj);

extern "C" wasm_array_obj_t get_array_ref(wasm_struct_obj_t obj);

char getFeatureSignature(FEATURE::FeatureType featureType)
{
    if (FT_IS_PRIMITIVE(featureType)) {
        switch (FT_GET_VALUE(featureType)) {
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
    } else if (FT_IS_COMPLEX(featureType)) {
        return 'r';
    }
}

wasm_struct_obj_t getWasmString(wasm_exec_env_t exec_env, const char *str)
{
    char *p, *p_end;
    wasm_value_t value = {0};
    wasm_array_obj_t new_arr;
    wasm_struct_type_t string_struct_type = nullptr;
    wasm_struct_obj_t new_string_struct = nullptr;
    wasm_array_type_t string_array_type = nullptr;
    wasm_local_obj_ref_t local_ref = {0};
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    uint32 len = strlen(str);
    /* get struct_string_type */
    int32_t id = get_string_struct_type(module, &string_struct_type);

    bh_assert(string_struct_type != nullptr);
    bh_assert(wasm_defined_type_is_struct_type((wasm_defined_type_t)string_struct_type));
    /* wrap with string struct */
    new_string_struct = wasm_struct_obj_new_with_type(exec_env, string_struct_type);
    if (!new_string_struct) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env), "alloc memory failed");
        goto fail;
    }

    /* Push object to local ref to avoid being freed at next allocation */
    wasm_runtime_push_local_object_ref(exec_env, &local_ref);
    local_ref.val = (wasm_obj_t)new_string_struct;

    /* Create new array for holding string contents */
    value.i32 = 0;
    get_string_array_type(module, &string_array_type);
    new_arr = wasm_array_obj_new_with_type(exec_env, string_array_type, len, &value);

    if (!new_arr) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env), "alloc memory failed");
        goto fail;
    }

    p = (char *)wasm_array_obj_first_elem_addr(new_arr);
    p_end = p + len;
    bh_assert(p);
    bh_assert(p_end);

    bh_memcpy_s(p, len, str, len);
    p += len;
    bh_assert(p == p_end);

    value.gc_obj = (wasm_obj_t)new_arr;
    wasm_struct_obj_set_field(new_string_struct, 1, &value);
    /* pop local ref before return */
    wasm_runtime_pop_local_object_ref(exec_env);
    (void)p_end;
    return new_string_struct;

fail:
    if (local_ref.val) {
        wasm_runtime_pop_local_object_ref(exec_env);
    }
    return nullptr;
}

bool convertValueToGuest(FeatureInstance* instance, FEATURE::FeatureType featureType, void* ptr,
    wasm_exec_env_t exec_env,  wasm_val_t& value)
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
            }
            case FT_INT: {
                //value = feature_int(ctx, *((int32_t*)ptr));
                value.of.i32 = *((int32_t*)ptr);
                value.kind = WASM_I32;
                break;
            }
            // case FT_INT8: {
            //     value = feature_int(ctx, *((int8_t*)ptr));
            // } break;
            // case FT_UINT8: {
            //     value = feature_uint(ctx, *((uint8_t*)ptr));
            // } break;
            // case FT_INT16: {
            //     value = feature_int(ctx, *((int16_t*)ptr));
            // } break;
            // case FT_UINT16: {
            //     value = feature_uint(ctx, *((uint16_t*)ptr));
            // } break;
            case FT_INT32: {
                //value = feature_int(ctx, *((int32_t*)ptr));
                value.of.i32 = *((int32_t*)ptr);
                value.kind = WASM_I32;
                break;
            }
            case FT_UINT32: {
                //value = feature_uint(ctx, *((uint32_t*)ptr));
                value.of.i32 = *((int32_t*)ptr);
                value.kind = WASM_I32;
                break;
            }
            // case FT_INT64: {
            //     value = feature_int64(ctx, *((int64_t*)ptr));
            // } break;
            // case FT_UINT64: {
            //     value = feature_uint64(ctx, *((uint64_t*)ptr));
            // } break;
            // case FT_FLOAT: {
            //     value = feature_double(ctx, *((float*)ptr));
            // } break;
            // case FT_DOUBLE: {
            //     value = feature_double(ctx, *((double*)ptr));
            // } break;
            // case FT_BOOLEAN: {
            //     value = feature_boolean(ctx, *((bool*)ptr));
            // } break;
            case FT_CHAR: {
                value.of.foreign = (uintptr_t)ptr;
                value.kind = WASM_ANYREF;
                // value = feature_string(ctx, (const char*)ptr);
                break;
            }
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
    }
    return true;
}

bool convertValueToHost(FeatureInstance* instance, FEATURE::FeatureType featureType, void*& ptr,
	wasm_exec_env_t exec_env, uint64_t* value)
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
        if (!convertValueToHost(instance, FT_REMOVE_REFERENCE(featureType), value_ptr, exec_env, value)) {
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
            }
            case FT_INT: {
                native_raw_get_arg(double, number_value, value);
                // if (!feature_to_int(ctx, (int32_t*)ptr, value)) {
                //	   FEATURE_LOG_ERROR("convert to int32 failed !");
                //	   return false;
                // }
                *(int32_t*)ptr = (int32_t)number_value;
                break;
            }
            case FT_INT32: {
                native_raw_get_arg(double, number_value, value);
                // if (!feature_to_int(ctx, (int32_t*)ptr, value)) {
                //	   FEATURE_LOG_ERROR("convert to int32 failed !");
                //	   return false;
                // }
                *(int32_t*)ptr = (int32_t)number_value;
                break;
            }
            case FT_UINT32: {
                native_raw_get_arg(double, number_value, value);
                *(uint32_t*)ptr = (uint32_t)number_value;
                break;
            }
            case FT_CHAR: {
                native_raw_get_arg(void *, str, value);
                wasm_value_t arr_obj = { 0 };
                wasm_obj_t str_ref = (wasm_obj_t)str;
                wasm_struct_obj_get_field((wasm_struct_obj_t)str, 1, false, &arr_obj);
                WASMArrayObjectRef arr_ref = (WASMArrayObjectRef)(arr_obj.gc_obj);
                int arrlen = wasm_array_obj_length(arr_ref);
                char *p_str = (char *)wasm_array_obj_first_elem_addr(arr_ref);
                // const char* str = feature_to_cstring(ctx, value);
                char* alloc_ptr = (char*)FTMalloc(strlen(p_str) + 1, FT_CHAR);
                strcpy(alloc_ptr, p_str);
                ptr = alloc_ptr;
                // feature_free_cstring(ctx, str);
                break;
            }
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return false;
            }
        }
    } else if (FT_IS_COMPLEX(featureType)) {
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        switch (complexType->type) {
            case COMPLEX_CALLBACK:
            {
                // save into instance
                CallbackType *callbackType = (CallbackType *)complexType;
                native_raw_get_arg(wasm_obj_t, cb_value, value);
                //*(int32_t*)ptr = (int32_t)number_value;
                FEATURE::FeatureCallbackId id = ((FeatureInstanceWamr*)instance)->addCallback(cb_value, callbackType);
                *(FeatureCallbackId *)ptr = id; // write callback id to pointer.
                break;
            }
            case COMPLEX_ARRAY:
            {
                uint32_t len;
                wasm_value_t value1 = {0};
                ArrayType &arrayType = *(ArrayType *)complexType;
                auto element_type = arrayType.element_type;
                native_raw_get_arg(wasm_struct_obj_t, arrayValue, value);
                wasm_array_obj_t arr_ref = get_array_ref(arrayValue);
                len = get_array_length(arrayValue);
                FTArray *arrayData = (FTArray *)ptr;
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
                break;
            }
            //	   case COMPLEX_PROMISE: {
            //		   FEATURE_LOG_ERROR("do not support convert promise to guest !");
            //		   return false;
            //	   } break;
            //	   default: {
            //		   FEATURE_LOG_ERROR("unsupported complex type !");
            //		   return false;
            //	   }
        }
    }
    return true;
}

}
} // namespace ferry

