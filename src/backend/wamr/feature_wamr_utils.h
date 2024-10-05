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

#ifdef __cplusplus
extern "C" {
#endif

#include "feature_types.h"
#include "gc_export.h"
#include "libdyntype_export.h"
#include "object_utils.h"
#include "quickjs/quickjs.h"
#include "type_utils.h"
#include "wamr_utils.h"
#include "wasm_export.h"

#define set_wasm_var_by_type(type, val, var) ((type&)(var) = (val))

#define get_wasm_args_by_type(type, args) (*((type*)(&args)))

typedef double wasm_number;

static inline void push_local_obj_ref(wasm_exec_env_t exec_env, void* obj)
{
    if (!obj) {
        return;
    }
    wasm_local_obj_ref_t* ref = (wasm_local_obj_ref_t*)malloc(sizeof(wasm_local_obj_ref_t));
    wasm_runtime_push_local_obj_ref(exec_env, ref);
    ref->val = (wasm_obj_t)obj;
}

static inline void pop_local_obj_ref_to_head(wasm_exec_env_t exec_env, wasm_local_obj_ref_t* obj_ref_head)
{
    if (!obj_ref_head) {
        return;
    }
    wasm_local_obj_ref_t* cur_obj_ref = wasm_runtime_get_cur_local_obj_ref(exec_env);
    while (cur_obj_ref != obj_ref_head) {
        cur_obj_ref = wasm_runtime_pop_local_obj_ref(exec_env);
        if (!cur_obj_ref) {
            return;
        }
        free(cur_obj_ref);
        cur_obj_ref = wasm_runtime_get_cur_local_obj_ref(exec_env);
    }
}

/* wasm runtime lib */
JSValue* dynamic_dup_value(JSContext* ctx, JSValue value);
uint32_t get_libdyntype_symbols(char** p_module_name, NativeSymbol** p_native_symbols);
uint32_t get_lib_console_symbols(char** p_module_name, NativeSymbol** p_native_symbols);
uint32_t get_lib_array_symbols(char** p_module_name, NativeSymbol** p_native_symbols);
uint32_t get_lib_timer_symbols(char** p_module_name, NativeSymbol** p_native_symbols);
uint32_t get_struct_indirect_symbols(char** p_module_name, NativeSymbol** p_native_symbols);

dyn_value_t dyntype_callback_wasm_dispatcher(void* exec_env_v, dyn_ctx_t ctx, void* vfunc,
    dyn_value_t this_obj, int argc, dyn_value_t* args);

wasm_anyref_obj_t create_anyref_obj(wasm_exec_env_t exec_env, const void* ptr);
wasm_struct_obj_t create_array_with_type(wasm_exec_env_t exec_env, uint32_t elem_count, wasm_value_type_t elem_type);
int32_t find_struct_type(wasm_exec_env_t exec_env, wasm_value_type_t field_types[],
    uint32_t member_count, wasm_struct_type_t* p_struct_type);
wasm_value_type_t ft_type_to_wasm_type(FeatureType ftype);

#ifdef __cplusplus
}
#endif

#endif // __FEATURE_WAMR_UTILS_H__