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

#ifndef __VALUE_TRANSLATOR_QJS_H__
#define __VALUE_TRANSLATOR_QJS_H__

#include "feature.h"
#include "feature_log.h"
#include "feature_context_qjs.h"

#include <cstdarg>
#include <stdalign.h>

namespace value_translator {

// to native values
static inline bool toNative(JSContext* ctx, const JSValue& target, int32_t* pnative) {
    if (!JS_IsNumber(target)) {
        FEATURE_LOG_ERROR("arg type mismatch, need number type arg !");
        return false;
    }
    return JS_ToInt32(ctx, pnative, target) == 0;
}

static inline bool toNative(JSContext* ctx, const JSValue& target, uint32_t* pnative) {
    if (!JS_IsNumber(target)) {
        FEATURE_LOG_ERROR("arg type mismatch, need number type arg !");
        return false;
    }
    return JS_ToUint32(ctx, pnative, target) == 0;
}

static inline bool toNative(JSContext* ctx, const JSValue& target, int64_t* pnative) {
    if (!JS_IsNumber(target)) {
        FEATURE_LOG_ERROR("arg type mismatch, need number type arg !");
        return false;
    }
    return JS_ToInt64(ctx, pnative, target) == 0;
}

static inline bool toNative(JSContext* ctx, const JSValue& target, uint64_t* pnative) {
    if (!JS_IsNumber(target)) {
        FEATURE_LOG_ERROR("arg type mismatch, need number type arg !");
        return false;
    }
    return JS_ToIndex(ctx, pnative, target) == 0;
}

static inline bool toNative(JSContext* ctx, const JSValue& target, double* pnative) {
    if (!JS_IsNumber(target)) {
        FEATURE_LOG_ERROR("arg type mismatch, need number type arg !");
        return false;
    }
    return JS_ToFloat64(ctx, pnative, target) == 0;
}

bool toNative(JSContext* ctx, const JSValue& target, bool* pnative);

bool toNative(JSContext* ctx, const JSValue& target, char** pnative);

bool toNative(JSContext* ctx, const JSValue& target, ft_value_t* pnative);

// ArrayBuffer or TypedArrayBuffer
bool toNativeBuffer(JSContext* ctx, const JSValue& target, uint8_t** pnative, size_t* psize);

// to target values
static inline bool toTarget(JSContext* ctx, ft_value_t native, JSValue* ptarget) {
    *ptarget = FT_VAL_GET_JS_VAL(native);
    return true;
}

static inline bool toTarget(JSContext* ctx, int32_t native, JSValue* ptarget) {
    *ptarget = JS_NewInt32(ctx, native);
    return true;
}

static inline bool toTarget(JSContext* ctx, uint32_t native, JSValue* ptarget) {
    *ptarget = JS_NewUint32(ctx, native);
    return true;
}

static inline bool toTarget(JSContext* ctx, int64_t native, JSValue* ptarget) {
    *ptarget = JS_NewInt64(ctx, native);
    return true;
}

static inline bool toTarget(JSContext* ctx, uint64_t native, JSValue* ptarget) {
    *ptarget = JS_NewBigUint64(ctx, native);
    return true;
}

static inline bool toTarget(JSContext* ctx, double native, JSValue* ptarget) {
    *ptarget = JS_NewFloat64(ctx, native);
    return true;
}

static inline bool toTarget(JSContext* ctx, bool native, JSValue* ptarget) {
    *ptarget = JS_NewBool(ctx, native);
    return true;
}

static inline bool toTarget(JSContext* ctx, const char* native, JSValue* ptarget) {
    *ptarget = JS_NewString(ctx, native);
    return true;
}

bool toTarget(JSContext* ctx, ft_value_t native, JSValue* ptarget);

// for ArrayBuffer
static inline bool toTargetBuffer(JSContext* ctx, uint8_t* buff, uint32_t size, JSValue* ptarget) {
    *ptarget = JS_NewArrayBufferCopy(ctx, buff, size);
    return true;
}

// for TypedArrayBuffer
bool toTargetTypedBuffer(JSContext* ctx, uint8_t* buff, uint32_t size, uint32_t type, JSValue* ptarget);

// for arrays
bool toTargetArray(JSContext* ctx, int32_t* val, uint32_t size, JSValue* ptarget);

bool toTargetArray(JSContext* ctx, uint32_t* val, uint32_t size, JSValue* ptarget);

bool toTargetArray(JSContext* ctx, int64_t* val, uint32_t size, JSValue* ptarget);

bool toTargetArray(JSContext* ctx, uint64_t* val, uint32_t size, JSValue* ptarget);

bool toTargetArray(JSContext* ctx, double* val, uint32_t size, JSValue* ptarget);

bool toTargetArray(JSContext* ctx, bool* val, uint32_t size, JSValue* ptarget);

bool toTargetArray(JSContext* ctx, const char** val, uint32_t size, JSValue* ptarget);

// object
static inline JSValue newObject (JSContext* ctx) {
    return JS_NewObject(ctx);
}

bool getObjectField(JSContext* ctx, const JSValue& obj, const char* name, int idx, JSValue* pfield);

bool setObjectField(JSContext* ctx, const JSValue& obj, const char* name, JSValue field);

// function for handling arrays
static inline JSValue newArray (JSContext* ctx) {
    return JS_NewArray(ctx);
}

uint32_t arraySize(JSContext* ctx, const JSValue& array);

JSValue arrayGet(JSContext* ctx, const JSValue& array, uint32_t idx);

bool arraySet(JSContext* ctx, const JSValue& array, int32_t idx, JSValue val);

static inline bool isNull(JSContext* ctx, const JSValue& target) {
    return !!JS_IsNull(target);
}

static inline bool isUndefined(JSContext* ctx, const JSValue& target) {
    return !!JS_IsUndefined(target);
}

static inline bool isString(JSContext* ctx, const JSValue& target) {
    return !!JS_IsString(target);
}

static inline bool isArray(JSContext* ctx, const JSValue& target) {
    return !!JS_IsArray(ctx, target);
}

static inline void freeValue(JSContext* ctx, JSValue target) {
    JS_FreeValue(ctx, target);
    target = JS_UNDEFINED;
}

static inline void freeCString(JSContext* ctx, char* str) {
    JS_FreeCString(ctx, str);
}

JSValue parseJson(JSContext* ctx, const char* buf, size_t buf_len, const char* file_name);

static inline ft_value_t targetToFtVal(JSValue& target)
{
    ft_value_t ft_val;
    qjs_val_t* qjs_val = FT_VAL_TO_QJS_PTR(ft_val);
    qjs_val->js_val = target;
    return ft_val;
}

static inline JSValue ftValToTarget(ft_value_t& ft_val)
{
    qjs_val_t* qjs_val = FT_VAL_TO_QJS_PTR(ft_val);
    return qjs_val->js_val;
}

static inline ft_value_t nullFtVal()
{
    ft_value_t ft_val;
    qjs_val_t* qjs_val = FT_VAL_TO_QJS_PTR(ft_val);
    qjs_val->js_val = JS_NULL;
    return ft_val;
}

static inline ft_value_t undefinedFtVal()
{
    ft_value_t ft_val;
    qjs_val_t* qjs_val = FT_VAL_TO_QJS_PTR(ft_val);
    qjs_val->js_val = JS_UNDEFINED;
    return ft_val;
}

static inline JSValue getVariArg(JSContext* ctx, JSValue& arg)
{
    return arg;
}

static inline void toTargetPromise(JSContext* ctx, const JSValue& promise, JSValue& ret_val)
{
    ret_val = promise;
}

void* interfaceFromTarget(JSValue& target);

JSValue targetFromInterface(void* instance);

static inline JSValue toCallbackValue(JSValue& target)
{
    return target;
}

}
#endif // __VALUE_TRANSLATOR_QJS_H__
