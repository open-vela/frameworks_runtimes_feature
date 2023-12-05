#ifndef FEATURE_UTILS_H
#define FEATURE_UTILS_H

#include "feature_log.h"

#include <cassert>
#include <cstdint>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define FEATURE_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define weakref_container_of(ptr, type, member) \
    ((type*)((uintptr_t)(ptr)-offsetof(type, member)))

struct weakref_list_node {
    struct weakref_list_node* prev;
    struct weakref_list_node* next;
};

#define weakref_list_initialize(list)              \
    do {                                           \
        struct weakref_list_node* __list = (list); \
        __list->prev = __list->next = __list;      \
    } while (0)

#define weakref_list_delete(item)                  \
    do {                                           \
        struct weakref_list_node* __item = (item); \
        __item->next->prev = __item->prev;         \
        __item->prev->next = __item->next;         \
        __item->prev = __item->next = NULL;        \
    } while (0)

#define weakref_list_add_tail(list, item)          \
    do {                                           \
        struct weakref_list_node* __list = (list); \
        struct weakref_list_node* __item = (item); \
        __item->prev = __list->prev;               \
        __item->next = __list;                     \
        __list->prev->next = __item;               \
        __list->prev = __item;                     \
    } while (0)

#define weakref_list_for_every_entry_safe(list, entry, temp, type, member) \
    for (entry = weakref_container_of((list)->next, type, member),         \
        temp = weakref_container_of(entry->member.next, type, member);     \
         &entry->member != (list); entry = temp,                           \
        temp = weakref_container_of(temp->member.next, type, member))

namespace FEATURE {

struct FeatureAssertionInfo {
    const char* fileLine; // 格式为filename:line
    const char* message; // 输出信息
    const char* function; // 发生断言的函数名
};
}

#define FEATURE_STRINGIFY_(x) #x
#define FEATURE_STRINGIFY(x) FEATURE_STRINGIFY_(x)

/**
 * @brief 断言,处理错误中止运行
 */
#define FEATURE_ERROR_AND_ABORT(expr)                                                                                                                 \
    do {                                                                                                                                              \
        static const struct FEATURE::FeatureAssertionInfo args__ = { __FILE__ ":" FEATURE_STRINGIFY(__LINE__), #expr, FEATURE_PRETTY_FUNCTION_NAME }; \
        FEATURE_LOG_ERROR("%s:%s%s Assertion `%s' failed.", args__.fileLine, args__.function, *args__.function ? ":" : "", args__.message);           \
        assert(0);                                                                                                                                    \
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
#define FEATURE_CHECK(expr)                \
    do {                                   \
        if (FEATURE_UNLIKELY(!(expr))) {   \
            FEATURE_ERROR_AND_ABORT(expr); \
        }                                  \
    } while (0)

#define FEATURE_CHECK_EQ(a, b) FEATURE_CHECK((a) == (b))
#define FEATURE_CHECK_GE(a, b) FEATURE_CHECK((a) >= (b))
#define FEATURE_CHECK_GT(a, b) FEATURE_CHECK((a) > (b))
#define FEATURE_CHECK_LE(a, b) FEATURE_CHECK((a) <= (b))
#define FEATURE_CHECK_LT(a, b) FEATURE_CHECK((a) < (b))
#define FEATURE_CHECK_NE(a, b) FEATURE_CHECK((a) != (b))
#define FEATURE_CHECK_NULL(val) FEATURE_CHECK((val) == NULL)
#define FEATURE_CHECK_NOT_NULL(val) FEATURE_CHECK((val) != NULL)

#endif // FEATURE_UTILS_H
