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
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#define FT_PRIMITIVE_BIT ((uintptr_t)3)

#define FT_IS_PRIMITIVE(type) (((uintptr_t)type) & FT_PRIMITIVE_BIT)
#define FT_IS_COMPLEX(type) ((((uintptr_t)type) & FT_PRIMITIVE_BIT) == 0)
#define FT_GET_FLAG(featureType) ((FT_IS_COMPLEX(featureType) ? (((ComplexTypeHeader*)FT_GET_COMPLEX(featureType))->type & TYPE_FLAGS_UNMANAGED_POINTER) : (featureType & TYPE_FLAGS_UNMANAGED_POINTER)))
#define FT_IS_REFERENCE(featureType) (FT_GET_FLAG(featureType) & TYPE_FLAGS_POINTER)
#define FT_NEED_FREE(featureType) (FT_GET_FLAG(featureType) == TYPE_FLAGS_POINTER)
#define FT_IS_RAW_REFERENCE(featureType) (FT_GET_FLAG(featureType) & TYPE_FLAGS_RAWPOINTER)

#define FT_MK_COMPLEX(ptr) ((uintptr_t)ptr)
#define FT_MK_COMPLEX_REF(ptr) FT_MK_COMPLEX(ptr)
#define FT_MK_OPTIONAL(ptr) ((uintptr_t)ptr)

// FT_PARAM_REST_END set highest bit
#define FT_PARAM_REST_END ((uintptr_t)(((uintptr_t)1 << ((sizeof(uintptr_t) * 8 - 1)))))
#define FT_PARAM_END (0)
#define FT_GET_COMPLEX(ptr) ((uintptr_t)ptr)

#define FT_IS_PROMISE(ptr) (FT_IS_COMPLEX((ptr)) && ((ComplexTypeHeader*)FT_GET_COMPLEX((ptr)))->type == COMPLEX_PROMISE)

typedef struct FTObjHeader {
    int32_t ref_count;
    FeatureType featureType;
} FTObjHeader;

#define FT_OBJ_HEADER_SIZE sizeof(FTObjHeader)

enum MemberType {
    MEMBER_NULL, // 代表结束，定义为0
    MEMBER_METHOD,
    MEMBER_ACCESSOR,
    MEMBER_CONST
};

enum ComplexTypeBase {
    COMPLEX_STRUCT_MAP_BASE = 1,
    COMPLEX_OPTIONAL_BASE,
    COMPLEX_CALLBACK_BASE,
    COMPLEX_ARRAY_BASE,
    COMPLEX_PROMISE_BASE,
    COMPLEX_INTERFACE_BASE,
};

#define DEF_COMPLEX_TYPE(base, flags) ((base##_BASE) << 2 | flags)
enum ComplexType {
    COMPLEX_STRUCT_MAP = DEF_COMPLEX_TYPE(COMPLEX_STRUCT_MAP, TYPE_FLAGS_POINTER), // object map
    COMPLEX_OPTIONAL = DEF_COMPLEX_TYPE(COMPLEX_OPTIONAL, TYPE_FLAGS_VALUE), // optional value
    COMPLEX_CALLBACK = DEF_COMPLEX_TYPE(COMPLEX_CALLBACK, TYPE_FLAGS_VALUE), // callback object
    COMPLEX_ARRAY = DEF_COMPLEX_TYPE(COMPLEX_ARRAY, TYPE_FLAGS_POINTER), // array
    COMPLEX_PROMISE = DEF_COMPLEX_TYPE(COMPLEX_PROMISE, TYPE_FLAGS_UNMANAGED_POINTER), // promise
    COMPLEX_INTERFACE = DEF_COMPLEX_TYPE(COMPLEX_INTERFACE, TYPE_FLAGS_UNMANAGED_POINTER), // interface
};
#undef DEF_COMPLEX_TYPE

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
    union FuncData func;
    const FeatureType* parameters; // 参数描述数组, 以空结束
    FeatureType return_type;
    union AppendData data; // 附加数据
} MemberMethod;

typedef struct MemberAccessor {
    union FuncData getter; // getter & setter可以有一个为空
    union FuncData setter;
    FeatureType type;
    union AppendData data; // 附加数据
} MemberAccessor;

typedef struct MemberConst {
    FeatureType type;
    union FuncData func;
    union AppendData data; // 定义的数据, 如果callback != null, 那么data将传递给callback
} MemberConst;

typedef struct Member {
    enum MemberType type;
    const char* name; // member的名称
    union {
        const MemberMethod* method;
        const MemberAccessor* accessor;
        const MemberConst* value;
    };
} Member;

typedef struct ComplexTypeHeader {
    enum ComplexType type; // type值必须在最前面
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

/**
 * @brief register feature to feature registry
 *
 * @param handle
 * @param description
 * @return bool
 */
bool FeatureRegisterFeature(FeatureRegistryHandle handle, const FeatureDescription* description);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_DESCRIPTION_H
