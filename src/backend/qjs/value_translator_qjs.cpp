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
#include "array_buffer_qjs.h"
#include "feature_exports.h"
#include "feature_instance_qjs.h"
#include "feature_manager_qjs.h"

using namespace feature_framework;

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
    if (JS_IsBool(target)) {
        int ret = JS_ToBool(ctx, target);
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

bool toNative(JSContext* ctx, const JSValue& target, FtJsonObject* pnative)
{
    if (!JS_IsObject(target) || !pnative) {
        return false;
    }

    JSValue json = JS_JSONStringify(ctx, target, JS_UNDEFINED, JS_UNDEFINED);
    const char* cstr = JS_ToCString(ctx, json);
    FtJsonObject json_obj = FeatureNewJsonObject(cstr);
    JS_FreeCString(ctx, cstr);
    JS_FreeValue(ctx, json);
    *pnative = json_obj;
    return true;
}

bool toTarget(JSContext* ctx, FtArrayBuffer native, JSValue* ptarget)
{
    if (!native) {
        return false;
    }

    ArrayBufferQjs* array_buffer = (ArrayBufferQjs*)native;
    if (!array_buffer) {
        FEATURE_LOG_ERROR("invalid array buffer");
        return false;
    }

    *ptarget = array_buffer->getTarget();
    JS_DupValue(ctx, *ptarget);
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
bool toTargetTypedBuffer(JSContext* ctx, uint8_t* buff, uint32_t size, FtTypedArrayType type, JSValue* ptarget)
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

bool toTargetJson(JSContext* ctx, FtJsonObject native, JSValue* ptarget)
{
    if (!native) {
        *ptarget = JS_NULL;
        return true;
    }
    JSValue json = JS_ParseJSON(ctx, native->str, strlen(native->str), NULL);
    if (JS_IsException(json)) {
        JS_FreeValue(ctx, json);
        return false;
    }
    *ptarget = json;
    return true;
}

// funcitons for handling objects
bool getObjectField(JSContext* ctx, const JSValue& obj, const char* name, JSValue* pfield)
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

    ret = JS_GetPropertyUint32(ctx, array, idx);
    if (JS_IsException(ret)) {
        JS_FreeValue(ctx, ret);
        return JS_UNDEFINED;
    }
    return ret;
}

bool arraySet(JSContext* ctx, const JSValue& array, int32_t idx, JSValue val)
{
    if (!feature_is_array(ctx, array))
        return false;

    return JS_SetPropertyUint32(ctx, array, (uint32_t)idx, val) == 1;
}

bool isArrayBuffer(JSContext* ctx, const JSValue& target)
{
    size_t size = 0;
    if (JS_GetArrayBuffer(ctx, &size, target)) {
        return true;
    }
    return false;
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

void* interfaceFromTarget(JSContext* ctx, JSValue& target)
{
    auto opaque = JS_GetOpaque(target, FeatureManagerQjs::jsClassId());
    FEATURE_LOG_DEBUG("value: %p, get opaque: %p", JS_VALUE_GET_PTR(target), opaque);
    FEATURE_CHECK_NE(opaque, nullptr);
    return opaque;
}

JSValue targetFromInterface(JSContext* ctx, void* instance)
{
    return ((FeatureInstanceQjs*)instance)->dupTarget();
}

int checkAsyncCallbacks(JSContext* ctx, JSValue arg)
{
    if (JS_IsUndefined(arg) || !JS_IsObject(arg)) {
        return 0;
    }

    int result = 0;

    JSValue success_cb = JS_GetPropertyStr(ctx, arg, "success");
    JSValue fail_cb = JS_GetPropertyStr(ctx, arg, "fail");
    JSValue complete_cb = JS_GetPropertyStr(ctx, arg, "complete");

    bool success_valid = JS_IsFunction(ctx, success_cb);
    bool fail_valid = JS_IsFunction(ctx, fail_cb);
    bool complete_valid = JS_IsFunction(ctx, complete_cb);

    if ((!JS_IsUndefined(success_cb) && !JS_IsNull(success_cb) && !success_valid)
        || (!JS_IsUndefined(fail_cb) && !JS_IsNull(fail_cb) && !fail_valid)
        || (!JS_IsUndefined(complete_cb) && !JS_IsNull(complete_cb) && !complete_valid)) {
        FEATURE_LOG_ERROR("invalid callbacks!");
        result = -1;
        goto cleanup;
    }

    if (success_valid || fail_valid || complete_valid) {
        result = 1;
    }

cleanup:
    feature_free_value(ctx, success_cb);
    feature_free_value(ctx, fail_cb);
    feature_free_value(ctx, complete_cb);
    return result;
}

int addAsyncCallbacks(JSContext* ctx, void* instance, FeatureType ftype, JSValue arg)
{
    if (JS_IsUndefined(arg) || !JS_IsObject(arg)) {
        FEATURE_LOG_ERROR("arg is undefined or not object!");
        return -1;
    }
    JSValue success_cb = JS_GetPropertyStr(ctx, arg, "success");
    JSValue fail_cb = JS_GetPropertyStr(ctx, arg, "fail");
    JSValue complete_cb = JS_GetPropertyStr(ctx, arg, "complete");
    JS_SetPropertyStr(ctx, arg, "success", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, arg, "fail", JS_UNDEFINED);
    JS_SetPropertyStr(ctx, arg, "complete", JS_UNDEFINED);
    if (!JS_IsFunction(ctx, success_cb) && !JS_IsFunction(ctx, fail_cb)) {
        FEATURE_LOG_WARN("cannot find callbacks from last arg!");
        feature_free_value(ctx, success_cb);
        feature_free_value(ctx, fail_cb);
        feature_free_value(ctx, complete_cb);
        return -1;
    }
    return ((FeatureInstanceQjs*)instance)->addAsyncCallbacks(ftype, success_cb, fail_cb, complete_cb);
}

// funcitons for handling structs
bool getStructField(JSContext* ctx, const JSValue& obj, const char* name, int idx, JSValue* pfield)
{
    *pfield = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsUndefined(*pfield))
        return false;

    return true;
}

bool setStructField(JSContext* ctx, const JSValue& obj, const char* name, int idx, JSValue field)
{
    if (!JS_IsObject(obj))
        return false;

    int ret = JS_SetPropertyStr(ctx, obj, name, field);
    return ret > 0;
}

}
