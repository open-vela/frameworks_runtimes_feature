 /*
  * Copyright (C) 2023 Xiaomi Corporation
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * 	 http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

#include "feature_context_wamr.h"

#include "gc_object.h"
#include "quickjs/quickjs.h"

#include <stdio.h>
#include <malloc.h>

#define GET_WAMR_ENV(ft_ctx) (((WamrContext*)(ft_ctx->data))->exec_env)

extern "C" int32_t get_string_struct_type(wasm_module_t wasm_module,
                                          wasm_struct_type_t *p_struct_type);

extern "C" int32_t get_string_array_type(wasm_module_t wasm_module,
                                         wasm_array_type_t *p_array_type_t);

extern "C" int get_array_length(wasm_struct_obj_t obj);

extern "C" wasm_array_obj_t get_array_ref(wasm_struct_obj_t obj);

typedef struct WamrContext {
	wasm_exec_env_t exec_env;
	JSContext* js_cxt;
} WamrContext;

static ft_type _ft_get_type (ft_context_ref ft_ctx, ft_value_t ft_val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    return FT_TYPE_NONE;
}

// value creation
static ft_value_t _ft_int(ft_context_ref ft_ctx, int32_t val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    ret.wm_val.of.i32 = val;
    ret.wm_val.kind = WASM_I32;
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_uint(ft_context_ref ft_ctx, uint32_t val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    ret.wm_val.of.i32 = (int32_t)val; // to be fixed
    ret.wm_val.kind = WASM_I32;
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_int64(ft_context_ref ft_ctx, int64_t val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    ret.wm_val.of.i64 = val;
    ret.wm_val.kind = WASM_I64;
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_uint64(ft_context_ref ft_ctx, uint64_t val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    ret.wm_val.of.i64 = (int64_t)val;  // to be fixed
    ret.wm_val.kind = WASM_I64;
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_double(ft_context_ref ft_ctx, double val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    ret.wm_val.of.f64 = val;
    ret.wm_val.kind = WASM_F64;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_boolean(ft_context_ref ft_ctx, bool val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    ret.wm_val.of.i32 = (int32_t)val; // to be fixed
    ret.wm_val.kind = WASM_I32;
    return WM_VAL_TO_FT(ret);
}

static wasm_struct_obj_t to_wamr_str(wasm_exec_env_t exec_env, const char *str)
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
    if (!new_string_struct)
    {
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

    if (!new_arr)
    {
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
    if (local_ref.val)
    {
        wasm_runtime_pop_local_object_ref(exec_env);
    }
    return nullptr;
}

static ft_value_t _ft_string(ft_context_ref ft_ctx, const char* val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wasm_struct_obj_t str_obj = to_wamr_str(wasm_env, val);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_int_array (ft_context_ref ft_ctx, int32_t* val, uint32_t size) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_uint_array (ft_context_ref ft_ctx, uint32_t* val, uint32_t size) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_int64_array (ft_context_ref ft_ctx, int64_t* val, uint32_t size) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_uint64_array (ft_context_ref ft_ctx, uint64_t* val, uint32_t size) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_double_array (ft_context_ref ft_ctx, double* val, uint32_t size) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_bool_array (ft_context_ref ft_ctx, bool* val, uint32_t size) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

static ft_value_t _ft_string_array (ft_context_ref ft_ctx, const char** val, uint32_t size) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    wamr_val_t ret;
    // to be implemented
    return WM_VAL_TO_FT(ret);
}

// convert
static const char* _ft_to_string(ft_context_ref ft_ctx, ft_value_t f_val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    const char* ret_str = "hellp";
    return ret_str;
}

static bool _ft_to_int(ft_context_ref ft_ctx, ft_value_t f_val, int32_t* pres) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return true;
}

static bool _ft_to_uint(ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* pres) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return true;
}

static bool _ft_to_int64(ft_context_ref ft_ctx, ft_value_t f_val, int64_t* pres) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return true;
}

static bool _ft_to_uint64(ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* pres) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return true;
}

static bool _ft_to_double(ft_context_ref ft_ctx, ft_value_t f_val, double* pres) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return true;
}

static bool _ft_to_bool(ft_context_ref ft_ctx, ft_value_t f_val, bool* b) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return true;
}

static uint32_t _ft_array_size(ft_context_ref ft_ctx, const ft_value_t f_obj) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return 0;
}

static ft_value_t _ft_array_at(ft_context_ref ft_ctx, const ft_value_t f_obj, uint32_t idx) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    wamr_val_t ret;
    return WM_VAL_TO_FT(ret);
}

// object operations
static ft_value_t _ft_obj_get_property(ft_context_ref ft_ctx, ft_value_t f_obj, const char* key) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    wamr_val_t ret;
    return WM_VAL_TO_FT(ret);
}

static bool _ft_obj_set_property(ft_context_ref ft_ctx, ft_value_t f_obj, const char* prop, ft_value_t f_val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
    return true;
}

// free value
static void _ft_free_value(ft_context_ref ft_ctx, ft_value_t f_val) {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
}

static void _ft_free_string (ft_context_ref ft_ctx, const char* str)  {
    wasm_exec_env_t wasm_env = GET_WAMR_ENV(ft_ctx);
    // to be implemented
}

bool InitFeatureContextWamr(ft_context_ref rt_ctx, void* data0, void* data1){

    WamrContext* wamr_context = (WamrContext*)malloc(sizeof(WamrContext));
    wamr_context->exec_env = (wasm_exec_env_t)data0;
    wamr_context->js_cxt = (JSContext*)data1;
    rt_ctx->data = (void*)wamr_context;

    rt_ctx->ft_get_type = _ft_get_type;
    // value creation
    rt_ctx->ft_from_int = _ft_int;
    rt_ctx->ft_from_uint = _ft_uint;
    rt_ctx->ft_from_int64 = _ft_int64;
    rt_ctx->ft_from_uint64 = _ft_uint64;
    rt_ctx->ft_from_double = _ft_double;
    rt_ctx->ft_from_bool = _ft_boolean;
    rt_ctx->ft_from_string = _ft_string;

    rt_ctx->ft_from_int_array = _ft_int_array;
    rt_ctx->ft_from_uint_array = _ft_uint_array;
    rt_ctx->ft_from_int64_array = _ft_int64_array;
    rt_ctx->ft_from_uint64_array = _ft_uint64_array;
    rt_ctx->ft_from_double_array = _ft_double_array;
    rt_ctx->ft_from_bool_array = _ft_bool_array;
    rt_ctx->ft_from_string_array = _ft_string_array;

    // convert
    rt_ctx->ft_to_int = _ft_to_int;
    rt_ctx->ft_to_uint = _ft_to_uint;
    rt_ctx->ft_to_int64 = _ft_to_int64;
    rt_ctx->ft_to_uint64 = _ft_to_uint64;
    rt_ctx->ft_to_double = _ft_to_double;
    rt_ctx->ft_to_bool = _ft_to_bool;
    rt_ctx->ft_to_string = _ft_to_string;
    // array operations
    rt_ctx->ft_array_size = _ft_array_size;
    rt_ctx->ft_array_at = _ft_array_at;
    // object operations
    rt_ctx->ft_obj_get_property = _ft_obj_get_property;
    rt_ctx->ft_obj_set_property = _ft_obj_set_property;
    // free value
    rt_ctx->ft_free_value = _ft_free_value;
    rt_ctx->ft_free_string = _ft_free_string;
    return true;
}

void UninitFeatureContextWamr(ft_context_ref rt_ctx) {
    free(rt_ctx->data);
}


