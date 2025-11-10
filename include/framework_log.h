/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#ifndef __FRAMEWORK_LOG_H__
#define __FRAMEWORK_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum QUICK_PROFILE_MOUDLE {
    QUICK_PROFILE_QUICKAPP_FRAMEWORK,
    QUICK_PROFILE_FEATURE_FRAMEWORK,
    QUICK_PROFILE_FEATURE_MODULE,
    QUICK_PROFILE_QUICKAPP_APP,
    QUICK_PROFILE_OTHER,
};

typedef struct profile_buffer_ {
    int pos;
    char* framework_buf;
    int fd;
} profile_buffer_t;

void QuickProfileLogClose();

void QuickProfileLogTimeStamp(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc);
void QuickProfileLogBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc);
void QuickProfileLogEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc);
void QuickProfileLogMemory(QUICK_PROFILE_MOUDLE module, const char* name, uint64_t size, uint64_t count, const char* dsc);
void QuickProfileLogAsyncBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc);
void QuickProfileLogAsyncEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc);

#ifdef __cplusplus
}
#endif
#endif // __FRAMEWORK_LOG_H__