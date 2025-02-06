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
#include <chrono>
#include <cstdio>
#include <inttypes.h>
#include <syslog.h>
#endif

#ifndef CONFIG_FRAMEWORK_LOG_BUFFER_SIZE
#define CONFIG_FRAMEWORK_LOG_BUFFER_SIZE (1024 * 1024)
#endif

#if CONFIG_FRAMEWORK_LOG_BUFFER_SIZE > 100
#define BUFFER_SIZE (CONFIG_FRAMEWORK_LOG_BUFFER_SIZE - 100)
#else
#define BUFFER_SIZE CONFIG_FRAMEWORK_LOG_BUFFER_SIZE
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

#ifdef CONFIG_FRAMEWORK_ENABLE_LOG

inline int64_t GetTimeStamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
}

static profile_buffer_t g_profile_buffer { 0, nullptr };

struct ProfileBufferWrapper {
    profile_buffer_t* buffer_;
    ProfileBufferWrapper(profile_buffer_t* buffer)
    {
        buffer_ = buffer;
        if (!buffer_->framework_buf) {
            buffer_->framework_buf = new char[CONFIG_FRAMEWORK_LOG_BUFFER_SIZE];
        }
    }

    ~ProfileBufferWrapper()
    {
    }

    void flush()
    {
        if (buffer_->pos > 0) {
            syslog(LOG_ERR, "%s", buffer_->framework_buf);
            buffer_->pos = 0;
        }
    }

    int addLogTimeStamp(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
    {
        int len = sprintf(buffer_->framework_buf + buffer_->pos, "|%s|TS|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        buffer_->pos += len;
        if (buffer_->pos >= BUFFER_SIZE) {
            // dump it
            syslog(LOG_ERR, "%s", buffer_->framework_buf);
            buffer_->pos = 0;
        }
        return len;
    }

    int addLogBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
    {
        int len = sprintf(buffer_->framework_buf + buffer_->pos, "|%s|TDB|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        buffer_->pos += len;
        if (buffer_->pos >= BUFFER_SIZE) {
            syslog(LOG_ERR, "%s", buffer_->framework_buf);
            buffer_->pos = 0;
        }
        return len;
    }

    int addLogEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
    {
        int len = sprintf(buffer_->framework_buf + buffer_->pos, "|%s|TDE|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        buffer_->pos += len;
        if (buffer_->pos >= BUFFER_SIZE) {
            syslog(LOG_ERR, "%s", buffer_->framework_buf);
            buffer_->pos = 0;
        }
        return len;
    }
};
#endif

void QuickProfileLogFlush()
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    ProfileBufferWrapper(&g_profile_buffer).flush();
#endif
}

void QuickProfileLogTimeStamp(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    ProfileBufferWrapper(&g_profile_buffer).addLogTimeStamp(module, name, dsc);
#endif
}

void QuickProfileLogBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    ProfileBufferWrapper(&g_profile_buffer).addLogBegin(module, name, dsc);
#endif
}

void QuickProfileLogEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    ProfileBufferWrapper(&g_profile_buffer).addLogEnd(module, name, dsc);
#endif
}

void QuickProfileLogMemory(QUICK_PROFILE_MOUDLE module, const char* name, uint64_t size, uint64_t count, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|M|%" PRIu64 "-%" PRIu64 "|%s|%s|", GetModuleName(module), size, count, name, dsc ? dsc : "");
#endif
}

void QuickProfileLogAsyncBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|ATD|%" PRId64 "|%s|%s|", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
#endif
}

void QuickProfileLogAsyncEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    syslog(LOG_ERR, "QAPP_PROFILE|%s|ATD|%" PRId64 "|%s|%s|", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
#endif
}