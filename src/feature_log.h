#ifndef FEATURE_LOG_H
#define FEATURE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define FEATURE_LOG_LEVEL_DEBUG 0
#define FEATURE_LOG_LEVEL_INFO 1
#define FEATURE_LOG_LEVEL_WARNING 2
#define FEATURE_LOG_LEVEL_ERROR 3
#define FEATURE_LOG_LEVEL_ALERT 4
#define FEATURE_LOG_LEVEL_OFF 5

#ifndef FEATURE_LOG_LEVEL
#define FEATURE_LOG_LEVEL FEATURE_LOG_LEVEL_INFO
#endif

static inline void featurelogPrintf(int level, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#ifdef __NuttX__
    static const int log_map[] = { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR, LOG_ALERT };
    if (level >= FEATURE_LOG_LEVEL) {
        vsyslog(log_map[level], fmt, ap);
    }
#else
    static const char* log_map[] = { "DEBUG", "INFO", "WARN", "ERROR", "ALERT" };
    if (level >= FEATURE_LOG_LEVEL) {
        printf("[%s] ", log_map[level]);
        vprintf(fmt, ap);
    }
#endif
    va_end(ap);
}

#define FEATURE_LOG(level, fmt, ...) \
    featurelogPrintf(level, "[FEATURE] [%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

// void logPrintf(int level, const char *fmt, ...);

#if FEATURE_LOG_LEVEL == FEATURE_LOG_LEVEL_DEBUG
#define FEATURE_LOG_DEBUG(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define FEATURE_LOG_INFO(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define FEATURE_LOG_WARN(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define FEATURE_LOG_ERROR(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#elif FEATURE_LOG_LEVEL == FEATURE_LOG_LEVEL_INFO
#define FEATURE_LOG_DEBUG(fmt, ...)
#define FEATURE_LOG_INFO(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define FEATURE_LOG_WARN(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define FEATURE_LOG_ERROR(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#elif FEATURE_LOG_LEVEL == FEATURE_LOG_LEVEL_WARNING
#define FEATURE_LOG_DEBUG(fmt, ...)
#define FEATURE_LOG_INFO(fmt, ...)
#define FEATURE_LOG_WARN(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define FEATURE_LOG_ERROR(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#elif FEATURE_LOG_LEVEL == FEATURE_LOG_LEVEL_ERROR
#define FEATURE_LOG_DEBUG(fmt, ...)
#define FEATURE_LOG_INFO(fmt, ...)
#define FEATURE_LOG_WARN(fmt, ...)
#define FEATURE_LOG_ERROR(fmt, ...) FEATURE_LOG(FEATURE_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#elif FEATURE_LOG_LEVEL == FEATURE_LOG_LEVEL_OFF
#define FEATURE_LOG_INFO(fmt, ...)
#define FEATURE_LOG_WARN(fmt, ...)
#define FEATURE_LOG_ERROR(fmt, ...)
#define FEATURE_LOG_DEBUG(fmt, ...)
#else
#error invalid log level!
#endif

#ifdef __cplusplus
}
#endif

#endif
