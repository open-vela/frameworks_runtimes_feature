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
#ifndef __FEATURE_H__
#define __FEATURE_H__

// use quickjs as default
#ifndef BACKEND_ENGINE_TYPE
#define BACKEND_ENGINE_TYPE 0
#endif
#if BACKEND_ENGINE_TYPE == 0

#include "feature_log.h"
#include "quickjs/quickjs.h"

typedef void* context_ref;

typedef JSValue feature_value_t;
typedef JSValue* feature_value_ref;
typedef JSContext* feature_context_ref;
typedef JSRuntime* feature_runtime_ref;
typedef JSClassDef feature_classdef_t;
typedef JSClassID feature_classid_t;
typedef JSAtom feature_atom_t;
typedef JS_MarkFunc feature_mark_func;

#define FEATURE_VALUE_UNDEFINED JS_UNDEFINED
#define FEATURE_EXCEPTION JS_EXCEPTION
#define FEATURE_UNDEFINED JS_UNDEFINED
#define FEATURE_PROP_ENUMERABLE JS_PROP_ENUMERABLE
#define FEATURE_PROP_CONFIGURABLE JS_PROP_CONFIGURABLE

#define feature_mark_value(rt, val, mark_func) JS_MarkValue(static_cast<JSRuntime*>(rt), val, mark_func)

// value creation
#define feature_int(ctx, v) JS_NewInt32(static_cast<JSContext*>(ctx), v)
#define feature_uint(ctx, v) JS_NewUint32(static_cast<JSContext*>(ctx), v)
#define feature_string(ctx, str) JS_NewString(static_cast<JSContext*>(ctx), str)
#define feature_double(ctx, v) JS_NewFloat64(static_cast<JSContext*>(ctx), v)
#define feature_int64(ctx, v) JS_NewInt64(static_cast<JSContext*>(ctx), v)
#define feature_uint64(ctx, v) JS_NewBigUint64(static_cast<JSContext*>(ctx), v)
#define feature_boolean(ctx, v) JS_NewBool(static_cast<JSContext*>(ctx), v ? 1 : 0)

// convert
#define feature_to_cstring(ctx, val) JS_ToCString(static_cast<JSContext*>(ctx), val)
#define feature_to_string(ctx, val) JS_ToString(static_cast<JSContext*>(ctx), val)

#define feature_global_object(ctx) JS_GetGlobalObject(static_cast<JSContext*>(ctx))
#define feature_object(ctx) JS_NewObject(static_cast<JSContext*>(ctx))
#define feature_eval(ctx, input, input_len, filename, eval_flags) JS_Eval(static_cast<JSContext*>(ctx), input, input_len, filename, eval_flags)
#define feature_cfunction(ctx, func, name, length) JS_NewCFunction(static_cast<JSContext*>(ctx), func, name, length)
#define feature_cfunctiondata(ctx, func, len, mgc, dlen, data) JS_NewCFunctionData(static_cast<JSContext*>(ctx), func, len, mgc, dlen, data)
static inline bool feature_to_int(context_ref ctx, int32_t* pres, feature_value_t val)
{
    int r = JS_ToInt32(static_cast<JSContext*>(ctx), pres, val);
    return r == 0;
}

static inline bool feature_to_uint(context_ref ctx, uint32_t* pres, feature_value_t val)
{
    int r = JS_ToUint32(static_cast<JSContext*>(ctx), pres, val);
    return r == 0;
}

static inline bool feature_to_int64(context_ref ctx, int64_t* pres, feature_value_t val)
{
    int r = JS_ToInt64(static_cast<JSContext*>(ctx), pres, val);
    return r == 0;
}

static inline bool feature_to_uint64(context_ref ctx, uint64_t* pres, feature_value_t val)
{
    int r = JS_ToIndex(static_cast<JSContext*>(ctx), pres, val);
    return r == 0;
}

static inline bool feature_to_double(context_ref ctx, double* pres, feature_value_t val)
{
    int r = JS_ToFloat64(static_cast<JSContext*>(ctx), pres, val);
    return r == 0;
}

static inline bool feature_to_boolean(context_ref ctx, bool* pres, feature_value_t val)
{
    int r = JS_ToBool(static_cast<JSContext*>(ctx), val);
    if (r >= 0) {
        *pres = !!r;
        return true;
    }
    return false;
}

static inline uint8_t* feature_to_arraybuffer(context_ref ctx, size_t* psize, feature_value_t obj)
{
    return JS_GetArrayBuffer(static_cast<JSContext*>(ctx), psize, obj);
}

// free value
#define feature_free_value(ctx, value) JS_FreeValue(static_cast<JSContext*>(ctx), value)
#define feature_free_cstring(ctx, val) JS_FreeCString(static_cast<JSContext*>(ctx), val)

static inline bool feature_is_object(feature_value_t val)
{
    return !!(JS_IsObject(val));
}

static inline bool feature_is_number(feature_value_t val)
{
    return !!(JS_IsNumber(val));
}

static inline bool feature_is_string(feature_value_t val)
{
    return !!(JS_IsString(val));
}

static inline bool feature_is_undefined(feature_value_t val)
{
    return !!(JS_IsUndefined(val));
}

static inline bool feature_is_null(feature_value_t val)
{
    return !!(JS_IsNull(val));
}

static inline bool feature_is_boolean(feature_value_t val)
{
    return !!(JS_IsBool(val));
}

static inline bool feature_is_array(context_ref ctx, feature_value_t val)
{
    return !!(JS_IsArray(static_cast<JSContext*>(ctx), val));
}

static inline bool feature_is_exception(feature_value_t val)
{
    return !!(JS_IsException(val));
}

static inline bool feature_is_error(context_ref ctx, feature_value_t val)
{
    return !!(JS_IsError(static_cast<JSContext*>(ctx), val));
}

#define feature_promise_capability(ctx, val) JS_NewPromiseCapability(static_cast<JSContext*>(ctx), (feature_value_ref)val)

#define feature_array(ctx) JS_NewArray(static_cast<JSContext*>(ctx))

#define feature_atom(ctx, str) JS_NewAtom(static_cast<JSContext*>(ctx), str)

#define feature_free_atom(ctx, atom) JS_FreeAtom(static_cast<JSContext*>(ctx), atom)

static inline uint32_t feature_get_array_length(context_ref ctx, feature_value_t val)
{
    uint32_t value = 0;
    JSValue length = JS_GetPropertyStr(static_cast<JSContext*>(ctx), val, "length");
    if (!!(JS_IsException((JSValueConst)length))) {
        JS_FreeValue(static_cast<JSContext*>(ctx), length);
        return -1;
    }

    JS_ToUint32(static_cast<JSContext*>(ctx), &value, length);
    JS_FreeValue(static_cast<JSContext*>(ctx), length);

    return value;
}

static inline feature_value_t feature_get_array_idx_safe(context_ref ctx, feature_value_t val, uint32_t idx)
{
    JSValue value = JS_GetPropertyUint32(static_cast<JSContext*>(ctx), val, idx);
    if (!!JS_IsException(value)) {
        JS_FreeValue(static_cast<JSContext*>(ctx), value);
        value = JS_UNDEFINED;
    }
    return value;
}

static inline bool feature_set_array_idx(context_ref ctx, const feature_value_t obj, int32_t idx, feature_value_t val)
{
    if (!feature_is_array(ctx, obj)) {
        return false;
    }
    return JS_SetPropertyUint32(static_cast<JSContext*>(ctx), obj, (uint32_t)idx, val) == 1;
}

static inline bool feature_is_same_value(context_ref ctx, feature_value_t first, feature_value_t second)
{
    JS_BOOL ret = JS_IsSameValue(static_cast<JSContext*>(ctx), first, second);
    return ret == 1;
}

#define feature_stringify(ctx, this_obj) JS_JSONStringify(static_cast<JSContext*>(ctx), this_obj, JS_UNDEFINED, JS_UNDEFINED)

#define feature_dup_value(ctx, val) JS_DupValue(static_cast<JSContext*>(ctx), val)

#define feature_object(ctx) JS_NewObject(static_cast<JSContext*>(ctx))

// object operations
#define feature_get_object_property(ctx, this_obj, prop) JS_GetPropertyStr(static_cast<JSContext*>(ctx), this_obj, prop)
#define feature_set_object_property(ctx, this_obj, prop, val) JS_SetPropertyStr(static_cast<JSContext*>(ctx), this_obj, prop, val)
#define feature_define_object_property(ctx, obj, prop, val, flags) JS_DefinePropertyValueStr(static_cast<JSContext*>(ctx), obj, prop, val, flags)

#define feature_set_opaque(obj, opaque) JS_SetOpaque(obj, opaque)
#define feature_get_opaque(obj, class_id) JS_GetOpaque(obj, class_id)

#define feature_call(ctx, func_obj, this_obj, argc, argv) JS_Call(static_cast<JSContext*>(ctx), func_obj, this_obj, argc, argv)

#define feature_get_runtime(ctx) JS_GetRuntime(static_cast<JSContext*>(ctx))
/**
 * 打印对象
 */
static inline void feature_dump_obj(feature_context_ref ctx, feature_value_t val)
{
    const char* str = feature_to_cstring(ctx, val);
    if (str) {
        FEATURE_LOG_ERROR("%s", str);
        feature_free_cstring(ctx, str);
    } else {
        FEATURE_LOG_ERROR("[exception]");
    }
}

/**
 * 打印错误信息
 */
static inline void feature_dump_error1(feature_context_ref ctx, feature_value_t exception_val)
{
    bool is_error = feature_is_error(ctx, exception_val);
    feature_dump_obj(ctx, exception_val);
    if (is_error) {
        // 如果是Error，则打印栈信息
        feature_value_t val = feature_get_object_property(ctx, exception_val, "stack");
        if (!feature_is_undefined(val)) {
            feature_dump_obj(ctx, val);
        }
        feature_free_value(ctx, val);
    }
}

static inline void feature_dump_error(feature_context_ref ctx)
{
    feature_value_t exception_val = JS_GetException(ctx);

    feature_dump_error1(ctx, exception_val);
    feature_free_value(ctx, exception_val);
}

#define FEATURE_THROW_INTERNAL_ERROR(ctx, ...) JS_ThrowInternalError(static_cast<JSContext*>(ctx), __VA_ARGS__);

#elif BACKEND_ENGINE_TYPE == 1

#else
#error "unsupported backend engine !"
#endif

#endif