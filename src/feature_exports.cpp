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

#include "feature_common.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_instance.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_main_exports.h"
#include "feature_manager.h"
#include "feature_manager_qjs.h"
#include "feature_prototype.h"
#include "feature_registry.h"
#include "feature_types.h"
#include "feature_utils.h"
#include "protobuf/proto_utils.h"
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <protobuf-c/protobuf-c.h>
#include <string.h>

#define CONFIG_FEATURE_FRAMEWORK_UTILS_TYPE_VALIDATE
using namespace feature_framework;

#define ARRAY_NEW_CAPACITY(size) ((size) + 5)

#define FEATURE_INSTANCE_CHECK(__instance_handle__, __ret__)                                \
    do {                                                                                    \
        if (!__instance_handle__) {                                                         \
            FEATURE_LOG_ERROR(#__instance_handle__ " is null !");                           \
            return __ret__;                                                                 \
        }                                                                                   \
        FeatureInstance* __instance__ = static_cast<FeatureInstance*>(__instance_handle__); \
        if (__instance__->isDetached()) {                                                   \
            FEATURE_LOG_ERROR(#__instance_handle__ " is detached !");                       \
            return __ret__;                                                                 \
        }                                                                                   \
    } while (0);

#define FEATURE_CHECK_PTR(__ptr__, __ret__, __log__) \
    do {                                             \
        if (!__ptr__) {                              \
            FEATURE_LOG_ERROR(__log__);              \
            return __ret__;                          \
        }                                            \
    } while (0);

static void* FeatureInstanceAllocTypeInternal(FeatureInstanceHandle handle, size_t size, FeatureType type, bool freeType);

// memory utils functions
char* FeatureStrCopy(FeatureInstanceHandle handle, const char* str)
{
    char* buf = static_cast<char*>(FeatureInstanceAllocType(handle, strlen(str) + 1, FT_STRING));
    strcpy(buf, str);
    return buf;
}

FtArray* FeatureCreateArray(FeatureInstanceHandle handle, size_t capacity, FeatureType element_type)
{

    ArrayType* array_type = static_cast<ArrayType*>(malloc(sizeof(ArrayType)));
    array_type->header.type = COMPLEX_ARRAY;
    array_type->header.size = sizeof(FtArray);
    array_type->element_type = element_type;

    FtArray* pArray = static_cast<FtArray*>(FeatureInstanceAllocTypeInternal(handle, sizeof(FtArray), FT_MK_COMPLEX(array_type), true));
    pArray->_capacity = capacity;
    pArray->_size = 0;
    pArray->_element = malloc(capacity * getValueSize(element_type));
    memset(pArray->_element, 0, capacity * getValueSize(element_type));
    return pArray;
}

FtArray* FeaturenArrayCopyRaw(FeatureInstanceHandle handle, FeatureType element_type, const void* data, size_t count)
{
    FtArray* pArray = nullptr;
    if (element_type == FT_STRING) {
        pArray = FeatureCreateArray(handle, count, element_type);
        for (size_t i = 0; i < count; i++) {
            ((char**)pArray->_element)[i] = FeatureStrCopy(handle, ((char**)data)[i]);
        }
        pArray->_size = count;
    } else {
        FEATURE_CHECK(false, "invalid element type !");
    }
    return pArray;
}

FtArray* FeaturenArrayCopy(FeatureInstanceHandle handle, FeatureType element_type, const void* data, size_t count)
{
    FtArray* pArray = FeatureCreateArray(handle, count, element_type);
    pArray->_size = count;
    pArray->_capacity = count;

    int elem_size = getValueSize(element_type);
    if (FT_IS_REFERENCE(element_type)) {
        for (size_t i = 0; i < count; ++count) {
            FeatureInstanceDupValue((void*)((uintptr_t)data + i * elem_size));
        }
    }

    memcpy(pArray->_element, data, count * elem_size);
    return pArray;
}

FeatureType getElementType(FtArray* arr)
{
    FTObjHeader* pHeader = (FTObjHeader*)((uintptr_t)arr - sizeof(FTObjHeader));
    FEATURE_CHECK_EQ(pHeader->type, MEMORY_FEATURE_TYPE);

    FeatureType featureType = *(FeatureType*)((uintptr_t)pHeader - sizeof(FeatureType));
    FEATURE_CHECK_EQ(FT_IS_COMPLEX(featureType), true);

    ComplexTypeHeader* complexType1 = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
    FEATURE_CHECK_EQ(complexType1->type, COMPLEX_ARRAY);

    ArrayType* arrType = (ArrayType*)complexType1;
    FeatureType elem_type = arrType->element_type;
    FEATURE_CHECK_NE(elem_type, FT_VOID);

    return elem_type;
}

FtArray* FeatureArrayResize(FtArray* arr, size_t new_size)
{
    if (new_size <= (size_t)arr->_capacity) {
        return arr;
    }

    FeatureType element_type = getElementType(arr);
    int elem_size = getValueSize(element_type);
    void* new_elem = realloc(arr->_element, new_size * elem_size);

    if (new_elem == nullptr) {
        // allocate new space
        new_elem = malloc(new_size * elem_size);
        // Copy data from old array to new array
        memcpy(new_elem, arr->_element, arr->_size * elem_size);
        // Release the original space
        free(arr->_element);
    }

    arr->_element = new_elem;
    arr->_capacity = new_size;
    return arr;
}

size_t FeatureArrayGetLength(FtArray* arr)
{
    return arr->_size;
}

void* FeatureArrayGetDatas(FtArray* arr, int start)
{
    return arr->_element;
}

int FeatureArrayClear(FtArray* arr)
{
    int ret = arr->_size;

    FeatureType element_type = getElementType(arr);
    int elem_size = getValueSize(element_type);
    if (FT_IS_REFERENCE(element_type)) {
        for (int i = 0; i < arr->_size; i++) {
            void* elem = *(void**)((char*)arr->_element + elem_size * i);
            FeatureFreeValue(elem);
        }
    }

    arr->_size = 0;
    return ret;
}

int FeatureArrayRemove(FtArray* arr, int start, size_t count)
{
    // To delete [start,start+count)
    size_t del_size = std::min((int)count, (int)(arr->_size - start));
    if (del_size <= 0)
        return 0;

    FeatureType element_type = getElementType(arr);
    int elem_size = getValueSize(element_type);
    size_t left_count = arr->_size - start - del_size;
    if (FT_IS_REFERENCE(element_type)) {
        for (size_t i = start; i < start + del_size; i++) {
            void* elem = *(void**)((char*)arr->_element + elem_size * i);
            FeatureFreeValue(elem);
        }
    }
    if (left_count) {
        memmove((void*)((uintptr_t)arr->_element + start * elem_size),
            (void*)((uintptr_t)arr->_element + (start + del_size) * elem_size), left_count * elem_size);
    }

    arr->_size -= del_size;
    return del_size;
}

#ifdef CONFIG_FEATURE_FRAMEWORK_UTILS_TYPE_VALIDATE
bool isFeatureTypeEqual(const void* ptr, FeatureType type)
{
    auto pHeader = (FTObjHeader*)((uintptr_t)ptr - sizeof(FTObjHeader));
    if (pHeader->type != MEMORY_FEATURE_TYPE)
        return false;
    return *((FeatureType*)((uintptr_t)pHeader - sizeof(FeatureType))) == type;
}
#endif

FtArray* FeatureArrayAppend(FtArray* arr, const void* data)
{
    FeatureType element_type = getElementType(arr);
#ifdef CONFIG_FEATURE_FRAMEWORK_UTILS_TYPE_VALIDATE
    if (!isFeatureTypeEqual(data, element_type)) {
        FEATURE_LOG_ERROR("element_type do not equal to data type !");
        return nullptr;
    }
#endif
    int elem_size = getValueSize(element_type);
    // Determine whether there is still capacity
    if (arr->_size + 1 > arr->_capacity) {
        // Allocate another piece of memory and copy the contents of the original array there.
        arr = FeatureArrayResize(arr, ARRAY_NEW_CAPACITY(arr->_size));
    }
    if (FT_IS_REFERENCE(element_type)) {
        // If data is a string or other object pointer,
        // the reference count will be increased and the data will not be copied.
        FeatureInstanceDupValue((void*)data);
        // Add new content to the end
        memcpy((void*)((uintptr_t)arr->_element + arr->_size * elem_size), &data, elem_size);
    } else {
        // Add new content to the end
        memcpy((void*)((uintptr_t)arr->_element + arr->_size * elem_size), data, elem_size);
    }

    arr->_size += 1;
    return arr;
}

FtArray* FeatureArrayAppendRaw(FtArray* arr, const void* data)
{
    FeatureType element_type = getElementType(arr);
    if (element_type == FT_STRING) {
        char* feature_str = FeatureStrCopy(nullptr, (const char*)data);
        auto ret = FeatureArrayAppend(arr, feature_str);
        FeatureFreeValue(feature_str);
        return ret;
    }
    FEATURE_LOG_ERROR("only support string as raw data");
    return nullptr;
}

int FeatureArrayInsertAfter(FtArray* arr, int start, const void* data, size_t count)
{
    // The element at arr->_element[start] does not need to be moved.
    // The first element to be moved is arr->_element[start+1]
    if (start >= arr->_size) {
        return 0;
    }
    // Determine whether the capacity is sufficient
    if ((int)(arr->_size + count) > arr->_capacity) {
        // Expand capacity
        arr = FeatureArrayResize(arr, ARRAY_NEW_CAPACITY(arr->_size + count));
    }
    // If data is a reference type, increment the reference count
    FeatureType element_type = getElementType(arr);
    int elem_size = getValueSize(element_type);
    if (FT_IS_REFERENCE(element_type)) {
        for (size_t i = 0; i < count; ++i) {
            FeatureInstanceDupValue((void*)((uintptr_t)data + i * elem_size));
        }
    }

    // Hang the contents of data[0]~data[count-1] to the end of arr->_element
    if (start == arr->_size - 1) {
        memcpy((void*)((uintptr_t)arr->_element + elem_size * arr->_size),
            data, count * elem_size);
    } else {
        // The element at the position arr->_element[start] does not need to be moved.
        // Firstly, move the elements of arr->_element[start+1]~arr->_element[size-1]
        // to
        // the position arr->_element[start+count+1]
        memmove((void*)((uintptr_t)arr->_element + elem_size * (start + count + 1)),
            (void*)((uintptr_t)arr->_element + elem_size * (start + 1)), elem_size * (arr->_size - start - 1));

        // Then hang the count elements of data to arr->_element[start+1]
        memcpy((void*)((uintptr_t)arr->_element + elem_size * (start + 1)),
            data, count * elem_size);
    }
    arr->_size += count;
    return count;
}

int FeatureArrayInsertRawAfter(FtArray* arr, int start, const void* data, size_t count)
{
    // The element at arr->_element[start] does not need to be moved.
    // The first element to be moved is arr->_element[start+1]
    if (start >= arr->_size) {
        return 0;
    }
    // Determine whether the capacity is sufficient
    if ((int)(arr->_size + count) > arr->_capacity) {
        // Expand capacity
        arr = FeatureArrayResize(arr, ARRAY_NEW_CAPACITY(arr->_size + count));
    }

    FeatureType element_type = getElementType(arr);
    int elem_size = getValueSize(element_type);
    if (element_type == FT_STRING) {
        int ret = 0;
        // The element at the position arr->_element[start] does not need to be moved.
        // Firstly, move the elements of arr->_element[start+1]~arr->_element[size-1]
        // to
        // the position arr->_element[start+count+1]
        memmove((void*)((uintptr_t)arr->_element + elem_size * (start + count + 1)),
            (void*)((uintptr_t)arr->_element + elem_size * (start + 1)), elem_size * (arr->_size - start - 1));

        // Then hang the count elements of data to arr->_element[start+1]

        for (size_t i = 0; i < count; ++i) {
            char* feature_str = FeatureStrCopy(nullptr, ((char**)data)[i]);
            if (feature_str) {
                memcpy((void*)((uintptr_t)arr->_element + elem_size * (start + 1 + i)),
                    &feature_str, elem_size);
                FeatureInstanceDupValue(feature_str);
                FeatureFreeValue(feature_str);
                ++ret;
            } else {
                // arr->_element[start + 1+i] waiting for inserting
                // but copy fail
                // Move the element (originally moved to the back) to the arr->_element[start + 1+i] position
                memmove((void*)((uintptr_t)arr->_element + elem_size * (start + i + 1)),
                    (void*)((uintptr_t)arr->_element + elem_size * (start + 1)), elem_size * (arr->_size - start - 1));
                break;
            }
        }

        arr->_size += ret;
        return ret;
    }

    FEATURE_LOG_ERROR("only support string as raw data");
    return 0;
}

int FeatureArrayInsertBefore(FtArray* arr, int start, const void* data, size_t count)
{
    // The element at arr->_element[start] position needs to be moved
    if (start < 0) {
        return 0;
    }

    // Equivalent to moving to FeatureArrayInsertafter(start-1)
    //  ==move after the start-1 position
    return FeatureArrayInsertAfter(arr, start - 1, data, count);
}

int FeatureArrayInsertRawBefore(FtArray* arr, int start, const void* data, size_t count)
{
    // The element at arr->_element[start] position needs to be moved
    if (start < 0) {
        return 0;
    }

    // Equivalent to moving to FeatureArrayInsertRawafter(start-1)
    //  ==move after the start-1 position
    return FeatureArrayInsertRawAfter(arr, start - 1, data, count);
}

void* FeatureMalloc(size_t size, FeatureType featureType)
{
    return FeatureInstanceAllocType(nullptr, size, featureType);
}

void* FeatureDupValue(void* ptr)
{
    FEATURE_CHECK_PTR(ptr, nullptr, "ptr is null !")
    FTObjHeader* header = (FTObjHeader*)((char*)ptr - FT_OBJ_HEADER_SIZE);
    header->ref_count++;
    return ptr;
}

void FeatureFreeValue(void* ptr)
{
    if (!ptr) {
        FEATURE_LOG_DEBUG("ptr is null !");
        return;
    }
    void* header_ptr = ((char*)ptr - FT_OBJ_HEADER_SIZE);
    FTObjHeader* header = (FTObjHeader*)header_ptr;
    if (--header->ref_count > 0) {
        return;
    }

    if (header->type == MEMORY_FEATURE_TYPE) {
        FeatureType* featureTypePtr = (FeatureType*)((uintptr_t)header_ptr - sizeof(FeatureType));
        FeatureType featureType = *featureTypePtr;
        if (FT_IS_COMPLEX(featureType)) {
            ComplexTypeHeader* complexType1 = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
            switch (complexType1->type) {
            case COMPLEX_STRUCT_MAP: {
                ObjectMapType& objMapType = *(ObjectMapType*)complexType1;
                auto member_count = countMember(objMapType.members);
                for (int i = 0; i < member_count; i++) {
                    ObjectMember* member = &objMapType.members[i];
                    FeatureType mtype = FT_GET_REAL_TYPE(member->type);
                    if (FT_NEED_FREE(mtype)) {
                        void* member_ptr = (void*)((char*)ptr + member->offset);
                        FeatureFreeValue(*(void**)member_ptr);
                    }
                }
            } break;
            case COMPLEX_OPTIONAL: {
                // shouldn't contains optional
                // optional is only exit in feature description, we use it's real type for malloc.
                FEATURE_LOG_ERROR("unreachable for COMPLEX_OPTIONAL in FeatureFreeValue !");
                FEATURE_CHECK_NE(false, false);
            } break;
            case COMPLEX_CALLBACK: {

            } break;
            case COMPLEX_ARRAY: {
                // free array elements and ptr
                ArrayType& arrayType = *(ArrayType*)complexType1;
                auto element_type = arrayType.element_type;
                FtArray* arrayData = (FtArray*)ptr;
                // free elements one by one if it's reference.
                if (FT_NEED_FREE(element_type)) {
                    size_t element_size = sizeof(uintptr_t);
                    for (int32_t i = 0; i < arrayData->_size; i++) {
                        void* element_ptr = (char*)arrayData->_element + element_size * i;
                        if (element_ptr) {
                            // free element.
                            FeatureFreeValue(*(void**)element_ptr);
                        }
                    }
                }
                if (arrayData->_element) {
                    free(arrayData->_element);
                }
            } break;
            case COMPLEX_PROMISE: {

            } break;
            case COMPLEX_PROTOBUF: {
                FEATURE_LOG_ERROR("MEM LEAK HERE!!!");
                assert(0);
            } break;

            default: {
                FEATURE_LOG_ERROR("unsupported type !");
            } break;
            }
            if (header->complex_free) {
                free(complexType1);
            }
        }

        // finally, free header
        // NOTE: it's user's responsibility to avoid free unmanaged pointer
        free(featureTypePtr);
    } else if (header->type == MEMORY_REF_COUNT_ONLY) {
        // do nothing
        free(header);
    } else if (header->type == MEMORY_PROTOBUF) {
        proto_utils::release((ProtobufCMessage*)ptr);
        free(header);
    } else {
        FEATURE_LOG_ERROR("UNSUPPORTED FEATURE MEMORY TYPE!!!");
    }
}

static void FeatureRecordMemoryUsage(FeatureInstanceHandle handle, FTObjHeader* ptr)
{
    if (handle == nullptr) {
        return;
    }
// todo
#ifdef ENABLE_FEATURE_MEM_TRACE
    ptr->desc = ...;
#endif
}

void* FeatureInstanceAlloc(FeatureInstanceHandle handle, size_t size)
{
    size_t len { sizeof(FTObjHeader) + size };
    void* p = malloc(len);
    if (!p) {
        FEATURE_LOG_ERROR("malloc failed !");
        return nullptr;
    }
    memset(p, 0, len);
    FTObjHeader* header = (FTObjHeader*)p;
    header->ref_count = 1;
    header->type = MEMORY_REF_COUNT_ONLY;
    FeatureRecordMemoryUsage(handle, header);
    return (void*)((uintptr_t)p + sizeof(FTObjHeader));
}

void* FeatureInstanceAllocProtobuf(FeatureInstanceHandle handle, const ProtobufCMessageDescriptor* desc)
{
    void* p = FeatureInstanceAlloc(handle, desc->sizeof_message);
    FTObjHeader* header = (FTObjHeader*)(uintptr_t(p) - sizeof(FTObjHeader));
    header->ref_count = 1;
    header->type = MEMORY_PROTOBUF;
    return p;
}

static void* FeatureInstanceAllocTypeInternal(FeatureInstanceHandle handle, size_t size, FeatureType type, bool freeType)
{
    size += sizeof(FTObjHeader) + sizeof(FeatureType);
    void* p = malloc(size);
    if (!p) {
        FEATURE_LOG_ERROR("malloc failed !");
        return nullptr;
    }
    memset(p, 0, size);
    *(FeatureType*)p = type;
    FTObjHeader* header = (FTObjHeader*)((uintptr_t)p + sizeof(FeatureType));
    header->ref_count = 1;
    header->complex_free = freeType;
    header->type = MEMORY_FEATURE_TYPE;
    FeatureRecordMemoryUsage(handle, header);
    return (void*)((uintptr_t)p + sizeof(FeatureType) + sizeof(FTObjHeader));
}

void* FeatureInstanceAllocType(FeatureInstanceHandle handle, size_t size, FeatureType type)
{
    return FeatureInstanceAllocTypeInternal(handle, size, type, false);
}

void* FeatureInstanceDupValue(void* ptr)
{
    FTObjHeader* header = (FTObjHeader*)((uintptr_t)ptr - sizeof(FTObjHeader));
    header->ref_count++;
    return header;
}

void FeatureInstanceFreeValue(void* ptr)
{
    FeatureFreeValue(ptr);
}

static inline FeatureManager* manager_from_instance(FeatureInstanceHandle handle)
{
    return static_cast<FeatureInstance*>(handle)->featureManager();
}

FeatureProtoHandle FeatureGetProtoHandle(FeatureInstanceHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    return (FeatureProtoHandle) static_cast<FeatureInstance*>(handle)
        ->prototype();
}

void* FeatureGetProtoData(FeatureProtoHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->native();
}

void FeatureSetProtoData(FeatureProtoHandle handle, void* data)
{
    FEATURE_CHECK_PTR(handle, ;, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    proto->setNative(data);
}

void* FeatureGetObjectData(FeatureInstanceHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    return static_cast<FeatureInstance*>(handle)->native();
}

void FeatureSetObjectData(FeatureInstanceHandle handle, void* data)
{
    FEATURE_CHECK_PTR(handle, ;, "handle is null !")
    auto instance = static_cast<FeatureInstance*>(handle);
    instance->setNative(data);
}

ft_context_ref FeatureGetContext(FeatureInstanceHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    return manager_from_instance(handle)->getFeatureContext();
}

JSValue FeatureGetBindingObject(FeatureInstanceHandle handle)
{
    FEATURE_INSTANCE_CHECK(handle, JS_UNDEFINED)
    FeatureInstanceQjs* instance = static_cast<FeatureInstanceQjs*>(handle);
    return (JSValue)instance->getVmObject();
}

const char* FeatureGetPackageName(FeatureProtoHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->featureManager()->packageName();
}

const char* FeatureGetPackageVersion(FeatureProtoHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->featureManager()->packageVesion();
}

const char* FeatureGetEnvironmentName(FeatureProtoHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->featureManager()->envName();
}

void* FeatureInstanceGetManagerUserData(FeatureInstanceHandle handle,
    const char* name)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    return manager_from_instance(handle)->getUserData(name);
}

bool FeatureInvokeCallback(FeatureInstanceHandle handle, FtCallbackId cid,
    ...)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    auto instance = static_cast<FeatureInstance*>(handle);

    va_list ap;
    va_start(ap, cid);
    int ret = instance->invokeCallback(cid, ap);
    va_end(ap);
    return ret == 0;
}

bool FeatureInvokeCallbackCount(FeatureInstanceHandle handle, FtCallbackId cid,
    int count, ...)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    auto instance = static_cast<FeatureInstance*>(handle);

    va_list ap;
    va_start(ap, count);
    int ret = instance->invokeCallbackCount(cid, ap, count);
    va_end(ap);
    return ret == 0;
}

bool FeatureRemoveCallback(FeatureInstanceHandle handle, FtCallbackId cid)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    auto instance = static_cast<FeatureInstance*>(handle);
    return instance->removeCallback(cid);
}

bool FeaturePromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, ...)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, pid);
    int ret = instance->resolvePromise(pid, ap);
    va_end(ap);
    return ret == 0;
}

bool FeaturePromiseReject(FeatureInstanceHandle handle, FtPromiseId pid,
    int code, const char* msg)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    int ret = instance->rejectPromise(pid, code, msg);
    return ret == 0;
}

FeaturePromiseType FeatureGetPromiseType(FeatureInstanceHandle handle, FtPromiseId pid)
{
    FEATURE_INSTANCE_CHECK(handle, FEATURE_PROMISE_TYPE_INVALID)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    int promise_type = instance->getPromiseType(pid);
    if (promise_type < 0 || promise_type > 1) {
        return FEATURE_PROMISE_TYPE_INVALID;
    }
    return (FeaturePromiseType)promise_type;
}

FeatureInterfaceHandle FeatureCreateInterface(FeatureInstanceHandle handle,
    VTable* vtable)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    FeaturePrototype* module_proto = instance->prototype()->modulePrototype();
    return module_proto->createInterface(vtable);
}

NativeFunc FeatureGetInterfaceMember(FeatureInterfaceHandle handle, size_t index)
{
    if (!handle) {
        FEATURE_LOG_ERROR("handle is null !");
        return nullptr;
    }
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->getVirtualFunction(index);
}

bool FeaturePost(FeatureInstanceHandle handle, FeatureTaskCallback task_cb,
    void* data)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    manager_from_instance(handle)->addTask(handle, task_cb, data);
    return true;
}

FeatureManagerHandle FeatureCreateManager(FeatureManagerCreateInfo* pinfo)
{
    return (FeatureManagerHandle)FeatureManager::CreateFeatureManager(pinfo);
}

ft_context_ref FeatureManagerGetContext(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->getFeatureContext();
}

void FeatureSetArgsErrorCb(FeatureManagerHandle handle, ArgsErrorCb cb, void* data)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    if (manager) {
        manager->setArgsErrorCb(cb, data);
    }
}

void FeatureSetPackageVersion(FeatureManagerHandle handle, const char* package_version)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    if (manager) {
        manager->setPackageVesion(package_version);
    }
}

void FeatureFreeManager(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    delete manager;
}

void FeatureSetUVLoop(FeatureManagerHandle handle, uv_loop_t* loop)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->setUVLoop(loop);
}

void FeatureUnsetUVLoop(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->unsetUVLoop();
}

uv_loop_t* FeatureGetUVLoop(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->getUVLoop();
}

void FeatureSetManagerUserData(FeatureManagerHandle handle, const char* name, void* data)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->setUserData(name, data);
}

bool FeatureHasFeature(FeatureManagerHandle handle, FtString feature_method)
{
    FEATURE_CHECK_PTR(handle, false, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->hasFeature(feature_method);
}

void* FeatureGetManagerUserData(FeatureManagerHandle handle, const char* name)
{
    FEATURE_CHECK_PTR(handle, nullptr, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->getUserData(name);
}

void FeatureUninit(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    manager->uninit();
}

ft_value_t FeatureRequire(FeatureManagerHandle handle, ft_value_t binding_obj, const char* name)
{
    ft_value_t ret = { 0 };
    FEATURE_CHECK_PTR(handle, ret, "handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->featureRequire(binding_obj, name);
}

ft_value_t FeatureFindFeature(FeatureManagerHandle handle, const char* name)
{
    ft_value_t ret = { 0 };
    FEATURE_CHECK_PTR(handle, ret, "handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->findFeature(name);
}

ft_value_t FeatureCreateFeature(FeatureManagerHandle handle, ft_value_t prototype, ft_value_t binding_obj)
{
    ft_value_t ret = { 0 };
    FEATURE_CHECK_PTR(handle, ret, "handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->createFeature(prototype, binding_obj);
}

FeatureManagerHandle FeatureGetManagerHandleFromInstance(FeatureInstanceHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    return manager_from_instance(handle);
}

FeatureManagerHandle FeatureGetManagerHandleFromProto(FeatureProtoHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->featureManager();
}

bool FeatureCheckCallbackId(FeatureInstanceHandle handle, FtCallbackId cid)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    FeatureInstanceQjs* instance = static_cast<FeatureInstanceQjs*>(handle);
    return instance->checkCallback(cid);
}

bool FeatureRegisterFeature(FeatureRegistryHandle handle, const FeatureDescription* description)
{
    FEATURE_CHECK_PTR(description, false, "description is null !")
    FeatureRegistry* registry = static_cast<FeatureRegistry*>(handle);
    if (!registry) {
        FEATURE_LOG_ERROR("Failed to get FeatureRegistry instance!");
        return false;
    }
    return registry->registerFeature(description);
}

FeatureRegistryHandle FeatureGetRegistryFromManager(FeatureManagerHandle handle)
{
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return static_cast<FeatureRegistryHandle>(manager->getFeatureRegistry());
}

bool FeatureRegisterFeatures(FeatureRegistryHandle handle, const FeatureRegistryTableHandle regTableHandle)
{
    FeatureRegistry* registry = static_cast<FeatureRegistry*>(handle);
    FEATURE_CHECK_PTR(registry, false, "Failed to get FeatureRegistry instance!")
    FeatureRegistryTable* regTable = static_cast<FeatureRegistryTable*>(regTableHandle);
    FEATURE_CHECK_PTR(regTable, false, "registry table is null !")
    if (regTable->data[0] == nullptr) {
        FEATURE_LOG_WARN("registry table is empty !");
        return true;
    }
    int16_t i = 0;
    while(regTable->data[i] != nullptr) {
        regTable->data[i++](handle);
    }
    return true;
}

FeatureInstanceHandle FeatureDupInstanceHandle(FeatureInstanceHandle handle)
{
    if (handle) {
        FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
        instance->addRef();
    }
    return handle;
}

void FeatureFreeInstanceHandle(FeatureInstanceHandle handle)
{
    if (handle) {
        FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
        instance->release();
    }
}

bool FeatureInstanceIsDetached(FeatureInstanceHandle handle)
{
    if (handle) {
        FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
        return instance->isDetached();
    }
    return false;
}

void FeatureDumpMemory(FeatureManagerHandle handle, FeatureMemoryDump* dump, void* userdata)
{
    if (handle) {
        FeatureManager* manager = static_cast<FeatureManager*>(handle);
        manager->onDumpMemory(dump, userdata);
    }
}

FtEventId FeatureGetEventId(FeatureInstanceHandle handle, const char* name)
{
    FEATURE_INSTANCE_CHECK(handle, 0)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->getEventId(name);
}

const char* FeatureGetEventName(FeatureInstanceHandle handle, FtEventId eid)
{
    FEATURE_INSTANCE_CHECK(handle, nullptr)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->getEventName(eid);
}

bool FeatureEmitEvent(FeatureInstanceHandle handle, FtEventId eid, ...)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, eid);
    bool ret = instance->emitEvent(eid, ap);
    va_end(ap);
    return ret;
}

bool FeatureEmitEventByName(FeatureInstanceHandle handle, const char* name, ...)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    FtEventId eid = FeatureGetEventId(handle, name);
    if (eid <= 0) {
        return false;
    }

    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, name);
    bool ret = instance->emitEvent(eid, ap);
    va_end(ap);
    return ret;
}

void FeatureSetEventChangeListener(FeatureInstanceHandle handle, FeatureEventChangeListener listener)
{
    FEATURE_INSTANCE_CHECK(handle, ;)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    instance->setEventChangeListener(listener);
}

int FeatureGetEventCallbackCount(FeatureInstanceHandle handle, FtEventId eid)
{
    FEATURE_INSTANCE_CHECK(handle, 0)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->getEventCallbackCount(eid);
}

// permissions related
void FeatureSetPermissionsCallback(FeatureManagerHandle hmanager, FeaturePermissionsCb cb, void* data)
{
    FEATURE_CHECK_PTR(hmanager, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(hmanager);
    if (manager) {
        manager->permissionsManager().SetPermissionsCallback(cb, data);
    }
}

static void grant_permisions_task(int mode, void* data)
{
    if (mode == FEATURE_TASK_MODE_NORMAL) {
        PermissionsInfo* info = (PermissionsInfo*)data;
        FeatureManager* manager = info->Instance()->featureManager();
        manager->permissionsManager().GrantPermissions(info);
    }
}

void FeatureGrantPermissions(FeatureManagerHandle hmanager, FeaturePermissionsHandle handle)
{
    FEATURE_CHECK_PTR(hmanager, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(hmanager);
    PermissionsInfo* info = (PermissionsInfo*)handle;
    if (manager->permissionsManager().CheckPermissions(info)) {
        manager->addTask((FeatureInstanceHandle)(info->Instance()), grant_permisions_task, info);
    }
}

static void reject_permisions_task(int mode, void* data)
{
    if (mode == FEATURE_TASK_MODE_NORMAL) {
        PermissionsInfo* info = (PermissionsInfo*)data;
        FeatureManager* manager = info->Instance()->featureManager();
        manager->permissionsManager().RejectPermissions(info);
    }
}

void FeatureRejectPermissions(FeatureManagerHandle hmanager, FeaturePermissionsHandle handle, FeaturePermsRejectReason reason)
{
    FEATURE_CHECK_PTR(hmanager, ;, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(hmanager);
    PermissionsInfo* info = (PermissionsInfo*)handle;
    if (manager->permissionsManager().CheckPermissions(info)) {
        info->SetRejectReason(reason);
        manager->addTask((FeatureInstanceHandle)(info->Instance()), reject_permisions_task, info);
    }
}

bool FeatureRequestPermissions(FeatureInstanceHandle handle, FeaturePermissionsRequestInfo* info)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    if (!info)
        return false;

    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->requestPermissions(info);
}
