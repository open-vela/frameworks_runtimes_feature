/*
 * Copyright (C) 2023 Xiaomi Corporation
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

/**
 * @file feature_types.h
 * @brief 该文件中定义了一系列featrue框架及feature开发者用到的数据类型
 */
#ifndef FEATURE_TYPES_H
#define FEATURE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "feature_context.h"
#include <stdbool.h>

// primitive type definations
typedef int FtInt; /**< FtInt for int32_t */
typedef int8_t FtInt8; /**< FtInt8 for int8_t */
typedef uint8_t FtUint8; /**< FtUint8 for uint8_t */
typedef int16_t FtInt16; /**< FtInt16 for int16_t */
typedef uint16_t FtUint16; /**< FtUint16 for uint16_t */
typedef int32_t FtInt32; /**< FtInt32 for int32_t */
typedef uint32_t FtUint32; /**< FtUint32 for uint32_t */
typedef int64_t FtInt64; /**< FtInt64 for int64_t */
typedef uint64_t FtUint64; /**< FtUint64 for uint64_t */
typedef float FtFloat; /**< FtFloat for float */
typedef double FtDouble; /**< FtDouble for double */
typedef bool FtBool; /**< FtBool for bool */
typedef const char* FtString; /**< FtString for const char* */
typedef ft_value_t* FtAny; /**< FtAny for ft_value_t* */
typedef int32_t FtCallbackId; /**< callbackid */
typedef int32_t FtEventId; /**< event id */
typedef int32_t FtPromiseId; /**< promise id */

typedef void* FeatureRuntimeContext; /**< guest runtime context, e.g qucikjs RuntimeContext */

typedef void* FeatureRegistryHandle; /**< feature registry handle. */
typedef void* FeatureManagerHandle; /**< feature manage handle. */
typedef void* FeatureProtoHandle; /**< feature prototype handle. */
typedef void* FeatureInstanceHandle; /**< feature instance handle. */
typedef void* FeatureInterfaceHandle; /**< feature interface handle. */

typedef uintptr_t FeatureType; /**< feature type flag */
typedef void (*NativeFunc)(void); /**< native func ptr */

/** 异步任务模式枚举 */
enum FeatureTaskMode {
    FEATURE_TASK_MODE_FREE = 0, /**< 异步任务已结束 */
    FEATURE_TASK_MODE_NORMAL = 1, /**< 异步任务正常 */
};

/** FeaturePromiseType枚举 */
typedef enum FeaturePromiseType {
    FEATURE_PROMISE_TYPE_INVALID = -1, /**< 非法类型 */
    FEATURE_PROMISE_TYPE_PROMISE = 0, /**< 表示Promise */
    FEATURE_PROMISE_TYPE_CALLBACKS = 1, /**< 表示Callback */
} FeaturePromiseType;

/** feature类型标志枚举，标记是否需要free */
enum TypeFlags {
    TYPE_FLAGS_VALUE = 1, /**< 值类型，不需要free */
    TYPE_FLAGS_POINTER, /**< 指针类型, 需要malloc&free */
    TYPE_FLAGS_RAWPOINTER = TYPE_FLAGS_POINTER | 1, /**< raw pointer, 不需要free */
    TYPE_FLAGS_UNMANAGED_POINTER = TYPE_FLAGS_RAWPOINTER, /**< such as TYPE_FLAGS_RAWPOINTER */
};

/** 异步任务回调 */
typedef void (*FeatureTaskCallback)(int status, void* data);

/** 事件状态枚举 */
typedef enum FeatureEventStatus {
    FEATURE_EVENT_ADDED, /**< 事件添加 */
    FEATURE_EVENT_REMOVED, /**< 事件移出 */
} FeatureEventStatus;

/** 事件监听函数指针 */
typedef void (*FeatureEventChangeListener)(FeatureInstanceHandle data, FtEventId eid, FeatureEventStatus status);

/** 虚表结构体，用于创建feature interface */
typedef struct VTable {
    int size; /**< 虚表成员个数 */
    NativeFunc finalizer; /**< 接口析构函数指针 */
    const NativeFunc* members; /**< 虚表成员数组 */
} VTable;

/** FeaturePrimitiveTypeBase */
enum FeaturePrimitiveTypeBase {
    FT_VOID_BASE = 0, /**< 0 */
    FT_INT_BASE, /**< 1 */
    FT_INT8_BASE, /**< 2 */
    FT_UINT8_BASE, /**< 3 */
    FT_INT16_BASE, /**< 4 */
    FT_UINT16_BASE, /**< 5 */
    FT_INT32_BASE, /**< 6 */
    FT_UINT32_BASE, /**< 7 */
    FT_INT64_BASE, /**< 8 */
    FT_UINT64_BASE, /**< 9 */
    FT_FLOAT_BASE, /**< 10 */
    FT_DOUBLE_BASE, /**< 11 */
    FT_BOOLEAN_BASE, /**< 12 */
    FT_STRING_BASE, /**< 13 */
    FT_ANY_REF_BASE, /**< 14 */
};

/** set FeaturePrimitiveType */
#define FT_SET_PRIMITIVE_TYPE(base, flags) ((base << 2) | (flags))

/** FeaturePrimitiveType */
enum FeaturePrimitiveType {
    FT_VOID = FT_SET_PRIMITIVE_TYPE(FT_VOID_BASE, TYPE_FLAGS_VALUE), /**< 1: void defination */
    FT_INT = FT_SET_PRIMITIVE_TYPE(FT_INT_BASE, TYPE_FLAGS_VALUE), /**< 5: int32_t defination */
    FT_INT8 = FT_SET_PRIMITIVE_TYPE(FT_INT8_BASE, TYPE_FLAGS_VALUE), /**< 9: int8_t defination */
    FT_UINT8 = FT_SET_PRIMITIVE_TYPE(FT_UINT8_BASE, TYPE_FLAGS_VALUE), /**< 13: uint8_t defination */
    FT_INT16 = FT_SET_PRIMITIVE_TYPE(FT_INT16_BASE, TYPE_FLAGS_VALUE), /**< 17: int16_t defination */
    FT_UINT16 = FT_SET_PRIMITIVE_TYPE(FT_UINT16_BASE, TYPE_FLAGS_VALUE), /**< 21: uint16_t defination */
    FT_INT32 = FT_SET_PRIMITIVE_TYPE(FT_INT32_BASE, TYPE_FLAGS_VALUE), /**< 25: int32_t defination */
    FT_UINT32 = FT_SET_PRIMITIVE_TYPE(FT_UINT32_BASE, TYPE_FLAGS_VALUE), /**< 29: uint32_t defination */
    FT_INT64 = FT_SET_PRIMITIVE_TYPE(FT_INT64_BASE, TYPE_FLAGS_VALUE), /**< 33: int64_t defination */
    FT_UINT64 = FT_SET_PRIMITIVE_TYPE(FT_UINT64_BASE, TYPE_FLAGS_VALUE), /**< 37: uint64_t defination */
    FT_FLOAT = FT_SET_PRIMITIVE_TYPE(FT_FLOAT_BASE, TYPE_FLAGS_VALUE), /**< 41: float defination */
    FT_DOUBLE = FT_SET_PRIMITIVE_TYPE(FT_DOUBLE_BASE, TYPE_FLAGS_VALUE), /**< 45: double defination */
    FT_BOOLEAN = FT_SET_PRIMITIVE_TYPE(FT_BOOLEAN_BASE, TYPE_FLAGS_VALUE), /**< 49: bool defination */
    FT_STRING = FT_SET_PRIMITIVE_TYPE(FT_STRING_BASE, TYPE_FLAGS_POINTER), /**< 54: const char* defination */
    FT_CHAR = FT_STRING, /**< 54: const char* defination */
    FT_ANY_REF = FT_SET_PRIMITIVE_TYPE(FT_ANY_REF_BASE, TYPE_FLAGS_POINTER), /**< 58: ft_value_t* defination */
};

/** Feature异常退出码 */
typedef enum FeatureErrorCode {
    FT_ERR_GENERAL = 200, /**< 一般错误 */
    FT_ERR_ARGS = 202, /**< 参数错误 */
    FT_ERR_TIMEOUT = 204, /**< 超时 */
    FT_ERR_IOERROR = 300, /**< IO错误 */
    FT_ERR_CUSTOM_BEGIN = 400, /**< 自定义错误，从400开始拓展 */
} FeatureErrorCode;

/** union for AppendData */
typedef union AppendData {
    int32_t i32; /**< 32位整数 */
    int64_t i64; /**< 64位整数 */
    uint32_t u32; /**< 32位无符号整数 */
    uint64_t u64; /**< 64位无符号整数 */
    float f32; /**< 32位浮点数 */
    double f64; /**< 64位浮点数 */
    void* ptr; /**< 指针 */
    const char* str; /**< 字符串 */
} AppendData;

/** Feature Array struct defination */
typedef struct FtArray {
    int32_t _size; /**< 当前数组实际size大小 */
    void* _element; /**< 数组成员指针 */
} FtArray;

/** variadic parameters packet */
typedef struct FtVariParams {
    int32_t vari_count; /**< 参数数量 */
    ft_value_t* vari_args; /**< 参数指针 */
} FtVariParams;

#ifdef __cplusplus
}
#endif

#endif // FEATURE_TYPES_H
