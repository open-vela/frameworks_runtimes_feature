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
#ifndef FEATURE_EXPORT_H
#define FEATURE_EXPORT_H

#include <cstdint>
#include <inttypes.h>
#include <stdlib.h>
#include <type_traits>
#include "feature_context.h"

#define FT_COMPLEX_BIT ((uintptr_t)1 << ((sizeof(uintptr_t) * 8 - 1)))
#define FT_REFERENCE_BIT ((uintptr_t)1 << ((sizeof(uintptr_t) * 8 - 2)))
#define FT_VALUE_MASK (~(FT_COMPLEX_BIT | FT_REFERENCE_BIT))

#define FT_IS_PRIMITIVE(type) (((((uintptr_t)type) >> ((sizeof(uintptr_t) * 8 - 1))) & 1) == 0)
#define FT_IS_COMPLEX(type) (((((uintptr_t)type) >> ((sizeof(uintptr_t) * 8 - 1))) & 1) == 1)
// check if is reference
#define FT_IS_REFERENCE(type) (((((uintptr_t)type) >> ((sizeof(uintptr_t) * 8 - 2))) & 1) == 1)
#define FT_SET_REFERENCE(type) ((uintptr_t)(type) | FT_REFERENCE_BIT)
#define FT_REMOVE_REFERENCE(type) ((uintptr_t)(type) & ~FT_REFERENCE_BIT)

#define FT_IS_REST FT_IS_REFERENCE

#define FT_GET_VALUE(type) (((uintptr_t)type) & FT_VALUE_MASK)
#define FT_MK_COMPLEX(ptr) ((uintptr_t)((((uintptr_t)ptr) >> 2) | FT_COMPLEX_BIT))
#define FT_MK_COMPLEX_REF(ptr) (FT_SET_REFERENCE(FT_MK_COMPLEX(ptr)))

#define FT_MK_OPTIONAL(ptr) FT_MK_COMPLEX(ptr)

#define FT_PARAM_REST_END ((uintptr_t)(0) | FT_REFERENCE_BIT)
#define FT_PARAM_END (0)
#define FT_GET_COMPLEX(ptr) (((uintptr_t)ptr) << 2)

#define FT_IS_PROMISE(ptr)    (FT_IS_COMPLEX((ptr)) && ((ComplexTypeHeader*)FT_GET_COMPLEX((ptr)))->type == COMPLEX_PROMISE)

namespace FEATURE {

typedef void* FeatureRuntimeContext; // guest runtime context, e.g qucikjs RuntimeContext
typedef void* FeatureProtoHandle; // feature prototype handle.
typedef void* FeatureInstanceHandle; // feature instance handle.
typedef uintptr_t FeatureType; // feature type flag
typedef int32_t FeatureCallbackId; // feature callback id
typedef int32_t FeaturePromiseHandle; // feature promise handle

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
typedef char FtBool;
typedef const char* FtString;

struct FTArray {
    int32_t _size;
    void* _element;
};

/**
 * @brief variadic parameters packet
 *
 */
struct FtVariadicParameters {
    int32_t variadic_count; // variadic parameter count
    ft_value_t* variadic_args; // variadic parameter pointer array
};

struct FeatureCallbacks {
    void (*onRegister)(FeatureRuntimeContext ctx); // 插件注册
    void (*onCreate)(FeatureRuntimeContext ctx, FeatureProtoHandle handle); // 插件原型创建
    void (*onRequired)(FeatureRuntimeContext ctx, FeatureInstanceHandle handle); // 插件实例化
    void (*onDetached)(FeatureRuntimeContext ctx, FeatureInstanceHandle handle); // 插件实例销毁
    void (*onDestroy)(FeatureRuntimeContext ctx, FeatureProtoHandle handle); // 插件原型销毁
    void (*onUnregister)(FeatureRuntimeContext ctx); // 插件反注册
};

/**
 * @brief dump feature value, add ref_count.
 *      ptr must be allocated using FTMalloc
 *
 * @param ptr
 */
void DupFeatureValue(void* ptr);

/**
 * @brief free feature value, decrease ref_count
 *
 *      ptr must be allocated using FTMalloc
 * @param ptr
 */
void FreeFeatureValue(void* ptr);

/**
 * @brief get the native object pointer bind to feature proto(global object for all feature instance)
 *
 * @param handle
 * @return void*
 */
void* GetFeatureProtoData(FeatureProtoHandle handle);

/**
 * @brief Set the Feature Proto Data object
 *
 * @param handle
 * @param data
 */
void SetFeatureProtoData(FeatureProtoHandle handle, void* data);

/**
 * @brief get the native object pointer bind to feature instance
 *
 * @param handle
 * @return void*
 */
void* GetFeatureObjectData(FeatureInstanceHandle handle);

/**
 * @brief Set the Feature Object Data object
 *
 * @param handle
 * @param data
 */
void SetFeatureObjectData(FeatureInstanceHandle handle, void* data);

/**
 * @brief get feature context from FeatureInstanceHandle, feature context is guest context.
 *
 * @param handle
 * @return context_ref
 */
ft_context_ref GetFeatureContext(FeatureInstanceHandle handle);

/**
 * @brief invoke callback via cid
 *
 * @param handle
 * @param cid
 * @param ...
 * @return int 0: success, 1: failed
 */
// int InvokeFeatureCallback(FeatureRuntimeContext ctx, FeatureInstanceHandle handle, void** ret_value, int cid, ...);
int InvokeFeatureCallback(FeatureInstanceHandle handle, int cid, ...);

/**
 * @brief invoke callback via cid with variadic parameter
 *
 * @param handle
 * @param ret_value return value from callback, must free manually.
 * @param cid
 * @param count
 * @param ...
 * @return int
 */
// int InvokeFeatureCallbackCount(FeatureRuntimeContext ctx, FeatureInstanceHandle handle, void** ret_value, int cid, int count, ...);
int InvokeFeatureCallbackCount(FeatureInstanceHandle handle, int cid, int count, ...);

/**
 * @brief remove callback from instance via cid.
 *
 * @param handle
 * @param id
 * @return true
 * @return false
 */
bool RemoveCallback(FeatureInstanceHandle handle, FeatureCallbackId id);

/**
 * @brief promise resolve, only support one param
 *
 * @param handle
 * @param promiseHandle
 * @param ...
 * @return int
 */
int FeaturePromiseResolve(FeatureInstanceHandle handle, FeaturePromiseHandle promiseHandle, ...);
/**
 * @brief promise reject，only support one param
 *
 * @param handle
 * @param promiseHandle
 * @param ...
 * @return int
 */
int FeaturePromiseReject(FeatureInstanceHandle handle, FeaturePromiseHandle promiseHandle, ...);

}

using namespace FEATURE;

namespace ferry {
/**
 * @brief the management header
 *
 */
typedef struct FTObjHeader {
    int32_t ref_count;
    FEATURE::FeatureType featureType;
} FTObjHeader;

#define FT_OBJ_HEADER_SIZE sizeof(ferry::FTObjHeader)
#define FT_IS_MANAGEMENT_OBJ(ptr) ((uintptr_t)ptr & 0x1)

inline void* FT_GET_OBJ(void* ptr)
{
    if (FT_IS_MANAGEMENT_OBJ(ptr)) {
        return (char*)ptr + FT_OBJ_HEADER_SIZE;
    }
    return nullptr;
}

enum MemberType {
    MEMBER_NULL, //代表结束，定义为0
    MEMBER_METHOD,
    MEMBER_ACCESSOR,
    MEMBER_CONST
};

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
    FT_PRIMITIVE_END = FT_REFERENCE_BIT - 1,
    FT_POINTER, // pointer
    FT_STRING = FT_REFERENCE_BIT | FT_CHAR, // string
    FT_ANY, // any means guest value
    // FT_OBJECT,
    // FT_ARRY, // fixed array
    // FT_ARRAY_INT,
    // FT_ARRAY_FLOAT,
    // FT_ARRAY_BOOLEAN,
    // FT_ARRAY_STRING,
    // FT_ARRAY_OBJECT,
    // FT_ARRAY_ANY,
};

inline bool isPrimitiveType(FEATURE::FeatureType type)
{
    return type < FT_PRIMITIVE_END;
}

enum ComplexType {
    COMPLEX_STRUCT_MAP = 1, // object map
    COMPLEX_OPTIONAL, // optional value
    COMPLEX_CALLBACK, // callback object
    COMPLEX_ARRAY, // array
    COMPLEX_PROMISE, // promise
};

typedef struct ObjectMember {
    const char* name;
    const FEATURE::FeatureType type;
    int offset; // 在对象中的偏移
    int size; //所占空间大小
} ObjectMember;

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

typedef struct MemberMethod {
    void (*callback)(void); // 最终实现函数
    const FEATURE::FeatureType* parameters; // 参数描述数组, 以空结束
    FEATURE::FeatureType return_type;
    AppendData data; //附加数据
} MemberMethod;

typedef struct MemberAccessor {
    void (*getter)(void); // getter & setter可以有一个为空
    void (*setter)(void);
    FEATURE::FeatureType type;
    AppendData data; //附加数据
} MemberAccessor;

typedef struct MemberConst {
    FEATURE::FeatureType type;
    void (*callback)(void); // initializer callback.
    AppendData data; // 定义的数据, 如果callback != null, 那么data将传递给callback
} MemberConst;

typedef struct Member {
    MemberType type;
    const char* name; // member的名称
    union {
        MemberMethod method;
        MemberAccessor accessor;
        MemberConst value;
    };
} Member;

typedef struct ComplexTypeHeader {
    ComplexType type; // type值必须在最前面
    size_t size; /* Note: 此处是实际对象大小，并不是Complex对象大小，例如CallbackType里描述了回调信息，但实际的callback只是一个cid */
} ComplexTypeHeader;

typedef struct OptionalType {
    ComplexTypeHeader header;
    FEATURE::FeatureType type; // exact type
    union {
        int32_t ival;
        int64_t lval;
        double fval;
        const char* str;
        void* ptr;
    };
} OptionalType;

typedef struct ObjectMapType {
    ComplexTypeHeader header;
    ObjectMember* members;
} ObjectMapType;

typedef struct StringType {
    ComplexTypeHeader header;
    uint32_t length;
    char* data;
} StringType;

typedef struct CallbackType {
    ComplexTypeHeader header;
    const FEATURE::FeatureType* parameters; // parameters type array, null terminated
    FEATURE::FeatureType return_type; // return type
} CallbackType;

typedef struct ArrayType {
    ComplexTypeHeader header;
    FEATURE::FeatureType element_type;
} ArrayType;

/***/
typedef struct PromiseType {
    ComplexTypeHeader header;
    const FEATURE::FeatureType resolveTypes[2];
} PromiseType;

typedef struct FeatureDescription {
    int version; // 待后面扩展使用. 目前可以统一为1
    const char* name; // feature名字, 在require时提供的
    const char* description; // feature的描述, 可以为null
    int flags; // 配置信息, 暂时不用
    const FEATURE::FeatureCallbacks* native_callbacks; // native对象接口
    int member_count; // 成员数量
    const Member* members; //定义成员数量, 后面详细介绍
} FeatureDescription;

}
#endif
