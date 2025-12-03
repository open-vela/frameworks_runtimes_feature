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

#include "feature_context_qjs.h"
#include "feature_qjs_exports.h"
// clang-format off
#include "value_translator_qjs.h"
#include "feature_value_translator.h"
// clang-format on

#include <malloc.h>
#include <stdio.h>

ft_type _ft_get_type(ft_context_ref ft_ctx, ft_value_t ft_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JSValue js_val = FT_VAL_GET_JS_VAL(ft_val);

    if (JS_IsUndefined(js_val))
        return FT_TYPE_UNDEF;

    if (JS_IsNull(js_val))
        return FT_TYPE_NULL;

    if (JS_IsArray(js_ctx, js_val))
        return FT_TYPE_ARRAY;
    else if (JS_IsNumber(js_val))
        return FT_TYPE_NUMBER;
    else if (JS_IsBool(js_val))
        return FT_TYPE_BOOL;
    else if (JS_IsString(js_val))
        return FT_TYPE_STRING;
    else if (JS_IsObject(js_val)) {
        size_t size;
        if (JS_GetArrayBuffer(js_ctx, &size, js_val))
            return FT_TYPE_BUFFER;
        size_t offset;
        size_t length;
        size_t byte_per_elem;
        JSValue buffer = JS_GetTypedArrayBuffer(js_ctx, js_val, &offset, &length, &byte_per_elem);
        if (!JS_IsException(buffer)) {
            if (JS_GetArrayBuffer(js_ctx, &size, buffer)) {
                JS_FreeValue(js_ctx, buffer);
                return FT_TYPE_TYPED_BUFFER;
            }
        }
        JS_FreeValue(js_ctx, buffer);
        return FT_TYPE_OBJECT;
    }

    return FT_TYPE_NONE;
}

template <typename TCtx, typename TTarget>
struct InitContext {
    template <typename TNative>
    using TransType = FtValTranslator<TNative, TCtx, TTarget>;

    static inline void init(ft_context_ref rt_ctx)
    {
        rt_ctx->ft_from_int = TransType<int32_t>::from;
        rt_ctx->ft_from_uint = TransType<uint32_t>::from;
        rt_ctx->ft_from_int64 = TransType<int64_t>::from;
        rt_ctx->ft_from_uint64 = TransType<uint64_t>::from;
        rt_ctx->ft_from_double = TransType<double>::from;
        rt_ctx->ft_from_bool = TransType<bool>::from;
        rt_ctx->ft_from_string = TransType<const char*>::from;
        // for arrays
        rt_ctx->ft_from_int_array = TransType<int32_t>::fromArray;
        rt_ctx->ft_from_uint_array = TransType<uint32_t>::fromArray;
        rt_ctx->ft_from_int64_array = TransType<int64_t>::fromArray;
        rt_ctx->ft_from_uint64_array = TransType<uint64_t>::fromArray;
        rt_ctx->ft_from_double_array = TransType<double>::fromArray;
        rt_ctx->ft_from_bool_array = TransType<bool>::fromArray;
        rt_ctx->ft_from_string_array = TransType<const char*>::fromArray;
        // for ArrayBuffer and TypedArrayBuffer
        rt_ctx->ft_from_buffer = TransType<int>::fromBuffer;
        rt_ctx->ft_from_typed_buffer = TransType<int>::fromTypedBuffer;

        rt_ctx->ft_to_int = TransType<int32_t>::to;
        rt_ctx->ft_to_uint = TransType<uint32_t>::to;
        rt_ctx->ft_to_int64 = TransType<int64_t>::to;
        rt_ctx->ft_to_uint64 = TransType<uint64_t>::to;
        rt_ctx->ft_to_double = TransType<double>::to;
        rt_ctx->ft_to_bool = TransType<bool>::to;
        rt_ctx->ft_to_string = TransType<int>::toString;
        // for ArrayBuffer and TypedArrayBuffer
        rt_ctx->ft_to_buffer = TransType<int>::toBuffer;
        // array operations
        rt_ctx->ft_array_size = TransType<int>::arraySize;
        rt_ctx->ft_array_at = TransType<int>::arrayGet;
        // object operations
        rt_ctx->ft_new_object = TransType<int>::newObject;
        rt_ctx->ft_obj_get_property = TransType<int>::objectGetProperty;
        rt_ctx->ft_obj_set_property = TransType<int>::objectSetProperty;
        // free value
        rt_ctx->ft_free_value = TransType<int>::freeValue;
        rt_ctx->ft_dup_value = TransType<int>::dupValue;
        rt_ctx->ft_free_string = TransType<int>::freeCString;
        rt_ctx->ft_parse_json = TransType<int>::parseJson;
        rt_ctx->ft_undefined = TransType<int>::undefined;
    }
};

bool InitFeatureContextQjs(ft_context_ref rt_ctx, void* data)
{
    rt_ctx->data = data;
    rt_ctx->ft_get_type = _ft_get_type;

    InitContext<JSContext*, JSValue>::init(rt_ctx);
    return true;
}

void UninitFeatureContextQjs(ft_context_ref context) { }