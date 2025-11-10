#ifndef TRACE_UTILS_H
#define TRACE_UTILS_H

#include "framework_log.h"

#ifdef CONFIG_QUICKAPP_QUALITY_PROFILER
#define QUICKAPP_PROFILE_ENABLED 1
/* clang-format off */
#define PROFILE_FEATURE_MODULE_LOG_TIMESTAMPE(name, dsc)           QuickProfileLogTimeStamp(QUICK_PROFILE_FEATURE_MODULE, name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_BEGIN(name, dsc)                QuickProfileLogBegin(QUICK_PROFILE_FEATURE_MODULE, name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_END(name, dsc)                  QuickProfileLogEnd(QUICK_PROFILE_FEATURE_MODULE, name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_MEM(name, size, count, dsc)     QuickProfileLogMemory(QUICK_PROFILE_FEATURE_MODULE, name, size, count, dsc)
#define PROFILE_FEATURE_MODULE_LOG_ASYNC_BEGIN(name, dsc)          QuickProfileLogAsyncBegin(QUICK_PROFILE_FEATURE_MODULE, name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_ASYNC_END(name, dsc)            QuickProfileLogAsyncEnd(QUICK_PROFILE_FEATURE_MODULE, name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_CLOSE()                         QuickProfileLogClose()
#define PROFILE_FEATURE_MODULE_JS_MEMORY_USAGE(rt, reason)         profile_js_usage(rt, reason)
/* clang-format on */
#else
#define QUICKAPP_PROFILE_ENABLED 0
#define PROFILE_FEATURE_MODULE_LOG_TIMESTAMPE(name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_BEGIN(name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_END(name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_MEM(name, size, count, dsc)
#define PROFILE_FEATURE_MODULE_LOG_ASYNC_BEGIN(name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_ASYNC_END(name, dsc)
#define PROFILE_FEATURE_MODULE_LOG_CLOSE()
#define PROFILE_FEATURE_MODULE_JS_MEMORY_USAGE(rt, reason)
#endif

#endif