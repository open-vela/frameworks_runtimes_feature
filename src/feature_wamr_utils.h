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

/* this file for new feature framework to deal with some basic operatrions about wasm obj */

#ifndef __FEATURE_WAMR_UTILS_H__
#define __FEATURE_WAMR_UTILS_H__

#include "gc_export.h"
#include "wasm_export.h"

extern "C"
{
    wasm_stringref_obj_t create_wasm_string(wasm_exec_env_t exec_env, const char *str);
    uint32_t wasm_string_get_length(wasm_stringref_obj_t str_obj);
    uint32_t wasm_string_to_cstring(wasm_stringref_obj_t str_obj, char *buffer, uint32_t len);
    wasm_struct_obj_t create_wasm_array_with_string(wasm_exec_env_t exec_env, void *ptr, uint32_t arrlen);
    int32_t get_array_struct_type(wasm_module_t wasm_module, int32_t array_type_idx, wasm_struct_type_t *p_struct_type);
    int get_array_length(wasm_struct_obj_t obj);
    wasm_array_obj_t get_array_ref(wasm_struct_obj_t obj);
}

enum field_flag {
    FIELD = 0,
    METHOD = 1,
    GETTER = 2,
    SETTER = 3,
};

typedef enum ts_type {
    TS_OBJECT = 0,
    TS_NULL = 3,
    TS_INT = 5,
    TS_NUMBER = 6,
    TS_BOOLEAN = 7,
    TS_STRING = 9,
    TS_ANY = 10,
    TS_ARRAY = 16,
    TS_FUNCTION = 24,
} ts_type;

typedef struct ts_value_t {
    ts_type type;
    /**
     * Type of the ts value, if it's TS_BOOLEAN or TS_INT, value can be retrieved from of.i32,
     * if it's TS_NUMBER, value can be retrived from f64, otherwise get value from of.ref.
    */
    union {
        int32_t i32;
        double f64;
        void *ref;
    } of;

} ts_value_t;

wasm_struct_obj_t create_wasm_struct(wasm_exec_env_t exec_env, ts_value_t obj_arr[],
                         uint32_t member_count);

#define set_wasm_var_by_type(type, val, var) ((type &)(var) = (val))

#define get_wasm_args_by_type(type, args) (*((type *)(&args)))

#endif // __FEATURE_WAMR_UTILS_H__