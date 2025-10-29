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

#include "array_buffer_qjs.h"
#include "feature_context_qjs.h"
#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_utils.h"

namespace feature_framework {

struct ArrayBufferFreeInfo {
    FeatureArrayBufferFreeFunc free_func;
    void* opaque;
};

static void free_func_with_opaque(JSRuntime* rt, void* data, void* ptr)
{
    auto info = (ArrayBufferFreeInfo*)data;
    if (info && info->free_func) {
        info->free_func(info->opaque, ptr);
        free(info);
    }
}

static void free_func_without_opaque(JSRuntime* rt, void* data, void* ptr)
{
    auto free_func = (FeatureArrayBufferFreeFunc)data;
    if (free_func) {
        free_func(NULL, ptr);
    }
}

static void default_free_func(JSRuntime* rt, void* data, void* ptr)
{
    if (ptr) {
        free(ptr);
    }
}

ArrayBufferQjs::ArrayBufferQjs(FeatureManagerQjs* manager, const ArrayBufferCreateParams& params)
    : manager_(manager)
{
    FEATURE_CHECK_NE(manager_, nullptr);
    JSContext* ctx = (JSContext*)ft_context_get_data(manager_->getFeatureContext());
    if (params.type == ArrayBufferCreateParams::kNative) {
        uint8_t* data = params.native.data;
        size_t size = params.native.size;
        FeatureArrayBufferFreeFunc free_func = params.native.free_func;
        void* opaque = params.native.opaque;
        if (free_func && opaque) {
            ArrayBufferFreeInfo* info = new ArrayBufferFreeInfo { free_func, opaque };
            val_ = JS_NewArrayBuffer(ctx, data, size, free_func_with_opaque, info, false);
        } else if (free_func) {
            val_ = JS_NewArrayBuffer(ctx, data, size, free_func_without_opaque, (void*)free_func, false);
        } else {
            val_ = JS_NewArrayBuffer(ctx, data, size, default_free_func, nullptr, false);
        }
    } else if (params.type == ArrayBufferCreateParams::kNativeCopy) {
        val_ = JS_NewArrayBufferCopy(ctx, params.copy.data, params.copy.size);
    } else { // params.type == ArrayBufferCreateParams::kTarget
        val_ = JS_DupValue(ctx, FT_VAL_GET_JS_VAL(params.target));
    }
    entry_ = manager_->insertClearable(this);
}

ArrayBufferQjs::~ArrayBufferQjs()
{
    release();
}

uint8_t* ArrayBufferQjs::getData(size_t* psize)
{
    if (JS_IsUndefined(val_)) {
        return NULL;
    }

    JSContext* ctx = (JSContext*)ft_context_get_data(manager_->getFeatureContext());
    size_t size = 0;
    uint8_t* data = JS_GetArrayBuffer(ctx, &size, val_);
    if (psize) {
        *psize = size;
    }
    return data;
}

void ArrayBufferQjs::destroy()
{
    this->~ArrayBufferQjs();
}

void ArrayBufferQjs::clear()
{
    release();
}

void ArrayBufferQjs::release()
{
    if (JS_IsUndefined(val_)) {
        return;
    }

    JSContext* ctx = (JSContext*)ft_context_get_data(manager_->getFeatureContext());
    manager_->removeClearable(entry_);
    JS_FreeValue(ctx, val_);
    val_ = JS_UNDEFINED;
}

} // namespace feature_framework