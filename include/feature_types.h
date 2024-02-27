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

#ifndef FEATURE_TYPES_H
#define FEATURE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "feature_context.h"
#include <stdbool.h>

// primitive type definations
typedef int FtInt;
typedef int8_t FtInt8;
typedef uint8_t FtUint8;
typedef int16_t FtInt16;
typedef uint16_t FtUint16;
typedef int32_t FtInt32;
typedef uint32_t FtUint32;
typedef int64_t FtInt64;
typedef uint64_t FtUint64;
typedef float FtFloat;
typedef double FtDouble;
typedef bool FtBool;
typedef const char* FtString;
typedef ft_value_t* FtAny;
typedef int32_t FtCallbackId; // feature callback id
typedef int32_t FtPromiseId; // feature promise id

typedef void* FeatureRuntimeContext; // guest runtime context, e.g qucikjs RuntimeContext

typedef void* FeatureRegistryHandle; // feature registry handle.
typedef void* FeatureManagerHandle; // feature manage handle.
typedef void* FeatureProtoHandle; // feature prototype handle.
typedef void* FeatureInstanceHandle; // feature instance handle.
typedef void* FeatureInterfaceHandle; // feature interface handle.

typedef uintptr_t FeatureType; // feature type flag
typedef void (*NativeFunc)(void);

enum FeatureTaskMode {
    FEATURE_TASK_MODE_FREE = 0,
    FEATURE_TASK_MODE_NORMAL = 1,
};

enum TypeFlags {
    TYPE_FLAGS_VALUE = 1, // value
    TYPE_FLAGS_POINTER, // pointer, need malloc/free
    TYPE_FLAGS_UNMANAGED_POINTER = TYPE_FLAGS_POINTER | 1, // unmanaged pointer, do not malloc/free
};

typedef void (*FeatureTaskCallback)(int status, void* data);

typedef struct VTable {
    int size;
    NativeFunc finalizer;
    const NativeFunc* members;
} VTable;

enum FeaturePrimitiveTypeBase {
    FT_VOID_BASE = 0,
    FT_INT_BASE,
    FT_INT8_BASE,
    FT_UINT8_BASE,
    FT_INT16_BASE,
    FT_UINT16_BASE,
    FT_INT32_BASE,
    FT_UINT32_BASE,
    FT_INT64_BASE,
    FT_UINT64_BASE,
    FT_FLOAT_BASE,
    FT_DOUBLE_BASE,
    FT_BOOLEAN_BASE,
    FT_STRING_BASE,
    FT_ANY_REF_BASE,
};
//
#define FT_SET_PRIMITIVE_TYPE(base, flags) ((base << 2) | (flags))

enum FeaturePrimitiveType {
    FT_VOID = FT_SET_PRIMITIVE_TYPE(FT_VOID_BASE, TYPE_FLAGS_VALUE), // void defination
    FT_INT = FT_SET_PRIMITIVE_TYPE(FT_INT_BASE, TYPE_FLAGS_VALUE),
    FT_INT8 = FT_SET_PRIMITIVE_TYPE(FT_INT8_BASE, TYPE_FLAGS_VALUE),
    FT_UINT8 = FT_SET_PRIMITIVE_TYPE(FT_UINT8_BASE, TYPE_FLAGS_VALUE),
    FT_INT16 = FT_SET_PRIMITIVE_TYPE(FT_INT16_BASE, TYPE_FLAGS_VALUE),
    FT_UINT16 = FT_SET_PRIMITIVE_TYPE(FT_UINT16_BASE, TYPE_FLAGS_VALUE),
    FT_INT32 = FT_SET_PRIMITIVE_TYPE(FT_INT32_BASE, TYPE_FLAGS_VALUE),
    FT_UINT32 = FT_SET_PRIMITIVE_TYPE(FT_UINT32_BASE, TYPE_FLAGS_VALUE),
    FT_INT64 = FT_SET_PRIMITIVE_TYPE(FT_INT64_BASE, TYPE_FLAGS_VALUE),
    FT_UINT64 = FT_SET_PRIMITIVE_TYPE(FT_UINT64_BASE, TYPE_FLAGS_VALUE),
    FT_FLOAT = FT_SET_PRIMITIVE_TYPE(FT_FLOAT_BASE, TYPE_FLAGS_VALUE),
    FT_DOUBLE = FT_SET_PRIMITIVE_TYPE(FT_DOUBLE_BASE, TYPE_FLAGS_VALUE),
    FT_BOOLEAN = FT_SET_PRIMITIVE_TYPE(FT_BOOLEAN_BASE, TYPE_FLAGS_VALUE),
    FT_STRING = FT_SET_PRIMITIVE_TYPE(FT_STRING_BASE, TYPE_FLAGS_POINTER),
    FT_CHAR = FT_STRING,
    FT_ANY_REF = FT_SET_PRIMITIVE_TYPE(FT_ANY_REF_BASE, TYPE_FLAGS_POINTER),
};

union AppendData {
    int32_t i32;
    int64_t i64;
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
    void* ptr;
    const char* str;
};

/**
 * @brief Feature Array struct defination
 *
 */
typedef struct FtArray {
    int32_t _size;
    void* _element;
} FtArray;

/**
 * @brief variadic parameters packet
 *
 */
typedef struct FtVariParams {
    int32_t vari_count; // variadic parameter count
    ft_value_t* vari_args; // variadic parameter pointer array
} FtVariParams;

#ifdef __cplusplus
}
#endif

#endif // FEATURE_TYPES_H
