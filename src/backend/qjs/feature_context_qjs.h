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
#ifndef __FEATURE_CONTEXT_QJS_H__
#define __FEATURE_CONTEXT_QJS_H__

#include "feature_context.h"
#include "feature_context_private.h"
#include "quickjs/quickjs.h"

#define GET_QJS_CTX(ft_ctx) static_cast<JSContext*>(ft_ctx->data)

typedef struct qjs_val_t {
    JSValue js_val;
} qjs_val_t;

typedef union {
    ft_value_t ft_val;
    qjs_val_t qjs_val;
} ft_qjs_union_t;

#define FT_VAL_TO_QJS(ft_val) \
    (((ft_qjs_union_t*)(void*)&(ft_val))->qjs_val)

#define FT_VAL_TO_QJS_PTR(ft_val) \
    ((qjs_val_t*)(void*)&(ft_val))

#define FT_VAL_GET_JS_VAL(ft_val) \
    (((ft_qjs_union_t*)(void*)&(ft_val))->qjs_val.js_val)

#define FT_VAL_GET_JS_VAL_PTR(ft_val) \
    (&(((ft_qjs_union_t*)(void*)&(ft_val))->qjs_val.js_val))

#define QJS_VAL_TO_FT(qjs_val) \
    (((ft_qjs_union_t*)(void*)&(qjs_val))->ft_val)

#define QJS_VAL_TO_FT_PTR(qjs_val) \
    ((ft_value_t*)(void*)&(qjs_val))

bool InitFeatureContextQjs(ft_context_ref ft_ctx, void* data);

void UninitFeatureContextQjs(ft_context_ref ft_ctx);

#endif // __FEATURE_CONTEXT_QJS_H__
