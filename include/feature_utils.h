#ifndef FEATURE_UTILS_H
#define FEATURE_UTILS_H

#include "feature_list.h"
#include "feature_log.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct FeatureAssertionInfo {
    const char* fileLine; // 格式为filename:line
    const char* message; // 输出信息
    const char* function; // 发生断言的函数名
};

#define FEATURE_STRINGIFY_(x) #x
#define FEATURE_STRINGIFY(x) FEATURE_STRINGIFY_(x)

/**
 * @brief 断言,处理错误中止运行
 */
#define FEATURE_ERROR_AND_ABORT(expr, fmt, ...)                                                                                                                \
    do {                                                                                                                                                       \
        static const struct FeatureAssertionInfo args__ = { __FILE__ ":" FEATURE_STRINGIFY(__LINE__), #expr, FEATURE_PRETTY_FUNCTION_NAME };                   \
        FEATURE_LOG_ERROR("%s:%s%s Assertion `%s' failed." fmt, args__.fileLine, args__.function, *args__.function ? ":" : "", args__.message, ##__VA_ARGS__); \
        /* assert(0); */                                                                                                                                       \
    } while (0)

#ifdef __GNUC__
#define FEATURE_LIKELY(expr) __builtin_expect(!!(expr), 1) // 将最有可能执行的分支告诉编译器
#define FEATURE_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define FEATURE_PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define FEATURE_LIKELY(expr) expr
#define FEATURE_UNLIKELY(expr) expr
#define FEATURE_PRETTY_FUNCTION_NAME ""
#endif

// 断言检查
#define FEATURE_CHECK(expr, fmt, ...)                          \
    do {                                                       \
        if (FEATURE_UNLIKELY(!(expr))) {                       \
            FEATURE_ERROR_AND_ABORT(expr, fmt, ##__VA_ARGS__); \
        }                                                      \
    } while (0)

#define FEATURE_CHECK_EQ(a, b) FEATURE_CHECK((a) == (b), "")
#define FEATURE_CHECK_GE(a, b) FEATURE_CHECK((a) >= (b), "")
#define FEATURE_CHECK_GT(a, b) FEATURE_CHECK((a) > (b), "")
#define FEATURE_CHECK_LE(a, b) FEATURE_CHECK((a) <= (b), "")
#define FEATURE_CHECK_LT(a, b) FEATURE_CHECK((a) < (b), "")
#define FEATURE_CHECK_NE(a, b) FEATURE_CHECK((a) != (b), "")
#define FEATURE_CHECK_NULL(val) FEATURE_CHECK((val) == NULL, "")
#define FEATURE_CHECK_NOT_NULL(val) FEATURE_CHECK((val) != NULL, "")

#define FEATURE_CHECK_EQ_LOG(a, b, fmt, ...) FEATURE_CHECK((a) == (b), fmt, ##__VA_ARGS__)
#define FEATURE_CHECK_GE_LOG(a, b, fmt, ...) FEATURE_CHECK((a) >= (b), fmt, ##__VA_ARGS__)
#define FEATURE_CHECK_GT_LOG(a, b, fmt, ...) FEATURE_CHECK((a) > (b), fmt, ##__VA_ARGS__)
#define FEATURE_CHECK_LE_LOG(a, b, fmt, ...) FEATURE_CHECK((a) <= (b), fmt, ##__VA_ARGS__)
#define FEATURE_CHECK_LT_LOG(a, b, fmt, ...) FEATURE_CHECK((a) < (b), fmt, ##__VA_ARGS__)
#define FEATURE_CHECK_NE_LOG(a, b, fmt, ...) FEATURE_CHECK((a) != (b), fmt, ##__VA_ARGS__)
#define FEATURE_CHECK_NULL_LOG(val, fmt, ...) FEATURE_CHECK((val) == NULL, fmt, ##__VA_ARGS__)
#define FEATURE_CHECK_NOT_NULL_LOG(val, fmt, ...) FEATURE_CHECK((val) != NULL, fmt, ##__VA_ARGS__)

#endif // FEATURE_UTILS_H
