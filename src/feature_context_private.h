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
#ifndef __FEATURE_RUNTIME_CONTEXT_PRIVATE_H__
#define __FEATURE_RUNTIME_CONTEXT_PRIVATE_H__

#include "feature_context.h"
#include "feature_main_exports.h"
#include "thread_checker.h"

ft_context_ref CreateFeatureContextQjs(void* data);

void ReleaseFeatureContextQjs(ft_context_ref ft_ctx);

ft_context_ref CreateFeatureContextWamr(void* data1, void* data2);

void ReleaseFeatureContextWamr(ft_context_ref ft_ctx);

void SetReleaseRawContextCb(ft_context_ref ft_ctx, ReleaseRawContextCb cb);

void ft_dup_value(ft_context_ref ft_ctx, ft_value_t ft_val);

typedef struct FeatureContext {
    void* data;
    ReleaseRawContextCb release_raw_ctx_cb;

#ifdef CONFIG_FEATURE_ENABLE_THREAD_CHECKER
    ThreadChecker* thread_checker;
#endif
    ft_type (*ft_get_type)(ft_context_ref ft_ctx, ft_value_t ft_val);

    // feature type creation from native types
    ft_value_t (*ft_from_int)(ft_context_ref ft_ctx, int32_t val);
    ft_value_t (*ft_from_uint)(ft_context_ref ft_ctx, uint32_t val);
    ft_value_t (*ft_from_int64)(ft_context_ref ft_ctx, int64_t val);
    ft_value_t (*ft_from_uint64)(ft_context_ref ft_ctx, uint64_t val);
    ft_value_t (*ft_from_double)(ft_context_ref ft_ctx, double val);
    ft_value_t (*ft_from_bool)(ft_context_ref ft_ctx, bool val);
    ft_value_t (*ft_from_string)(ft_context_ref ft_ctx, const char* val);
    ft_value_t (*ft_from_buffer)(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size);
    ft_value_t (*ft_from_typed_buffer)(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size, FtTypedArrayType type);

    ft_value_t (*ft_from_int_array)(ft_context_ref ft_ctx, int32_t* val, uint32_t size);
    ft_value_t (*ft_from_uint_array)(ft_context_ref ft_ctx, uint32_t* val, uint32_t size);
    ft_value_t (*ft_from_int64_array)(ft_context_ref ft_ctx, int64_t* val, uint32_t size);
    ft_value_t (*ft_from_uint64_array)(ft_context_ref ft_ctx, uint64_t* val, uint32_t size);
    ft_value_t (*ft_from_double_array)(ft_context_ref ft_ctx, double* val, uint32_t size);
    ft_value_t (*ft_from_bool_array)(ft_context_ref ft_ctx, bool* val, uint32_t size);
    ft_value_t (*ft_from_string_array)(ft_context_ref ft_ctx, const char** val, uint32_t size);
    ft_value_t (*ft_parse_json)(ft_context_ref ft_ctx, const char* buf, size_t buf_len, const char* filename);

    // feature type to native types
    bool (*ft_to_int)(ft_context_ref ft_ctx, ft_value_t f_val, int32_t* val);
    bool (*ft_to_uint)(ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* val);
    bool (*ft_to_int64)(ft_context_ref ft_ctx, ft_value_t f_val, int64_t* val);
    bool (*ft_to_uint64)(ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* val);
    bool (*ft_to_double)(ft_context_ref ft_ctx, ft_value_t f_val, double* val);
    bool (*ft_to_bool)(ft_context_ref ft_ctx, ft_value_t ft_val, bool* val);
    const char* (*ft_to_string)(ft_context_ref ft_ctx, ft_value_t f_val);
    uint8_t* (*ft_to_buffer)(ft_context_ref ft_ctx, size_t* p_size, ft_value_t f_val);

    // array operations
    uint32_t (*ft_array_size)(ft_context_ref ft_ctx, const ft_value_t array);
    ft_value_t (*ft_array_at)(ft_context_ref ft_ctx, const ft_value_t array, uint32_t idx);

    // object operations
    ft_value_t (*ft_new_object)(ft_context_ref ft_ctx);
    ft_value_t (*ft_obj_get_property)(ft_context_ref ft_ctx, ft_value_t ft_val, const char* prop);
    bool (*ft_obj_set_property)(ft_context_ref ft_ctx, ft_value_t obj, const char* prop, ft_value_t val);
    void (*ft_free_value)(ft_context_ref ft_ctx, ft_value_t ft_val);
    void (*ft_dup_value)(ft_context_ref ft_ctx, ft_value_t ft_val);
    void (*ft_free_string)(ft_context_ref ft_ctx, const char* str);
    ft_value_t (*ft_undefined)(ft_context_ref ft_ctx);

} FeatureContext;

#endif // __FEATURE_RUNTIME_CONTEXT_PRIVATE_H__
