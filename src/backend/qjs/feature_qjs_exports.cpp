/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#include "feature_qjs_exports.h"
#include "feature_context_qjs.h"
#include "feature_manager_qjs.h"

using namespace feature_framework;

ft_value_t ft_from_jsvalue(ft_context_ref rt_ctx, JSValue val)
{
    ft_value_t ft_val {};
    auto js_val_ptr = FT_VAL_GET_JS_VAL_PTR(ft_val);
    *js_val_ptr = val;
    return ft_val;
}

JSValue ft_to_jsvalue(ft_context_ref rt_ctx, ft_value_t ft_val)
{
    return FT_VAL_GET_JS_VAL(ft_val);
}

JSContext* ft_ctx_to_js_ctx(ft_context_ref rt_ctx)
{
    return (JSContext*)rt_ctx->data;
}

JSClassID get_feature_classid()
{
    return FeatureManagerQjs::jsClassId();
}