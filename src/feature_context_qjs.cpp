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

#include <malloc.h>
#include <stdio.h>

#define MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, func, val, ft_type) \
    do {                                                                 \
        JSContext* js_ctx = GET_QJS_CTX(ft_ctx);                         \
        qjs_val_t ret;                                                   \
        ret.js_val = func(js_ctx, val);                                  \
        ret.type = ft_type;                                              \
        return QJS_VAL_TO_FT(ret);                                       \
    } while (false)

#define MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, func, argv, argc) \
    do {                                                               \
        JSContext* js_ctx = GET_QJS_CTX(ft_ctx);                       \
        qjs_val_t ret;                                                 \
        ret.js_val = JS_UNDEFINED;                                     \
        JSValue array = JS_NewArray(js_ctx);                           \
        for (uint32_t i = 0; i < argc; ++i) {                          \
            JSValue elem = func(js_ctx, argv[i]);                      \
            if (!JS_SetPropertyUint32(js_ctx, array, i, elem))         \
                return QJS_VAL_TO_FT(ret);                             \
        }                                                              \
        ret.js_val = array;                                            \
        ret.type = FT_TYPE_ARRAY;                                      \
        return QJS_VAL_TO_FT(ret);                                     \
    } while (false)

ft_type _ft_get_type(ft_context_ref ft_ctx, ft_value_t ft_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JSValue js_val = FT_VAL_GET_JS_VAL(ft_val);

    size_t size;
    if (JS_GetArrayBuffer(js_ctx, &size, js_val))
        return FT_TYPE_ARRAY_BUFFER;

    if (JS_IsArray(js_ctx, js_val))
        return FT_TYPE_ARRAY;
    else if (JS_IsNumber(js_val))
        return FT_TYPE_NUMBER;
    else if (JS_IsBool(js_val))
        return FT_TYPE_NUMBER;
    else if (JS_IsString(js_val))
        return FT_TYPE_STRING;
    else if (JS_IsObject(js_val))
        return FT_TYPE_OBJECT;

    return FT_TYPE_NONE;
}

// value creation
static ft_value_t _ft_int(ft_context_ref ft_ctx, int32_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewInt32, val, FT_TYPE_NUMBER);
}

static ft_value_t _ft_uint(ft_context_ref ft_ctx, uint32_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewUint32, val, FT_TYPE_NUMBER);
}

static ft_value_t _ft_int64(ft_context_ref ft_ctx, int64_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewInt64, val, FT_TYPE_NUMBER);
}

static ft_value_t _ft_uint64(ft_context_ref ft_ctx, uint64_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewBigUint64, val, FT_TYPE_NUMBER);
}

static ft_value_t _ft_double(ft_context_ref ft_ctx, double val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewFloat64, val, FT_TYPE_NUMBER);
}

static ft_value_t _ft_boolean(ft_context_ref ft_ctx, bool val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewBool, val, FT_TYPE_BOOL);
}

static ft_value_t _ft_string(ft_context_ref ft_ctx, const char* val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewString, val, FT_TYPE_STRING);
}

static ft_value_t _ft_buffer(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t ret;
    ret.js_val = JS_NewArrayBufferCopy(js_ctx, buff, size);
    ret.type = FT_TYPE_ARRAY_BUFFER;
    return QJS_VAL_TO_FT(ret);
}

static ft_value_t _ft_int_array(ft_context_ref ft_ctx, int32_t* val, uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewInt32, val, size);
}

static ft_value_t _ft_uint_array(ft_context_ref ft_ctx, uint32_t* val, uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewUint32, val, size);
}

static ft_value_t _ft_int64_array(ft_context_ref ft_ctx, int64_t* val, uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewInt64, val, size);
}

static ft_value_t _ft_uint64_array(ft_context_ref ft_ctx, uint64_t* val, uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewBigUint64, val, size);
}

static ft_value_t _ft_double_array(ft_context_ref ft_ctx, double* val, uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewFloat64, val, size);
}

static ft_value_t _ft_bool_array(ft_context_ref ft_ctx, bool* val, uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewBool, val, size);
}

static ft_value_t _ft_string_array(ft_context_ref ft_ctx, const char** val, uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewString, val, size);
}

static ft_value_t _ft_parse_json(ft_context_ref ft_ctx, const char* buf, size_t buf_len, const char* filename)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t ret;
    ret.js_val = JS_UNDEFINED;
    JSValue obj = JS_ParseJSON(js_ctx, buf, buf_len, filename);
    if (JS_IsException(obj)) {
        JS_FreeValue(js_ctx, obj);
        return QJS_VAL_TO_FT(ret);
    }
    ret.js_val = obj;
    return QJS_VAL_TO_FT(ret);
}

// convert
static const char* _ft_to_string(ft_context_ref ft_ctx, ft_value_t f_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    JSValue val = q_val.js_val;
    if (JS_IsString(val))
        return JS_ToCString(js_ctx, val);

    if (JS_IsObject(val)) {
        JSValue json_obj = JS_JSONStringify(js_ctx, val, JS_UNDEFINED, JS_UNDEFINED);
        const char* obj_str = JS_ToCString(js_ctx, json_obj);
        JS_FreeValue(js_ctx, json_obj);
        return obj_str;
    }

    JSValue js_str = JS_ToString(js_ctx, val);
    const char* ret_str = JS_ToCString(js_ctx, js_str);
    JS_FreeValue(js_ctx, js_str);
    return ret_str;
}

static uint8_t* _ft_to_buffer(ft_context_ref ft_ctx, size_t* p_size, ft_value_t f_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    JSValue val = q_val.js_val;
    uint8_t* ret = JS_GetArrayBuffer(js_ctx, p_size, val);
    return ret;
}

static bool _ft_to_int(ft_context_ref ft_ctx, ft_value_t f_val, int32_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToInt32(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_uint(ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToUint32(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_int64(ft_context_ref ft_ctx, ft_value_t f_val, int64_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToInt64(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_uint64(ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToIndex(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_double(ft_context_ref ft_ctx, ft_value_t f_val, double* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToFloat64(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_bool(ft_context_ref ft_ctx, ft_value_t f_val, bool* b)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToBool(js_ctx, q_val.js_val);
    if (ret >= 0) {
        *b = !!ret;
        return true;
    }
    return false;
}

static uint32_t _ft_array_size(ft_context_ref ft_ctx, const ft_value_t f_obj)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_obj = FT_VAL_TO_QJS(f_obj);
    if (!JS_IsArray(js_ctx, q_obj.js_val))
        return 0;
    JSValue prop = JS_GetPropertyStr(js_ctx, q_obj.js_val, "length");
    if (JS_IsException(prop)) {
        JS_FreeValue(js_ctx, prop);
        return 0;
    }

    uint32_t value = 0;
    int r = JS_ToUint32(js_ctx, &value, prop);
    JS_FreeValue(js_ctx, prop);
    if (r == 0)
        return value;

    return 0;
}

static ft_value_t _ft_array_at(ft_context_ref ft_ctx, const ft_value_t f_obj, uint32_t idx)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_obj = FT_VAL_TO_QJS(f_obj);
    qjs_val_t ret;
    ret.js_val = JS_UNDEFINED;
    if (!JS_IsArray(js_ctx, q_obj.js_val))
        return QJS_VAL_TO_FT(ret);

    JSValue prop = JS_GetPropertyUint32(js_ctx, q_obj.js_val, idx);
    if (JS_IsException(prop)) {
        JS_FreeValue(js_ctx, prop);
        return QJS_VAL_TO_FT(ret);
    }
    ret.js_val = prop;
    return QJS_VAL_TO_FT(ret);
}

// object operations
static ft_value_t _ft_obj_get_property(ft_context_ref ft_ctx, ft_value_t f_obj, const char* key)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_obj = FT_VAL_TO_QJS(f_obj);
    qjs_val_t ret;
    ret.js_val = JS_UNDEFINED;
    if (!JS_IsObject(q_obj.js_val))
        return QJS_VAL_TO_FT(ret);

    JSValue prop = JS_GetPropertyStr(js_ctx, q_obj.js_val, key);
    ret.js_val = prop;
    return QJS_VAL_TO_FT(ret);
}

static bool _ft_obj_set_property(ft_context_ref ft_ctx, ft_value_t f_obj, const char* prop, ft_value_t f_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_obj = FT_VAL_TO_QJS(f_obj);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    if (!JS_IsObject(q_obj.js_val))
        return false;

    int ret = JS_SetPropertyStr(js_ctx, q_obj.js_val, prop, q_val.js_val);
    return ret > 0;
}

// free value
static void _ft_free_value(ft_context_ref ft_ctx, ft_value_t f_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    JS_FreeValue(js_ctx, q_val.js_val);
}

static void _ft_free_string(ft_context_ref ft_ctx, const char* str)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JS_FreeCString(js_ctx, str);
}

bool InitFeatureContextQjs(ft_context_ref rt_ctx, void* data)
{
    rt_ctx->data = data;
    rt_ctx->ft_get_type = _ft_get_type;
    // value creation
    rt_ctx->ft_from_int = _ft_int;
    rt_ctx->ft_from_uint = _ft_uint;
    rt_ctx->ft_from_int64 = _ft_int64;
    rt_ctx->ft_from_uint64 = _ft_uint64;
    rt_ctx->ft_from_double = _ft_double;
    rt_ctx->ft_from_bool = _ft_boolean;
    rt_ctx->ft_from_string = _ft_string;
    rt_ctx->ft_from_buffer = _ft_buffer;

    rt_ctx->ft_from_int_array = _ft_int_array;
    rt_ctx->ft_from_uint_array = _ft_uint_array;
    rt_ctx->ft_from_int64_array = _ft_int64_array;
    rt_ctx->ft_from_uint64_array = _ft_uint64_array;
    rt_ctx->ft_from_double_array = _ft_double_array;
    rt_ctx->ft_from_bool_array = _ft_bool_array;
    rt_ctx->ft_from_string_array = _ft_string_array;
    rt_ctx->ft_parse_json = _ft_parse_json;

    // convert
    rt_ctx->ft_to_int = _ft_to_int;
    rt_ctx->ft_to_uint = _ft_to_uint;
    rt_ctx->ft_to_int64 = _ft_to_int64;
    rt_ctx->ft_to_uint64 = _ft_to_uint64;
    rt_ctx->ft_to_double = _ft_to_double;
    rt_ctx->ft_to_bool = _ft_to_bool;
    rt_ctx->ft_to_string = _ft_to_string;
    rt_ctx->ft_to_buffer = _ft_to_buffer;
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

void UninitFeatureContextQjs(ft_context_ref context)
{
}
