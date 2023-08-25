#ifndef FEATURE_UTILS_H
#define FEATURE_UTILS_H

#include <cstdint>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>

#include <uv.h>
#ifndef NULL
#include <stddef.h>
#endif

#include <functional>
#include <vector>
#include <string>
#include "feature_log.h"

#if defined(_WIN32)
#define FEATURE_PATHSEP  '\\'
#define FEATURE_PATHSEPS "\\"
#else
#define FEATURE_PATHSEP  '/'
#define FEATURE_PATHSEPS "/"
#endif

#define FEATURE_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
// 字符串化
#define FEATURE_STRINGIFY_(x) #x
#define FEATURE_STRINGIFY(x)  FEATURE_STRINGIFY_(x)

// 禁止拷贝
#define FEATURE_DISABLE_COPY(cls) \
    cls(const cls &) = delete; \
    cls &operator=(const cls &) = delete

#define FEATURE_DISABLE_MOVE(cls) \
    cls(cls &&) = delete; \
    cls &operator=(cls &&) = delete

#define FEATURE_DISABLE_COPYMOVE(cls) \
    FEATURE_DISABLE_COPY(cls); \
    FEATURE_DISABLE_MOVE(cls)

namespace FEATURE {

    /**
     * @brief 断言信息
     */
    struct FeatureAssertionInfo{
        const char *fileLine;  // 格式为filename:line
        const char *message; // 输出信息
        const char *function;  // 发生断言的函数名
    };

    /**
     * @brief 检查后缀是否匹配
     */
    bool hasSuffix(const char *str, const char *suffix);
}

/**
 * @brief 断言,处理错误中止运行
 */
#define FEATURE_ERROR_AND_ABORT(expr)  \
    do { \
        static const struct FeatureAssertionInfo args__ = { __FILE__ ":" FEATURE_STRINGIFY(__LINE__), #expr, FEATURE_PRETTY_FUNCTION_NAME }; \
        FEATURE_LOG_ERROR("%s:%s%s Assertion `%s' failed.", args__.fileLine, args__.function, *args__.function ? ":" : "", args__.message); \
        assert(0); \
    } while (0)

#ifdef __GNUC__
#define FEATURE_LIKELY(expr)    __builtin_expect(!!(expr), 1)  // 将最有可能执行的分支告诉编译器
#define FEATURE_UNLIKELY(expr)  __builtin_expect(!!(expr), 0)
#define FEATURE_PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define FEATURE_LIKELY(expr)    expr
#define FEATURE_UNLIKELY(expr)  expr
#define FEATURE_PRETTY_FUNCTION_NAME ""
#endif

// 断言检查
#define FEATURE_CHECK(expr)                                                                                                    \
    do {                                                                                                               \
        if (FEATURE_UNLIKELY(!(expr))) {                                                                                  \
            FEATURE_ERROR_AND_ABORT(expr);                                                                                     \
        }                                                                                                              \
    } while (0)


#define FEATURE_CHECK_EQ(a, b)      FEATURE_CHECK((a) == (b))
#define FEATURE_CHECK_GE(a, b)      FEATURE_CHECK((a) >= (b))
#define FEATURE_CHECK_GT(a, b)      FEATURE_CHECK((a) > (b))
#define FEATURE_CHECK_LE(a, b)      FEATURE_CHECK((a) <= (b))
#define FEATURE_CHECK_LT(a, b)      FEATURE_CHECK((a) < (b))
#define FEATURE_CHECK_NE(a, b)      FEATURE_CHECK((a) != (b))
#define FEATURE_CHECK_NULL(val)     FEATURE_CHECK((val) == NULL)
#define FEATURE_CHECK_NOT_NULL(val) FEATURE_CHECK((val) != NULL)


#endif // FEATURE_UTILS_H

