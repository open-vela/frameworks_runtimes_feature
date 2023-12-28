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

#define FT_REFERENCE_BIT ((uintptr_t)2)

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

typedef void (*FeatureTaskCallback)(int status, void* data);

typedef struct VTable {
    int size;
    NativeFunc finalizer;
    const NativeFunc* members;
} VTable;

#define FT_REMOVE_REFERENCE(type) ((uintptr_t)(type) & ~FT_REFERENCE_BIT)

enum FeaturePrimitiveTypeIndex {
    FT_VOID_INDEX = 0,
    FT_INT_INDEX,
    FT_INT8_INDEX,
    FT_UINT8_INDEX,
    FT_INT16_INDEX,
    FT_UINT16_INDEX,
    FT_INT32_INDEX,
    FT_UINT32_INDEX,
    FT_INT64_INDEX,
    FT_UINT64_INDEX,
    FT_FLOAT_INDEX,
    FT_DOUBLE_INDEX,
    FT_BOOLEAN_INDEX,
    FT_CHAR_INDEX,
    FT_ANY_INDEX,
    FT_POINTER_INDEX = (0x800), // pointer index
    FT_RAWPOINTER_INDEX, // raw pointer index
};
//
#define FT_SET_PRIMITIVE_TYPE(index) ((index << 2) | 3)

enum FeaturePrimitiveType {
    FT_VOID = FT_SET_PRIMITIVE_TYPE(FT_VOID_INDEX), // void defination
    FT_INT = FT_SET_PRIMITIVE_TYPE(FT_INT_INDEX), 
    FT_INT8 = FT_SET_PRIMITIVE_TYPE(FT_INT8_INDEX),
    FT_UINT8 = FT_SET_PRIMITIVE_TYPE(FT_UINT8_INDEX),
    FT_INT16 = FT_SET_PRIMITIVE_TYPE(FT_INT16_INDEX),
    FT_UINT16 = FT_SET_PRIMITIVE_TYPE(FT_UINT16_INDEX),
    FT_INT32 = FT_SET_PRIMITIVE_TYPE(FT_INT32_INDEX),
    FT_UINT32 = FT_SET_PRIMITIVE_TYPE(FT_UINT32_INDEX),
    FT_INT64 = FT_SET_PRIMITIVE_TYPE(FT_INT64_INDEX),
    FT_UINT64 = FT_SET_PRIMITIVE_TYPE(FT_UINT64_INDEX),
    FT_FLOAT = FT_SET_PRIMITIVE_TYPE(FT_FLOAT_INDEX),
    FT_DOUBLE = FT_SET_PRIMITIVE_TYPE(FT_DOUBLE_INDEX),
    FT_BOOLEAN = FT_SET_PRIMITIVE_TYPE(FT_BOOLEAN_INDEX),
    FT_CHAR = FT_SET_PRIMITIVE_TYPE(FT_CHAR_INDEX), // char
    FT_ANY = FT_SET_PRIMITIVE_TYPE(FT_ANY_INDEX), // any means guest value
    FT_POINTER = FT_REMOVE_REFERENCE(FT_SET_PRIMITIVE_TYPE(FT_POINTER_INDEX)), // pointer
    FT_RAWPOINTER = FT_REMOVE_REFERENCE((FT_SET_PRIMITIVE_TYPE(FT_RAWPOINTER_INDEX))), // raw pointer
    FT_STRING = (FT_REMOVE_REFERENCE(FT_CHAR)), // string
    FT_ANY_REF = (FT_REMOVE_REFERENCE(FT_ANY)), // any ref
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
