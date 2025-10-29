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

#ifndef __VALUE_TRANSLATOR_WAMR_H__
#define __VALUE_TRANSLATOR_WAMR_H__

#include "feature.h"
#include "feature_context.h"
#include "feature_description.h"
#include "gc_export.h"

namespace value_translator {
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, int32_t* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, uint32_t* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, int64_t* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, uint64_t* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, float* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, double* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, bool* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, char** pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, FtJsonObject* pnative);
bool toNative(wasm_exec_env_t exec_env, const uint64_t& val, ft_value_t* pnative);

bool toTarget(wasm_exec_env_t exec_env, int32_t native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, uint32_t native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, int64_t native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, uint64_t native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, float native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, double native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, bool native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, const char* native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, ft_value_t native, uint64_t* ptarget);
bool toTarget(wasm_exec_env_t exec_env, FtArrayBuffer native, uint64_t* ptarget);

static inline bool toTargetJson(wasm_exec_env_t exec_env, FtJsonObject native, uint64_t* ptarget)
{
    return false;
}

bool isNull(wasm_exec_env_t exec_env, const uint64_t& value);

static inline uint64_t nullValue(wasm_exec_env_t exec_env)
{
    return 0;
}

bool isUndefined(wasm_exec_env_t exec_env, const uint64_t& value);

static inline uint64_t undefined(wasm_exec_env_t exec_env)
{
    return 0;
}

bool isString(wasm_exec_env_t exec_env, const uint64_t& value);
void freeCString(wasm_exec_env_t exec_env, char* str);
void freeValue(wasm_exec_env_t exec_env, uint64_t& target);
bool isArray(wasm_exec_env_t exec_env, uint64_t& target);
bool isObject(wasm_exec_env_t exec_env, uint64_t& target);
bool isPlainObject(wasm_exec_env_t exec_env, uint64_t& target);
bool isFunction(wasm_exec_env_t exec_env, uint64_t& target);
bool isArrayBuffer(wasm_exec_env_t exec_env, uint64_t& target);
uint32_t arraySize(wasm_exec_env_t exec_env, const uint64_t& array);
uint64_t arrayGet(wasm_exec_env_t exec_env, const uint64_t& array, uint32_t idx);
bool arraySet(wasm_exec_env_t exec_env, const uint64_t& array, int32_t idx, uint64_t val);
uint64_t newObject(wasm_exec_env_t exec_env);
bool getObjectField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, uint64_t* pfield);
bool setObjectField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, uint64_t field);
uint64_t newArray(wasm_exec_env_t exec_env);
ft_value_t nullFtVal();
JSValue getVariArg(wasm_exec_env_t exec_env, uint64_t* argv, uint32_t index);
uint64_t toTargetPromise(wasm_exec_env_t exec_env, JSContext* js_ctx, const JSValue& promise);
void* interfaceFromTarget(wasm_exec_env_t exec_env, uint64_t& target);
uint64_t targetFromInterface(wasm_exec_env_t exec_env, void* interf);
int checkAsyncCallbacks(wasm_exec_env_t exec_env, uint64_t arg);
int addAsyncCallbacks(wasm_exec_env_t exec_env, void* instance, FeatureType ftype, uint64_t arg);

static inline wasm_obj_t toCallbackValue(uint64_t& target)
{
    wasm_obj_t cb_value = *((wasm_obj_t*)(&target));
    return cb_value;
}

uint64_t createStruct(wasm_exec_env_t exec_env, ObjectMapType& obj_map_type, uint32_t member_count);
bool getStructField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, int idx, uint64_t* pfield);
bool setStructField(wasm_exec_env_t exec_env, const uint64_t& obj, const char* name, int idx, uint64_t field);

uint64_t createArray(wasm_exec_env_t exec_env, FeatureType& type, uint32_t array_size);

static inline uint64_t dupValue(wasm_exec_env_t exec_env, uint64_t& target)
{
    return target;
}

}
#endif // __VALUE_TRANSLATOR_QJS_H__
