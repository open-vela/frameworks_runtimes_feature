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
// clang-format off
#include "value_translator_qjs.h"
#include "feature_value_translator.h"
// clang-format on

#include <malloc.h>
#include <stdio.h>

#define MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, func, val) \
    do {                                                        \
        JSContext* js_ctx = GET_QJS_CTX(ft_ctx);                \
        qjs_val_t ret;                                          \
        ret.js_val = func(js_ctx, val);                         \
        return QJS_VAL_TO_FT(ret);                              \
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
        return QJS_VAL_TO_FT(ret);                                     \
    } while (false)

ft_type _ft_get_type(ft_context_ref ft_ctx, ft_value_t ft_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JSValue js_val = FT_VAL_GET_JS_VAL(ft_val);

    if (JS_IsUndefined(js_val))
        return FT_TYPE_UNDEF;

    if (JS_IsNull(js_val))
        return FT_TYPE_NULL;

    size_t size;
    if (JS_GetArrayBuffer(js_ctx, &size, js_val))
        return FT_TYPE_BUFFER;

    size_t offset;
    size_t length;
    size_t byte_per_elem;
    JSValue buffer = JS_GetTypedArrayBuffer(js_ctx, js_val, &offset, &length, &byte_per_elem);
    if (!JS_IsException(buffer) && JS_GetArrayBuffer(js_ctx, &size, buffer))
        return FT_TYPE_TYPED_BUFFER;

    if (JS_IsArray(js_ctx, js_val))
        return FT_TYPE_ARRAY;
    else if (JS_IsNumber(js_val))
        return FT_TYPE_NUMBER;
    else if (JS_IsBool(js_val))
        return FT_TYPE_BOOL;
    else if (JS_IsString(js_val))
        return FT_TYPE_STRING;
    else if (JS_IsObject(js_val))
        return FT_TYPE_OBJECT;

    return FT_TYPE_NONE;
}

// value creation
static ft_value_t _ft_int(ft_context_ref ft_ctx, int32_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewInt32, val);
}

static ft_value_t _ft_uint(ft_context_ref ft_ctx, uint32_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewUint32, val);
}

static ft_value_t _ft_int64(ft_context_ref ft_ctx, int64_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewInt64, val);
}

static ft_value_t _ft_uint64(ft_context_ref ft_ctx, uint64_t val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewBigUint64, val);
}

static ft_value_t _ft_double(ft_context_ref ft_ctx, double val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewFloat64, val);
}

static ft_value_t _ft_boolean(ft_context_ref ft_ctx, bool val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewBool, val);
}

static ft_value_t _ft_string(ft_context_ref ft_ctx, const char* val)
{
    MAKE_JS_VALUE_WITH_NEW_FUNC_AND_TYPE(ft_ctx, JS_NewString, val);
}

static ft_value_t _ft_buffer(ft_context_ref ft_ctx, uint8_t* buff,
    uint32_t size)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t ret;
    ret.js_val = JS_NewArrayBufferCopy(js_ctx, buff, size);
    return QJS_VAL_TO_FT(ret);
}

static ft_value_t _ft_typed_buffer(ft_context_ref ft_ctx, uint8_t* buff,
    uint32_t size, uint32_t type)
{
    static const char* type_names[] = {
        "Int8Array",
        "Uint8Array",
        "Int16Array",
        "Uint16Array",
        "Int32Array",
        "Uint32Array",
        "Float32Array",
        "Float64Array",
    };

    qjs_val_t ret;
    ret.js_val = JS_UNDEFINED;
    if (type >= (sizeof(type_names) / sizeof(type_names[0]))) {
        return QJS_VAL_TO_FT(ret);
    }

    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JSValue array_buffer = JS_NewArrayBufferCopy(js_ctx, buff, size);
    JSValueConst global = JS_GetGlobalObject(js_ctx);
    JSValueConst uint8array_ctr = JS_GetPropertyStr(js_ctx, global, type_names[type]);
    JSValue args[1] = { array_buffer };
    ret.js_val = JS_CallConstructor(js_ctx, uint8array_ctr, 1, args);
    JS_FreeValue(js_ctx, array_buffer);
    return QJS_VAL_TO_FT(ret);
}

static ft_value_t _ft_int_array(ft_context_ref ft_ctx, int32_t* val,
    uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewInt32, val, size);
}

static ft_value_t _ft_uint_array(ft_context_ref ft_ctx, uint32_t* val,
    uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewUint32, val, size);
}

static ft_value_t _ft_int64_array(ft_context_ref ft_ctx, int64_t* val,
    uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewInt64, val, size);
}

static ft_value_t _ft_uint64_array(ft_context_ref ft_ctx, uint64_t* val,
    uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewBigUint64, val, size);
}

static ft_value_t _ft_double_array(ft_context_ref ft_ctx, double* val,
    uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewFloat64, val, size);
}

static ft_value_t _ft_bool_array(ft_context_ref ft_ctx, bool* val,
    uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewBool, val, size);
}

static ft_value_t _ft_string_array(ft_context_ref ft_ctx, const char** val,
    uint32_t size)
{
    MAKE_JS_ARRAY_WITH_NEW_FUNC_AND_ARGS(ft_ctx, JS_NewString, val, size);
}

static ft_value_t _ft_parse_json(ft_context_ref ft_ctx, const char* buf,
    size_t buf_len, const char* filename)
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

static uint8_t* _ft_to_buffer(ft_context_ref ft_ctx, size_t* p_size,
    ft_value_t f_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JSValue val = FT_VAL_GET_JS_VAL(f_val);

    size_t offset;
    size_t length;
    size_t byte_per_elem;
    // first get buffer ptr from a typedArray
    JSValue array_buffer = JS_GetTypedArrayBuffer(js_ctx, val, &offset, &length, &byte_per_elem);
    if (!JS_IsException(array_buffer)) {
        uint8_t* ret = JS_GetArrayBuffer(js_ctx, p_size, array_buffer);
        JS_FreeValue(js_ctx, array_buffer);
        return ret;
    }

    // get buffer ptr from an arraybuffer
    return JS_GetArrayBuffer(js_ctx, p_size, val);
}

static bool _ft_to_int(ft_context_ref ft_ctx, ft_value_t f_val, int32_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToInt32(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_uint(ft_context_ref ft_ctx, ft_value_t f_val,
    uint32_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToUint32(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_int64(ft_context_ref ft_ctx, ft_value_t f_val,
    int64_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToInt64(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_uint64(ft_context_ref ft_ctx, ft_value_t f_val,
    uint64_t* pres)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t q_val = FT_VAL_TO_QJS(f_val);
    int ret = JS_ToIndex(js_ctx, pres, q_val.js_val);
    return ret == 0;
}

static bool _ft_to_double(ft_context_ref ft_ctx, ft_value_t f_val,
    double* pres)
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

static ft_value_t _ft_array_at(ft_context_ref ft_ctx, const ft_value_t f_obj,
    uint32_t idx)
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
ft_value_t _ft_new_object(ft_context_ref ft_ctx)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    qjs_val_t ret;
    ret.js_val = JS_NewObject(js_ctx);
    return QJS_VAL_TO_FT(ret);
}

static ft_value_t _ft_obj_get_property(ft_context_ref ft_ctx, ft_value_t f_obj,
    const char* key)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JSValue js_obj = FT_VAL_GET_JS_VAL(f_obj);
    qjs_val_t ret;
    ret.js_val = JS_UNDEFINED;
    if (!JS_IsObject(js_obj))
        return QJS_VAL_TO_FT(ret);

    JSValue prop = JS_GetPropertyStr(js_ctx, js_obj, key);
    ret.js_val = prop;
    return QJS_VAL_TO_FT(ret);
}

static bool _ft_obj_set_property(ft_context_ref ft_ctx, ft_value_t f_obj,
    const char* prop, ft_value_t f_val)
{
    JSContext* js_ctx = GET_QJS_CTX(ft_ctx);
    JSValue js_obj = FT_VAL_GET_JS_VAL(f_obj);
    JSValue js_val = FT_VAL_GET_JS_VAL(f_val);
    if (!JS_IsObject(js_obj))
        return false;

    int ret = JS_SetPropertyStr(js_ctx, js_obj, prop, js_val);
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

static ft_value_t _ft_undefined(ft_context_ref ft_ctx)
{
    qjs_val_t ret;
    ret.js_val = JS_UNDEFINED;
    return QJS_VAL_TO_FT(ret);
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
        rt_ctx->ft_free_string = TransType<int>::freeCString;
        rt_ctx->ft_parse_json = TransType<int>::parseJson;
    }
};

bool InitFeatureContextQjs(ft_context_ref rt_ctx, void* data)
{
    rt_ctx->data = data;
    rt_ctx->ft_get_type = _ft_get_type;

    // InitContext<JSContext*, JSValue>::init(rt_ctx);

    // value creation
    rt_ctx->ft_from_int = _ft_int;
    rt_ctx->ft_from_uint = _ft_uint;
    rt_ctx->ft_from_int64 = _ft_int64;
    rt_ctx->ft_from_uint64 = _ft_uint64;
    rt_ctx->ft_from_double = _ft_double;
    rt_ctx->ft_from_bool = _ft_boolean;
    rt_ctx->ft_from_string = _ft_string;
    rt_ctx->ft_from_buffer = _ft_buffer;
    rt_ctx->ft_from_typed_buffer = _ft_typed_buffer;

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
    rt_ctx->ft_new_object = _ft_new_object;
    rt_ctx->ft_obj_get_property = _ft_obj_get_property;
    rt_ctx->ft_obj_set_property = _ft_obj_set_property;
    // free value
    rt_ctx->ft_free_value = _ft_free_value;
    rt_ctx->ft_free_string = _ft_free_string;
    rt_ctx->ft_undefined = _ft_undefined;
    return true;
}

void UninitFeatureContextQjs(ft_context_ref context) { }
