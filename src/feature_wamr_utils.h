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

#include "gc_export.h"
#include "wasm_export.h"
#include "libdyntype_export.h"
#include "type_utils.h"
#include "wamr_utils.h"
#include "object_utils.h"
#include "quickjs/quickjs.h"

#define set_wasm_var_by_type(type, val, var) ((type &)(var) = (val))

#define get_wasm_args_by_type(type, args) (*((type *)(&args)))

/* wasm runtime lib */
JSValue* dynamic_dup_value(JSContext *ctx, JSValue value);
uint32_t get_libdyntype_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
uint32_t get_lib_console_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
uint32_t get_lib_array_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
uint32_t get_lib_timer_symbols(char **p_module_name, NativeSymbol **p_native_symbols);
uint32_t get_struct_indirect_symbols(char **p_module_name, NativeSymbol **p_native_symbols);

dyn_value_t dyntype_callback_wasm_dispatcher(void* exec_env_v, dyn_ctx_t ctx, void* vfunc,
                         dyn_value_t this_obj, int argc, dyn_value_t* args);

wasm_struct_obj_t create_wasm_struct(wasm_exec_env_t exec_env, ts_value_t obj_arr[],
                         uint32_t member_count);

#ifdef __cplusplus
}
#endif

#endif // __FEATURE_WAMR_UTILS_H__