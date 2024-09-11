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

#include "value_translator_qjs.h"
#include "feature_instance_qjs.h"
#include "feature_manager_qjs.h"

#define MAKE_JS_ARRAY(ctx, func, argv, argc, ptarget)         \
    do {                                                      \
        JSValue array = JS_NewArray(ctx);                     \
        for (uint32_t i = 0; i < argc; ++i) {                 \
            JSValue elem = func(ctx, argv[i]);                \
            if (!JS_SetPropertyUint32(ctx, array, i, elem)) { \
                *ptarget = JS_UNDEFINED;                      \
                break;                                        \
            }                                                 \
        }                                                     \
        *ptarget = array;                                     \
    } while (false)

namespace value_translator {

// to native values
bool toNative(JSContext* ctx, const JSValue& target, bool* pnative)
{
    int ret = JS_ToBool(ctx, target);
    if (ret >= 0) {
        *pnative = !!ret;
        return true;
    }
    return false;
}

bool toNative(JSContext* ctx, const JSValue& target, char** pnative)
{
    if (JS_IsString(target)) {
        *((const char**)pnative) = JS_ToCString(ctx, target);
        return true;
    }

    if (JS_IsObject(target)) {
        JSValue json_str = JS_JSONStringify(ctx, target, JS_UNDEFINED, JS_UNDEFINED);
        *((const char**)pnative) = JS_ToCString(ctx, json_str);
        JS_FreeValue(ctx, json_str);
        return true;
    }

    JSValue js_str = JS_ToString(ctx, target);
    *((const char**)pnative) = JS_ToCString(ctx, js_str);
    JS_FreeValue(ctx, js_str);
    return true;
}

bool toNative(JSContext* ctx, const JSValue& target, ft_value_t* pnative)
{
    qjs_val_t* q_val = (qjs_val_t*)pnative;
    if (JS_IsNull(target) || JS_IsUndefined(target)) {
        FEATURE_LOG_ERROR("object is null or undefined!");
        q_val->js_val = JS_UNDEFINED;
        return false;
    }

    q_val->js_val = target;
    return true;
}

// ArrayBuffer or TypedArrayBuffer
bool toNativeBuffer(JSContext* ctx, const JSValue& target, uint8_t** pnative, size_t* psize)
{
    size_t offset;
    size_t length;
    size_t byte_per_elem;
    // first get buffer ptr from a typedArray
    JSValue array_buffer = JS_GetTypedArrayBuffer(ctx, target, &offset, &length, &byte_per_elem);
    if (!JS_IsException(array_buffer)) {
        uint8_t* ret = JS_GetArrayBuffer(ctx, psize, array_buffer);
        JS_FreeValue(ctx, array_buffer);
        *pnative = ret;
        return true;
    }
    JS_FreeValue(ctx, array_buffer);
    *pnative = JS_GetArrayBuffer(ctx, psize, target);
    return true;
}

// for TypedArrayBuffer
bool toTargetTypedBuffer(JSContext* ctx, uint8_t* buff, uint32_t size, uint32_t type, JSValue* ptarget)
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

    if (type >= (sizeof(type_names) / sizeof(type_names[0]))) {
        *ptarget = JS_UNDEFINED;
        return false;
    }

    JSValue array_buffer = JS_NewArrayBufferCopy(ctx, buff, size);
    JSValueConst global = JS_GetGlobalObject(ctx);
    JSValueConst typed_array_ctr = JS_GetPropertyStr(ctx, global, type_names[type]);
    JSValue ret = JS_CallConstructor(ctx, typed_array_ctr, 1, &array_buffer);
    JS_FreeValue(ctx, array_buffer);
    JS_FreeValue(ctx, typed_array_ctr);
    JS_FreeValue(ctx, global);
    *ptarget = ret;
    return true;
}

// for arrays
bool toTargetArray(JSContext* ctx, int32_t* val, uint32_t size, JSValue* ptarget)
{
    MAKE_JS_ARRAY(ctx, JS_NewInt32, val, size, ptarget);
    return !JS_IsUndefined(*ptarget);
}

bool toTargetArray(JSContext* ctx, uint32_t* val, uint32_t size, JSValue* ptarget)
{
    MAKE_JS_ARRAY(ctx, JS_NewUint32, val, size, ptarget);
    return !JS_IsUndefined(*ptarget);
}

bool toTargetArray(JSContext* ctx, int64_t* val, uint32_t size, JSValue* ptarget)
{
    MAKE_JS_ARRAY(ctx, JS_NewInt64, val, size, ptarget);
    return !JS_IsUndefined(*ptarget);
}

bool toTargetArray(JSContext* ctx, uint64_t* val, uint32_t size, JSValue* ptarget)
{
    MAKE_JS_ARRAY(ctx, JS_NewBigUint64, val, size, ptarget);
    return !JS_IsUndefined(*ptarget);
}

bool toTargetArray(JSContext* ctx, double* val, uint32_t size, JSValue* ptarget)
{
    MAKE_JS_ARRAY(ctx, JS_NewFloat64, val, size, ptarget);
    return !JS_IsUndefined(*ptarget);
}

bool toTargetArray(JSContext* ctx, bool* val, uint32_t size, JSValue* ptarget)
{
    MAKE_JS_ARRAY(ctx, JS_NewBool, val, size, ptarget);
    return !JS_IsUndefined(*ptarget);
}

bool toTargetArray(JSContext* ctx, const char** val, uint32_t size, JSValue* ptarget)
{
    MAKE_JS_ARRAY(ctx, JS_NewString, val, size, ptarget);
    return !JS_IsUndefined(*ptarget);
}

// funcitons for handling objcects
bool getObjectField(JSContext* ctx, const JSValue& obj, const char* name, int idx, JSValue* pfield)
{
    *pfield = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsUndefined(*pfield))
        return false;

    return true;
}

bool setObjectField(JSContext* ctx, const JSValue& obj, const char* name, JSValue field)
{
    if (!JS_IsObject(obj))
        return false;

    int ret = JS_SetPropertyStr(ctx, obj, name, field);
    return ret > 0;
}

// funcitons for handling arrays
uint32_t arraySize(JSContext* ctx, const JSValue& array)
{
    uint32_t ret = 0;
    if (!JS_IsArray(ctx, array))
        return ret;

    JSValue prop = JS_GetPropertyStr(ctx, array, "length");
    if (!JS_IsException(prop)) {
        if (JS_ToUint32(ctx, &ret, prop) != 0)
            ret = 0;
    }

    JS_FreeValue(ctx, prop);
    return ret;
}

JSValue arrayGet(JSContext* ctx, const JSValue& array, uint32_t idx)
{
    JSValue ret = JS_UNDEFINED;
    if (!JS_IsArray(ctx, array))
        return ret;

    return JS_GetPropertyUint32(ctx, array, idx);
}

bool arraySet(JSContext* ctx, const JSValue& array, int32_t idx, JSValue val)
{
    if (!feature_is_array(ctx, array))
        return false;

    return JS_SetPropertyUint32(ctx, array, (uint32_t)idx, val) == 1;
}

JSValue parseJson(JSContext* ctx, const char* buf, size_t buf_len, const char* file_name)
{
    JSValue obj = JS_ParseJSON(ctx, buf, buf_len, file_name);
    if (JS_IsException(obj)) {
        JS_FreeValue(ctx, obj);
        return JS_UNDEFINED;
    }
    return obj;
}

void* interfaceFromTarget(JSValue& target)
{
    auto opaque = JS_GetOpaque(target, ferry::FeatureManagerQjs::jsClassId());
    FEATURE_LOG_DEBUG("value: %p, get opaque: %p", JS_VALUE_GET_PTR(target), opaque);
    FEATURE_CHECK_NE(opaque, nullptr);
    return opaque;
}

JSValue targetFromInterface(void* instance)
{
    return ((ferry::FeatureInstanceQjs*)instance)->dupTarget();
}

}
