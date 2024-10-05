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
#ifndef __FEATURE_CONTEXT_WAMR_H__
#define __FEATURE_CONTEXT_WAMR_H__

#include "feature_context_private.h"
#include "wasm_export.h"

#define FT_VAL_TO_WM(ft_val) (*((wamr_val_t*)(&(ft_val))))
#define FT_VAL_TO_WM_PTR(ft_val) ((wamr_val_t*)(&(ft_val)))
#define FT_VAL_GET_WM_VAL(ft_val) (((wamr_val_t*)(&(ft_val)))->wm_val)
#define FT_VAL_GET_WM_VAL_PTR(ft_val) (&(((wamr_val_t*)(&(ft_val)))->wm_val))

#define WM_VAL_TO_FT(wm_val) (*((ft_value_t*)(&(wm_val))))
#define WM_VAL_TO_FT_PTR(wm_val) ((ft_value_t*)(&(wm_val)))

typedef struct wamr_val_t {
    wasm_val_t wm_val;
    ft_type type;
} wamr_val_t;

bool InitFeatureContextWamr(ft_context_ref ft_ctx, void* data0, void* data1);

void UninitFeatureContextWamr(ft_context_ref ft_ctx);

#endif // __FEATURE_CONTEXT_WAMR_H__
