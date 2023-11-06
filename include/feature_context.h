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
#ifndef __FEATURE_CONTEXT_H__
#define __FEATURE_CONTEXT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>

typedef enum ft_type {
  FT_TYPE_NONE = 0,
  FT_TYPE_NUMBER,
  FT_TYPE_BOOL,
  FT_TYPE_STRING,
  FT_TYPE_ARRAY,
  FT_TYPE_OBJECT
} ft_type;

typedef struct ft_value_t {
#if INTPTR_MAX >= INT64_MAX
    uint64_t val[2];
#else
    uint64_t val;
#endif
    ft_type type;
} ft_value_t;

typedef ft_value_t *ft_value_ref;
typedef const ft_value_t ft_value_const;

struct FeatureContext;
typedef struct FeatureContext *ft_context_ref;

void* ft_context_get_data (ft_context_ref ft_ctx);

ft_type ft_get_type (ft_context_ref ft_ctx, ft_value_t ft_val);

// feature type creation from native types
ft_value_t ft_from_int(ft_context_ref ft_ctx, int32_t val);
ft_value_t ft_from_uint(ft_context_ref ft_ctx, uint32_t val);
ft_value_t ft_from_int64(ft_context_ref ft_ctx, int64_t val);
ft_value_t ft_from_uint64(ft_context_ref ft_ctx, uint64_t val);
ft_value_t ft_from_double(ft_context_ref ft_ctx, double val);
ft_value_t ft_from_bool(ft_context_ref ft_ctx, bool val);
ft_value_t ft_from_string(ft_context_ref ft_ctx, const char* val);

ft_value_t ft_from_int_array (ft_context_ref ft_ctx, int32_t* val, uint32_t size);
ft_value_t ft_from_uint_array (ft_context_ref ft_ctx, uint32_t* val, uint32_t size);
ft_value_t ft_from_int64_array (ft_context_ref ft_ctx, int64_t* val, uint32_t size);
ft_value_t ft_from_uint64_array (ft_context_ref ft_ctx, uint64_t* val, uint32_t size);
ft_value_t ft_from_bool_array (ft_context_ref ft_ctx, bool* val, uint32_t size);
ft_value_t ft_from_double_array (ft_context_ref ft_ctx, double* val, uint32_t size);
ft_value_t ft_from_string_array (ft_context_ref ft_ctx, const char** val, uint32_t size);

ft_value_t ft_prase_json (ft_context_ref ft_ctx, const char* buf, size_t buf_len, const char* filename);

// feature type to native types
bool ft_to_int (ft_context_ref ft_ctx, ft_value_t f_val, int32_t* val);
bool ft_to_uint (ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* val);
bool ft_to_int64 (ft_context_ref ft_ctx, ft_value_t f_val, int64_t* val);
bool ft_to_uint64 (ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* val);
bool ft_to_double (ft_context_ref ft_ctx, ft_value_t f_val, double* val);
bool ft_to_bool (ft_context_ref ft_ctx, ft_value_t ft_val, bool* val);
const char* ft_to_string (ft_context_ref ft_ctx, ft_value_t f_val);

// array operations
uint32_t ft_array_size(ft_context_ref ft_ctx, const ft_value_t array);
ft_value_t ft_array_at(ft_context_ref ft_ctx, const ft_value_t array, uint32_t idx);

// object operations
ft_value_t ft_obj_get_property (ft_context_ref ft_ctx, ft_value_t ft_val, const char* prop);
bool ft_obj_set_property (ft_context_ref ft_ctx, ft_value_t obj, const char* prop, ft_value_t val);
void ft_free_value (ft_context_ref ft_ctx, ft_value_t ft_val);
void ft_free_string (ft_context_ref ft_ctx, const char* str);

#ifdef __cplusplus
}
#endif

#endif // __FEATURE_CONTEXT_H__