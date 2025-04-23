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

#include "backend/qjs/feature_context_qjs.h"
#include "feature_context_private.h"
#ifdef CONFIG_FEATURE_USE_WAMR
#include "backend/wamr/feature_context_wamr.h"
#endif

#include "thread_checker.h"
#include <cstring>
#include <malloc.h>
#include <stdio.h>

ft_context_ref CreateFeatureContextQjs(void* data)
{
    FeatureContext* ft_ctx = (FeatureContext*)malloc(sizeof(FeatureContext));
    memset(ft_ctx, 0, sizeof(FeatureContext));
    InitFeatureContextQjs(ft_ctx, data);
    return ft_ctx;
}

void ReleaseFeatureContextQjs(ft_context_ref ft_ctx)
{
    if (!ft_ctx)
        return;

    UninitFeatureContextQjs(ft_ctx);
    if (ft_ctx->release_raw_ctx_cb) {
        ft_ctx->release_raw_ctx_cb(ft_ctx->data);
    }
    free(ft_ctx);
}

ft_context_ref CreateFeatureContextWamr(void* data0, void* data1)
{
    FeatureContext* ft_ctx = NULL;
#ifdef CONFIG_FEATURE_USE_WAMR
    ft_ctx = (FeatureContext*)malloc(sizeof(FeatureContext));
    memset(ft_ctx, 0, sizeof(FeatureContext));
    InitFeatureContextWamr(ft_ctx, data0, data1);
#endif
    return ft_ctx;
}

void ReleaseFeatureContextWamr(ft_context_ref ft_ctx)
{
#ifdef CONFIG_FEATURE_USE_WAMR
    if (!ft_ctx)
        return;

    UninitFeatureContextWamr(ft_ctx);
    if (ft_ctx->release_raw_ctx_cb) {
        ft_ctx->release_raw_ctx_cb(ft_ctx->data);
    }
    free(ft_ctx);
#endif
}

void SetReleaseRawContextCb(ft_context_ref ft_ctx, ReleaseRawContextCb cb)
{
    if (!ft_ctx)
        return;

    ft_ctx->release_raw_ctx_cb = cb;
}

void* ft_context_get_data(ft_context_ref ft_ctx)
{
    return ft_ctx->data;
}

ft_type ft_get_type(ft_context_ref ft_ctx, ft_value_t ft_val)
{
    return ft_ctx->ft_get_type(ft_ctx, ft_val);
}

// value creation
ft_value_t ft_from_int(ft_context_ref ft_ctx, int32_t val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_int(ft_ctx, val);
}

ft_value_t ft_from_uint(ft_context_ref ft_ctx, uint32_t val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_uint(ft_ctx, val);
}

ft_value_t ft_from_int64(ft_context_ref ft_ctx, int64_t val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_int64(ft_ctx, val);
}

ft_value_t ft_from_uint64(ft_context_ref ft_ctx, uint64_t val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_uint64(ft_ctx, val);
}

ft_value_t ft_from_double(ft_context_ref ft_ctx, double val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_double(ft_ctx, val);
}

ft_value_t ft_from_bool(ft_context_ref ft_ctx, bool val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_bool(ft_ctx, val);
}

ft_value_t ft_from_string(ft_context_ref ft_ctx, const char* val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_string(ft_ctx, val);
}

ft_value_t ft_from_buffer(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_buffer(ft_ctx, buff, size);
}

ft_value_t ft_from_int_array(ft_context_ref ft_ctx, int32_t* val, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_int_array(ft_ctx, val, size);
}

ft_value_t ft_from_typed_buffer(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size, FtTypedArrayType type)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_typed_buffer(ft_ctx, buff, size, type);
}

ft_value_t ft_from_uint_array(ft_context_ref ft_ctx, uint32_t* val, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_uint_array(ft_ctx, val, size);
}

ft_value_t ft_from_int64_array(ft_context_ref ft_ctx, int64_t* val, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_int64_array(ft_ctx, val, size);
}

ft_value_t ft_from_uint64_array(ft_context_ref ft_ctx, uint64_t* val, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_uint64_array(ft_ctx, val, size);
}

ft_value_t ft_from_bool_array(ft_context_ref ft_ctx, bool* val, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_bool_array(ft_ctx, val, size);
}

ft_value_t ft_from_double_array(ft_context_ref ft_ctx, double* val, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_double_array(ft_ctx, val, size);
}

ft_value_t ft_from_string_array(ft_context_ref ft_ctx, const char** val, uint32_t size)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_from_string_array(ft_ctx, val, size);
}

ft_value_t ft_parse_json(ft_context_ref ft_ctx, const char* buf, size_t buf_len, const char* filename)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_parse_json(ft_ctx, buf, buf_len, filename);
}

// convert
bool ft_to_int(ft_context_ref ft_ctx, ft_value_t f_val, int32_t* val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_int(ft_ctx, f_val, val);
}

bool ft_to_uint(ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_uint(ft_ctx, f_val, val);
}

bool ft_to_int64(ft_context_ref ft_ctx, ft_value_t f_val, int64_t* val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_int64(ft_ctx, f_val, val);
}

bool ft_to_uint64(ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_uint64(ft_ctx, f_val, val);
}

bool ft_to_double(ft_context_ref ft_ctx, ft_value_t f_val, double* val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_double(ft_ctx, f_val, val);
}

bool ft_to_bool(ft_context_ref ft_ctx, ft_value_t f_val, bool* val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_bool(ft_ctx, f_val, val);
}

const char* ft_to_string(ft_context_ref ft_ctx, ft_value_t f_val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_string(ft_ctx, f_val);
}

uint8_t* ft_to_buffer(ft_context_ref ft_ctx, size_t* p_size, ft_value_t f_val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_to_buffer(ft_ctx, p_size, f_val);
}

uint32_t ft_array_size(ft_context_ref ft_ctx, const ft_value_t array)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_array_size(ft_ctx, array);
}

ft_value_t ft_array_at(ft_context_ref ft_ctx, const ft_value_t array, uint32_t idx)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_array_at(ft_ctx, array, idx);
}

// object operations
ft_value_t ft_new_object(ft_context_ref ft_ctx)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_new_object(ft_ctx);
}

ft_value_t ft_obj_get_property(ft_context_ref ft_ctx, ft_value_t obj, const char* prop)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_obj_get_property(ft_ctx, obj, prop);
}

bool ft_obj_set_property(ft_context_ref ft_ctx, ft_value_t obj, const char* prop, ft_value_t ft_val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    return ft_ctx->ft_obj_set_property(ft_ctx, obj, prop, ft_val);
}

void ft_free_value(ft_context_ref ft_ctx, ft_value_t ft_val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    ft_ctx->ft_free_value(ft_ctx, ft_val);
}

void ft_free_string(ft_context_ref ft_ctx, const char* str)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    ft_ctx->ft_free_string(ft_ctx, str);
}

ft_value_t ft_undefined(ft_context_ref ft_ctx)
{
    return ft_ctx->ft_undefined(ft_ctx);
}

void ft_dup_value(ft_context_ref ft_ctx, ft_value_t ft_val)
{
    THREAD_CHECK(ft_ctx->thread_checker);
    ft_ctx->ft_dup_value(ft_ctx, ft_val);
}