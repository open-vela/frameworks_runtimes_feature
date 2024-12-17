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

#include "framework_log.h"
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
#include <cstdio>
#include <ctime>
#include <inttypes.h>
#include <syslog.h>
#endif

inline const char* GetModuleName(QUICK_PROFILE_MOUDLE module)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    static const char* QUICKAPP_PROFILE_MODULE_NAMES[] {
        "QF",
        "FF",
        "FM",
        "QA",
        "OT",
    };
    return QUICKAPP_PROFILE_MODULE_NAMES[module];
#endif
    return nullptr;
}

void QuickProfileLogTimeStamp(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|TS|%ld|%s|%s", GetModuleName(module), time(NULL), name, dsc);
#endif
}

void QuickProfileLogBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|TDB|%ld|%s|%s", GetModuleName(module), time(NULL), name, dsc);
#endif
}

void QuickProfileLogEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|TDE|%ld|%s|%s", GetModuleName(module), time(NULL), name, dsc);
#endif
}

void QuickProfileLogMemory(QUICK_PROFILE_MOUDLE module, const char* name, uint64_t size, uint64_t count, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|M|%" PRIu64 "-%" PRIu64 "|%s|%s", GetModuleName(module), size, count, name, dsc);
#endif
}

void QuickProfileLogAsyncBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|ATD|%ld|%s|%s", GetModuleName(module), time(NULL), name, dsc);
#endif
}

void QuickProfileLogAsyncEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|ATD|%ld|%s|%s", GetModuleName(module), time(NULL), name, dsc);
#endif
}
