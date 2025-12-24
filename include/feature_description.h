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

#include "feature_permission.h"
#include "feature_types.h"
#include <inttypes.h>
#include <protobuf-c/protobuf-c.h>
#ifdef __cplusplus
#include <atomic>
#else
#include <stdatomic.h>
#endif
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FT_PRIMITIVE_BIT ((uintptr_t)3)

#define FT_IS_PRIMITIVE(type) (((uintptr_t)type) & FT_PRIMITIVE_BIT)
#define FT_IS_COMPLEX(type) ((((uintptr_t)type) & FT_PRIMITIVE_BIT) == 0)

#define FT_GET_COMPLEX_ENUM(ftype) (((ComplexTypeHeader*)FT_GET_COMPLEX(ftype))->type)
#define FT_GET_REAL_TYPE(ftype) ((FT_IS_COMPLEX(ftype) && FT_GET_COMPLEX_ENUM(ftype) == COMPLEX_OPTIONAL) ? ((OptionalType*)FT_GET_COMPLEX(ftype))->type : ftype)
#define FT_GET_TYPE_ENUM(ftype) (FT_IS_COMPLEX(ftype) ? FT_GET_COMPLEX_ENUM(ftype) : ftype)
#define FT_GET_FLAG(ftype) (FT_GET_TYPE_ENUM(FT_GET_REAL_TYPE(ftype)) & TYPE_FLAGS_UNMANAGED_POINTER)
#define FT_IS_REFERENCE(ftype) (FT_GET_FLAG(ftype) & TYPE_FLAGS_POINTER)
#define FT_NEED_FREE(ftype) (FT_GET_FLAG(ftype) == TYPE_FLAGS_POINTER)
#define FT_IS_RAW_REFERENCE(ftype) (FT_GET_FLAG(ftype) & TYPE_FLAGS_RAWPOINTER)

#define FT_MK_COMPLEX(ptr) ((uintptr_t)ptr)
#define FT_MK_COMPLEX_REF(ptr) FT_MK_COMPLEX(ptr)
#define FT_MK_OPTIONAL(ptr) ((uintptr_t)ptr)

// FT_PARAM_REST_END set highest bit
#define FT_PARAM_REST_END ((uintptr_t)(((uintptr_t)1 << ((sizeof(uintptr_t) * 8 - 1)))))
#define FT_PARAM_END (0)
#define FT_GET_COMPLEX(ptr) ((uintptr_t)ptr)

#define FT_IS_PROMISE(ptr) (FT_IS_COMPLEX((ptr)) && ((ComplexTypeHeader*)FT_GET_COMPLEX((ptr)))->type == COMPLEX_PROMISE)
#define FT_IS_PROTOBUF_MESSAGE(ptr) (FT_IS_COMPLEX(ptr) && (((ComplexTypeHeader*)FT_GET_COMPLEX(ptr))->type == COMPLEX_PROTOBUF))

typedef void (*StubFunc)(FeatureInterfaceHandle handle, AppendData adata, void** argv, int argc, void* ret);

#define FT_GET_REF_COUNT(ptr) ((FTObjHeader*)ptr)->ref_count

//
#define MEMORY_REF_COUNT_ONLY 0
#define MEMORY_FEATURE_TYPE 1
#define MEMORY_PROTOBUF 1 << 1
int FeatureTypeGetValueSize(FeatureType ft);

typedef struct FTObjHeader {
    uint8_t type;
#ifdef __cplusplus
    std::atomic_uint ref_count;
#else
    atomic_uint ref_count;
#endif
#ifdef ENABLE_FEATURE_MEM_TRACE
    const FeatureDescription* desc; // 只能记录desc, 保证指针一直有效
#endif
} FTObjHeader;

typedef struct FTMemory {
    FTObjHeader header;
    char payload[0];
} FTMemory;

#define FT_OBJ_HEADER_SIZE sizeof(FTObjHeader)

enum MemberType {
    MEMBER_NULL, // 代表结束，定义为0
    MEMBER_METHOD,
    MEMBER_ACCESSOR,
    MEMBER_CONST,
    MEMBER_EVENT
};

enum ComplexTypeBase {
    COMPLEX_STRUCT_MAP_BASE = 1,
    COMPLEX_OPTIONAL_BASE,
    COMPLEX_CALLBACK_BASE,
    COMPLEX_ARRAY_BASE,
    COMPLEX_PROMISE_BASE,
    COMPLEX_INTERFACE_BASE,
    COMPLEX_PROTOBUF_BASE,
};

#define DEF_COMPLEX_TYPE(base, flags) ((base##_BASE) << 2 | flags)
enum ComplexType {
    COMPLEX_STRUCT_MAP = DEF_COMPLEX_TYPE(COMPLEX_STRUCT_MAP, TYPE_FLAGS_POINTER), // object map
    COMPLEX_OPTIONAL = DEF_COMPLEX_TYPE(COMPLEX_OPTIONAL, TYPE_FLAGS_VALUE), // optional value
    COMPLEX_CALLBACK = DEF_COMPLEX_TYPE(COMPLEX_CALLBACK, TYPE_FLAGS_VALUE), // callback object
    COMPLEX_ARRAY = DEF_COMPLEX_TYPE(COMPLEX_ARRAY, TYPE_FLAGS_POINTER), // array
    COMPLEX_PROMISE = DEF_COMPLEX_TYPE(COMPLEX_PROMISE, TYPE_FLAGS_UNMANAGED_POINTER), // promise
    COMPLEX_INTERFACE = DEF_COMPLEX_TYPE(COMPLEX_INTERFACE, TYPE_FLAGS_UNMANAGED_POINTER), // interface
    COMPLEX_PROTOBUF = DEF_COMPLEX_TYPE(COMPLEX_PROTOBUF, TYPE_FLAGS_POINTER), // protobuf message
};
#undef DEF_COMPLEX_TYPE

typedef union {
    NativeFunc callback; // 最终实现函数
    int32_t vtable_idx; // vtable index
} FuncData;

typedef struct ObjectMember {
    const char* name;
    const FeatureType type;
    int offset; // 在对象中的偏移
    int size; // 所占空间大小
} ObjectMember;

typedef struct MemberMethod {
    StubFunc func_stub;
    const FeatureType* parameters; // 参数描述数组, 以空结束
    FeatureType return_type;
    AppendData data; // 附加数据
} MemberMethod;

typedef struct MemberAccessor {
    StubFunc getter_stub;
    StubFunc setter_stub;
    FeatureType type;
    AppendData data; // 附加数据
} MemberAccessor;

typedef struct MemberConst {
    FeatureType type;
    FuncData func;
    AppendData data; // 定义的数据, 如果callback != null, 那么data将传递给callback
} MemberConst;

typedef struct MemberEvent {
    const FeatureType* parameters;
    FtEventId id;
    const char* name;
    AppendData data; // 附加数据
} MemberEvent;

typedef struct Member {
    enum MemberType type;
    const char* name; // member的名称
    union {
        const MemberMethod* method;
        const MemberAccessor* accessor;
        const MemberConst* value;
        const MemberEvent* event;
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
        uint32_t uval;
        int64_t lval;
        uint64_t ulval;
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
    const FeatureType resolveType;
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

typedef struct ProtobufMessageType {
    ComplexTypeHeader header;
    const ProtobufCMessageDescriptor* desc;
} ProtobufMessageType;

/**
 * @brief register feature to feature registry
 *
 * @param handle
 * @param description
 * @return bool
 */
bool FeatureRegisterFeature(FeatureRegistryHandle handle, const FeatureDescription* description);

/**
 * @brief get registry from manager
 *
 * @param handle
 * @return registry
 */
FeatureRegistryHandle FeatureGetRegistryFromManager(FeatureManagerHandle handle);

/**
 * @brief get a member function from the FeatureInterfaceHandle
 *
 * @param handle
 * @param index
 * @return member ptr from vtable
 */
NativeFunc FeatureGetInterfaceMember(FeatureInstanceHandle handle, size_t index);

typedef void (*FeaturePermissionsRequestCb)(
    FeatureInterfaceHandle handle, AppendData adata, void** argv, int argc, void* ret);

typedef struct FeaturePermissionsRequestInfo {
    AppendData adata;
    const char* api_name;
    void** argv;
    int argc;
    const MemberMethod* method;
    const FeaturePermissions* permissions;
    FeaturePermissionsRequestCb cb;
} FeaturePermissionsRequestInfo;

bool FeatureRequestPermissions(FeatureInstanceHandle handle, FeaturePermissionsRequestInfo* info);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_DESCRIPTION_H
