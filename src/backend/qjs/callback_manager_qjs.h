
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

#ifndef __CALLBACK_MANAGER_QJS_H__
#define __CALLBACK_MANAGER_QJS_H__

#include "feature.h"

namespace feature_framework {

static inline JSValue addRef(JSContext* ctx, JSValue& value)
{
    return JS_DupValue(ctx, value);
}

static inline void releaseRef(JSContext* ctx, JSValue& value)
{
    JS_FreeValue(ctx, value);
}

static inline bool isSameValue(JSContext* ctx, JSValue& lvalue, JSValue& ralue)
{
    JS_BOOL ret = JS_IsSameValue(ctx, lvalue, ralue);
    return ret == 1;
}

}
#endif // __CALLBACK_MANAGER_QJS_H__
