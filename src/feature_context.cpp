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

#include "feature_context_private.h"
#include "feature_context_qjs.h"
#include <stdio.h>
#include <cstring>
#include <malloc.h>

ft_context_ref CreateFeatureContextQjs(void* data) {
    FeatureContext* ft_ctx = (FeatureContext*)malloc(sizeof(FeatureContext));
    memset(ft_ctx, 0, sizeof(FeatureContext));
    InitFeatureContextQjs(ft_ctx, data);
    return ft_ctx;
}
void ReleaseFeatureContextQjs(ft_context_ref ft_ctx) {
    if (!ft_ctx)
        return;

    UninitFeatureContextQjs(ft_ctx);
    free(ft_ctx);
}

void* ft_context_get_data (ft_context_ref ft_ctx) {
    return ft_ctx->data;
}

ft_type ft_get_type (ft_context_ref ft_ctx, ft_value_t ft_val) {
    return ft_ctx->ft_get_type(ft_ctx, ft_val);
}

// value creation
ft_value_t ft_from_int (ft_context_ref ft_ctx, int32_t val) {
    return ft_ctx->ft_from_int(ft_ctx, val);
}

ft_value_t ft_from_uint (ft_context_ref ft_ctx, uint32_t val) {
    return ft_ctx->ft_from_uint(ft_ctx, val);
}

ft_value_t ft_from_int64 (ft_context_ref ft_ctx, int64_t val) {
    return ft_ctx->ft_from_int64(ft_ctx, val);
}

ft_value_t ft_from_uint64 (ft_context_ref ft_ctx, uint64_t val) {
    return ft_ctx->ft_from_uint64(ft_ctx, val);
}

ft_value_t ft_from_double (ft_context_ref ft_ctx, double val) {
    return ft_ctx->ft_from_double(ft_ctx, val);
}

ft_value_t ft_from_bool (ft_context_ref ft_ctx, bool val) {
    return ft_ctx->ft_from_bool(ft_ctx, val);
}

ft_value_t ft_from_string (ft_context_ref ft_ctx, const char* val) {
    return ft_ctx->ft_from_string(ft_ctx, val);
}

ft_value_t ft_from_int_array (ft_context_ref ft_ctx, int32_t* val, uint32_t size) {
    return ft_ctx->ft_from_int_array(ft_ctx, val, size);
}

ft_value_t ft_from_uint_array (ft_context_ref ft_ctx, uint32_t* val, uint32_t size) {
    return ft_ctx->ft_from_uint_array(ft_ctx, val, size);
}

ft_value_t ft_from_int64_array (ft_context_ref ft_ctx, int64_t* val, uint32_t size) {
    return ft_ctx->ft_from_int64_array(ft_ctx, val, size);
}

ft_value_t ft_from_uint64_array (ft_context_ref ft_ctx, uint64_t* val, uint32_t size) {
    return ft_ctx->ft_from_uint64_array(ft_ctx, val, size);
}

ft_value_t ft_from_bool_array (ft_context_ref ft_ctx, bool* val, uint32_t size) {
    return ft_ctx->ft_from_bool_array(ft_ctx, val, size);
}

ft_value_t ft_from_double_array (ft_context_ref ft_ctx, double* val, uint32_t size) {
    return ft_ctx->ft_from_double_array(ft_ctx, val, size);
}

ft_value_t ft_from_string_array (ft_context_ref ft_ctx, const char** val, uint32_t size) {
    return ft_ctx->ft_from_string_array(ft_ctx, val, size);
}

ft_value_t ft_parse_json (ft_context_ref ft_ctx, const char* buf, size_t buf_len, const char* filename) {
    return ft_ctx->ft_parse_json(ft_ctx, buf, buf_len, filename);
}

// convert
bool ft_to_int (ft_context_ref ft_ctx, ft_value_t f_val, int32_t* val) {
    return ft_ctx->ft_to_int(ft_ctx, f_val, val);
}

bool ft_to_uint (ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* val) {
    return ft_ctx->ft_to_uint(ft_ctx, f_val, val);
}

bool ft_to_int64 (ft_context_ref ft_ctx, ft_value_t f_val, int64_t* val) {
    return ft_ctx->ft_to_int64(ft_ctx, f_val, val);
}

bool ft_to_uint64 (ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* val) {
    return ft_ctx->ft_to_uint64(ft_ctx, f_val, val);
}

bool ft_to_double (ft_context_ref ft_ctx, ft_value_t f_val, double* val) {
    return ft_ctx->ft_to_double(ft_ctx, f_val, val);
}

bool ft_to_bool (ft_context_ref ft_ctx, ft_value_t f_val, bool* val) {
    return ft_ctx->ft_to_bool(ft_ctx, f_val, val);
}

const char* ft_to_string(ft_context_ref ft_ctx, ft_value_t f_val) {
    return ft_ctx->ft_to_string(ft_ctx, f_val);
}

uint32_t ft_array_size(ft_context_ref ft_ctx, const ft_value_t array) {
    return ft_ctx->ft_array_size(ft_ctx, array);
}

ft_value_t ft_array_at(ft_context_ref ft_ctx, const ft_value_t array, uint32_t idx) {
    return ft_ctx->ft_array_at(ft_ctx, array, idx);
}

// object operations
ft_value_t ft_obj_get_property (ft_context_ref ft_ctx, ft_value_t obj, const char* prop) {
    return ft_ctx->ft_obj_get_property(ft_ctx, obj, prop);
}

bool ft_obj_set_property (ft_context_ref ft_ctx, ft_value_t obj, const char* prop, ft_value_t ft_val) {
    return ft_ctx->ft_obj_set_property(ft_ctx, obj, prop, ft_val);
}

void ft_free_value (ft_context_ref ft_ctx, ft_value_t ft_val) {
    ft_ctx->ft_free_value(ft_ctx, ft_val);
}

void ft_free_string (ft_context_ref ft_ctx, const char* str) {
    ft_ctx->ft_free_string(ft_ctx, str);
}

