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

#ifndef __FEATURE_VALUE_TRANSLATOR_H__
#define __FEATURE_VALUE_TRANSLATOR_H__

#include "value_translator.h"

template <typename TNative, typename TCtx, typename TTarget>
struct FtValTranslator {
    static ft_value_t from(ft_context_ref ctx_ref, TNative native)
    {
        TTarget target;
        value_translator::toTarget((TCtx)(ctx_ref->data), native, &target);
        return value_translator::targetToFtVal(target);
    }

    static ft_value_t fromArray(ft_context_ref ctx_ref, TNative* pnative, uint32_t size)
    {
        TTarget target;
        value_translator::toTargetArray((TCtx)(ctx_ref->data), pnative, size, &target);
        return value_translator::targetToFtVal(target);
    }

    static ft_value_t fromBuffer(ft_context_ref ctx_ref, uint8_t* buff, uint32_t size)
    {
        TTarget target;
        value_translator::toTargetBuffer((TCtx)(ctx_ref->data), buff, size, &target);
        return value_translator::targetToFtVal(target);
    }

    static ft_value_t fromTypedBuffer(ft_context_ref ctx_ref, uint8_t* buff, uint32_t size, FtTypedArrayType type)
    {
        TTarget target;
        value_translator::toTargetTypedBuffer((TCtx)(ctx_ref->data), buff, size, type, &target);
        return value_translator::targetToFtVal(target);
    }

    static bool to(ft_context_ref ctx_ref, ft_value_t ft_val, TNative* pnative)
    {
        return value_translator::toNative((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val), pnative);
    }

    static const char* toString(ft_context_ref ctx_ref, ft_value_t ft_val)
    {
        char* ret = NULL;
        if (!value_translator::toNative((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val), &ret))
            return NULL;
        return ret;
    }

    // for ArrayBuffer and TypedArrayBuffer
    static uint8_t* toBuffer(ft_context_ref ctx_ref, size_t* psize, ft_value_t ft_val)
    {
        uint8_t* ret = NULL;
        if (!value_translator::toNativeBuffer((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val), &ret, psize))
            return NULL;
        return ret;
    }

    static uint32_t arraySize(ft_context_ref ctx_ref, ft_value_t ft_val)
    {
        return value_translator::arraySize((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val));
    }

    static ft_value_t arrayGet(ft_context_ref ctx_ref, ft_value_t ft_val, uint32_t idx)
    {
        TTarget target = value_translator::arrayGet((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val), idx);
        return value_translator::targetToFtVal(target);
    }

    static ft_value_t newObject(ft_context_ref ctx_ref)
    {
        TTarget target = value_translator::newObject((TCtx)(ctx_ref->data));
        return value_translator::targetToFtVal(target);
    }

    static ft_value_t objectGetProperty(ft_context_ref ctx_ref, ft_value_t ft_val, const char* name)
    {
        TTarget target = value_translator::undefined((TCtx)(ctx_ref->data));
        value_translator::getObjectField((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val), name, &target);
        return value_translator::targetToFtVal(target);
    }

    static bool objectSetProperty(ft_context_ref ctx_ref, ft_value_t ft_val, const char* name, ft_value_t field)
    {
        return value_translator::setObjectField((TCtx)(ctx_ref->data),
            value_translator::ftValToTarget(ft_val), name, value_translator::ftValToTarget(field));
    }

    static void freeValue(ft_context_ref ctx_ref, ft_value_t ft_val)
    {
        value_translator::freeValue((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val));
    }

    static void dupValue(ft_context_ref ctx_ref, ft_value_t ft_val)
    {
        value_translator::dupValue((TCtx)(ctx_ref->data), value_translator::ftValToTarget(ft_val));
    }

    static void freeCString(ft_context_ref ctx_ref, const char* str)
    {
        value_translator::freeCString((TCtx)(ctx_ref->data), (char*)str);
    }

    static ft_value_t parseJson(ft_context_ref ctx_ref, const char* buf, size_t buf_len, const char* file_name)
    {
        TTarget target = value_translator::parseJson((TCtx)(ctx_ref->data), buf, buf_len, file_name);
        return value_translator::targetToFtVal(target);
    }

    static ft_value_t undefined(ft_context_ref ctx_ref)
    {
        TTarget target = value_translator::undefined((TCtx)(ctx_ref->data));
        return value_translator::targetToFtVal(target);
    }
};

#endif // __FEATURE_VALUE_TRANSLATOR_H__