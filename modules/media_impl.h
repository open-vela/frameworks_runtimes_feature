/*
 * Copyright (C) 2025 Xiaomi Corporation. All rights reserved.
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
 *
 */

#ifndef FEATURE_MEDIA_IMPL_H_
#define FEATURE_MEDIA_IMPL_H_

#include <stdint.h>

using namespace std;

typedef void (*mediaFinishCallback)(void* feature, int32_t success, int32_t fail, int32_t complete, const char* msg, int32_t code);
typedef struct {
    void* handle;
    char* current;
    int32_t success;
    int32_t fail;
    int32_t complete;
    mediaFinishCallback finish_callback;
} MediaPreviewImageParams;

typedef void (*mediaPreviewImage)(MediaPreviewImageParams* params);

#endif