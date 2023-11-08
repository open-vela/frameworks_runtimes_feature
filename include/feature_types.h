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

#define FT_REFERENCE_BIT ((uintptr_t)1 << ((sizeof(uintptr_t) * 8 - 2)))

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

typedef void (*FeatureTaskCallback)(int status, void* data);

typedef struct VTable {
    size_t size;
    NativeFunc finalizer;
    const NativeFunc* members;
} VTable;

enum FeaturePrimitiveType {
    FT_VOID = 0, // void defination
    FT_INT,
    FT_INT8,
    FT_UINT8,
    FT_INT16,
    FT_UINT16,
    FT_INT32,
    FT_UINT32,
    FT_INT64,
    FT_UINT64,
    FT_FLOAT,
    FT_DOUBLE,
    FT_BOOLEAN,
    FT_CHAR, // char
    FT_ANY, // any means guest value
    FT_PRIMITIVE_END = FT_REFERENCE_BIT - 1,
    FT_POINTER, // pointer
    FT_RAWPOINTER, // raw pointer point to a native C struct which has no ref
                   // count header
    FT_STRING = FT_REFERENCE_BIT | FT_CHAR, // string
    FT_ANY_REF = FT_REFERENCE_BIT | FT_ANY, // any ref
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
