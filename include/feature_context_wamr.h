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
#ifndef __FEATURE_CONTEXT_QJS_H__
#define __FEATURE_CONTEXT_QJS_H__

#include "feature_context_private.h"

typedef struct wamr_val_t {
    ft_type type;
# if __WORDSIZE == 64
    uint64_t val[2];
#else
    uint64_t val;
#endif
} qjs_val_t;

bool InitFeatureContextWamr(ft_context_ref ft_ctx);

void UinitFeatureContextWamr(ft_context_ref ft_ctx);

#endif // __FEATURE_CONTEXT_QJS_H__

