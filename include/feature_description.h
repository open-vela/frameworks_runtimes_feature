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

#ifndef FEATURE_DESCRIPTION_H
#define FEATURE_DESCRIPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "feature_types.h"
#include <cstdint>
#include <inttypes.h>
#include <stdlib.h>

#define FT_COMPLEX_BIT ((uintptr_t)1 << ((sizeof(uintptr_t) * 8 - 1)))
#define FT_VALUE_MASK (~(FT_COMPLEX_BIT | FT_REFERENCE_BIT))

#define FT_IS_PRIMITIVE(type) (((((uintptr_t)type) >> ((sizeof(uintptr_t) * 8 - 1))) & 1) == 0)
#define FT_IS_COMPLEX(type) (((((uintptr_t)type) >> ((sizeof(uintptr_t) * 8 - 1))) & 1) == 1)
// check if is reference
#define FT_IS_REFERENCE(type) (((((uintptr_t)type) >> ((sizeof(uintptr_t) * 8 - 2))) & 1) == 1)
#define FT_SET_REFERENCE(type) ((uintptr_t)(type) | FT_REFERENCE_BIT)
#define FT_REMOVE_REFERENCE(type) ((uintptr_t)(type) & ~FT_REFERENCE_BIT)

#define FT_GET_VALUE(type) (((uintptr_t)type) & FT_VALUE_MASK)
#define FT_MK_COMPLEX(ptr) ((uintptr_t)((((uintptr_t)ptr) >> 2) | FT_COMPLEX_BIT))
#define FT_MK_COMPLEX_REF(ptr) (FT_SET_REFERENCE(FT_MK_COMPLEX(ptr)))

#define FT_MK_OPTIONAL(ptr) FT_MK_COMPLEX(ptr)

#define FT_PARAM_REST_END ((uintptr_t)(0) | FT_REFERENCE_BIT)
#define FT_PARAM_END (0)
#define FT_GET_COMPLEX(ptr) (((uintptr_t)ptr) << 2)

#define FT_IS_PROMISE(ptr) (FT_IS_COMPLEX((ptr)) && ((ComplexTypeHeader*)FT_GET_COMPLEX((ptr)))->type == COMPLEX_PROMISE)

typedef struct FTObjHeader {
    int32_t ref_count;
    FeatureType featureType;
} FTObjHeader;

#define FT_OBJ_HEADER_SIZE sizeof(FTObjHeader)
#define FT_IS_MANAGEMENT_OBJ(ptr) ((uintptr_t)ptr & 0x1)

inline void* FT_GET_OBJ(void* ptr)
{
    if (FT_IS_MANAGEMENT_OBJ(ptr)) {
        return (char*)ptr + FT_OBJ_HEADER_SIZE;
    }
    return nullptr;
}

enum MemberType {
    MEMBER_NULL, // 代表结束，定义为0
    MEMBER_METHOD,
    MEMBER_ACCESSOR,
    MEMBER_CONST
};

inline bool isPrimitiveType(FeatureType type)
{
    return type < FT_PRIMITIVE_END;
}

enum ComplexType {
    COMPLEX_STRUCT_MAP = 1, // object map
    COMPLEX_OPTIONAL, // optional value
    COMPLEX_CALLBACK, // callback object
    COMPLEX_ARRAY, // array
    COMPLEX_PROMISE, // promise
    COMPLEX_INTERFACE, // interface
};

union FuncData {
    NativeFunc callback; // 最终实现函数
    int32_t vtable_idx; // vtable index
};

typedef struct ObjectMember {
    const char* name;
    const FeatureType type;
    int offset; // 在对象中的偏移
    int size; // 所占空间大小
} ObjectMember;

typedef struct MemberMethod {
    FuncData func;
    const FeatureType* parameters; // 参数描述数组, 以空结束
    FeatureType return_type;
    AppendData data; // 附加数据
} MemberMethod;

typedef struct MemberAccessor {
    FuncData getter; // getter & setter可以有一个为空
    FuncData setter;
    FeatureType type;
    AppendData data; // 附加数据
} MemberAccessor;

typedef struct MemberConst {
    FeatureType type;
    FuncData func;
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
    FeatureType type; // exact type
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
    const FeatureType* parameters; // parameters type array, null terminated
    FeatureType return_type; // return type
} CallbackType;

typedef struct ArrayType {
    ComplexTypeHeader header;
    FeatureType element_type;
} ArrayType;

typedef struct PromiseType {
    ComplexTypeHeader header;
    const FeatureType resolveTypes[2];
} PromiseType;

typedef struct InterfaceType {
    ComplexTypeHeader header;
    const struct FeatureDescription* desc;
} InterfaceType;

typedef struct FeatureCallbacks {
    void (*onRegister)(const char* feature_name); // 插件注册
    void (*onCreate)(FeatureRuntimeContext ctx, FeatureProtoHandle handle); // 插件原型创建
    void (*onRequired)(FeatureRuntimeContext ctx, FeatureInstanceHandle handle); // 插件实例化
    void (*onDetached)(FeatureRuntimeContext ctx, FeatureInstanceHandle handle); // 插件实例销毁
    void (*onDestroy)(FeatureRuntimeContext ctx, FeatureProtoHandle handle); // 插件原型销毁
    void (*onUnregister)(const char* feature_name); // 插件反注册
} FeatureCallbacks;

typedef struct FeatureDescription {
    int version; // 待后面扩展使用. 目前可以统一为1
    const char* name; // feature名字, 在require时提供的
    const char* description; // feature的描述, 可以为null
    union {
        int flags; // 配置信息，通过位域定义
        struct {
            bool dynamic : 1; // if dynamic type
        };
    };
    const FeatureCallbacks* native_callbacks; // native对象接口
    int member_count; // 成员数量
    const Member* members; // 定义成员数量, 后面详细介绍
} FeatureDescription;

#ifdef __cplusplus
}
#endif

#endif // FEATURE_DESCRIPTION_H
