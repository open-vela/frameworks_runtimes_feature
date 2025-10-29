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
#include <fcntl.h>
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

static profile_buffer_t g_profile_buffer { 0, nullptr, -1 };

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
            if (g_profile_buffer.fd < 0) {
                // 打开文件：只写模式 | 不存在则创建 | 追加模式
                g_profile_buffer.fd = open("/data/sys.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }

            if (g_profile_buffer.fd > 0) {
                size_t len = strlen(buffer_->framework_buf);
                write(g_profile_buffer.fd, buffer_->framework_buf, len);
                // 重置缓冲区位置
                buffer_->pos = 0;
            }
        }
    }

    int addLogTimeStamp(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
    {
        // 预计算所需长度
        int needed_len = snprintf(nullptr, 0, "|%s|TS|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        if (buffer_->pos + needed_len >= BUFFER_SIZE) {
            flush();
        }
        // 应不存在单条日志超过缓冲区大小
        int len = sprintf(buffer_->framework_buf + buffer_->pos, "|%s|TS|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        buffer_->pos += len;
        return len;
    }

    int addLogBegin(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
    {
        int needed_len = snprintf(nullptr, 0, "|%s|TDB|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        if (buffer_->pos + needed_len >= BUFFER_SIZE) {
            flush();
        }
        int len = sprintf(buffer_->framework_buf + buffer_->pos, "|%s|TDB|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        buffer_->pos += len;
        return len;
    }

    int addLogEnd(QUICK_PROFILE_MOUDLE module, const char* name, const char* dsc)
    {
        int needed_len = snprintf(nullptr, 0, "|%s|TDE|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        if (buffer_->pos + needed_len >= BUFFER_SIZE) {
            flush();
        }
        int len = sprintf(buffer_->framework_buf + buffer_->pos, "|%s|TDE|%" PRId64 "|%s|%s|\n", GetModuleName(module), GetTimeStamp(), name, dsc ? dsc : "");
        buffer_->pos += len;
        return len;
    }
};
#endif

void QuickProfileLogClose()
{
#ifdef CONFIG_FRAMEWORK_ENABLE_LOG
    ProfileBufferWrapper(&g_profile_buffer).flush();
    close(g_profile_buffer.fd);
    g_profile_buffer.fd = -1;
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