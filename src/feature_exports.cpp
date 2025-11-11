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

#include "feature_exports.h"
#include "array_buffer.h"
#include "backend/qjs/feature_instance_qjs.h"
#include "backend/qjs/feature_manager_qjs.h"
#include "feature_common.h"
#include "feature_description.h"
#include "feature_instance.h"
#include "feature_log.h"
#include "feature_manager.h"
#include "feature_prototype.h"
#include "feature_registry.h"
#include "feature_types.h"
#include "feature_utils.h"
#include "protobuf/proto_utils.h"
#include "worker_manager.h"
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
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

// memory utils functions
char* FeatureStrCopy(FeatureInstanceHandle handle, const char* str)
{
    char* buf = static_cast<char*>(FeatureInstanceAllocType(handle, strlen(str) + 1, FT_STRING));
    strcpy(buf, str);
    return buf;
}

static FeatureType getArrayFeatureType(FeatureType element_type)
{
    static auto* g_array_type_map = new std::map<FeatureType, ArrayType*>;
    static auto* g_array_type_map_mutex = new std::mutex;
    std::lock_guard<std::mutex> lock(*g_array_type_map_mutex);

    auto [it, inserted] = g_array_type_map->emplace(element_type, nullptr);
    if (inserted) {
        ArrayType* new_array_type = static_cast<ArrayType*>(malloc(sizeof(ArrayType)));
        new_array_type->header.type = COMPLEX_ARRAY;
        new_array_type->header.size = sizeof(FtArray);
        new_array_type->element_type = element_type;
        it->second = new_array_type;
    }
    return FT_MK_COMPLEX(it->second);
}

FtArray* FeatureCreateArray(FeatureInstanceHandle handle, size_t capacity, FeatureType element_type)
{
    FtArray* pArray = static_cast<FtArray*>(FeatureInstanceAllocType(handle, sizeof(FtArray), getArrayFeatureType(element_type)));
    pArray->_capacity = capacity;
    pArray->_size = 0;
    pArray->_element = malloc(capacity * getValueSize(element_type));
    memset(pArray->_element, 0, capacity * getValueSize(element_type));
    return pArray;
}

FtArray* FeatureArrayCopyRaw(FeatureInstanceHandle handle, FeatureType element_type, const void* data, size_t count)
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

FtArray* FeatureArrayCopy(FeatureInstanceHandle handle, FeatureType element_type, const void* data, size_t count)
{
    FtArray* pArray = FeatureCreateArray(handle, count, element_type);
    pArray->_size = count;
    pArray->_capacity = count;

    int elem_size = getValueSize(element_type);
    if (FT_IS_REFERENCE(element_type)) {
        for (size_t i = 0; i < count; ++i) {
            FeatureInstanceDupValue(*(void**)((uintptr_t)data + i * elem_size));
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

void* FeatureArrayGetData(FtArray* arr, int start)
{
    if (start < arr->_size) {
        auto element_type = getElementType(arr);
        return (void*)((uintptr_t)(arr->_element) + start * getValueSize(element_type));
    } else {
        FEATURE_LOG_ERROR("OUT OF RANGE!!");
        return nullptr;
    }
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
        // resize
        FEATURE_CHECK_NE(FeatureArrayResize(arr, ARRAY_NEW_CAPACITY(arr->_size + count)), nullptr);
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
    unsigned int expected, desired;
#ifdef __cplusplus
    do {
        expected = header->ref_count.load(std::memory_order_relaxed);
        if (expected == 0) {
            return nullptr;
        }
        desired = expected + 1;
    } while (!header->ref_count.compare_exchange_weak(expected, desired,
        std::memory_order_relaxed, std::memory_order_relaxed));
#else
    do {
        expected = atomic_load(&header->ref_count);
        if (expected == 0) {
            return NULL;
        }
        desired = expected + 1;
        // use CAS to ensure refcount not changed during the check-and-change operation
    } while (!atomic_compare_exchange_weak(&header->ref_count, &expected, desired));
#endif
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
    unsigned int expected, desired;
#ifdef __cplusplus
    do {
        expected = header->ref_count.load(std::memory_order_relaxed);
        desired = expected - 1;
    } while (!header->ref_count.compare_exchange_weak(expected, desired,
        std::memory_order_relaxed, std::memory_order_relaxed));
#else
    do {
        expected = atomic_load(&header->ref_count);
        desired = expected - 1;
    } while (!atomic_compare_exchange_weak(&header->ref_count, &expected, desired));
#endif
    if (desired > 0) {
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
        } else if (featureType == FT_ARRAY_BUFFER) {
            ArrayBuffer* fab = (ArrayBuffer*)ptr;
            fab->destroy();
            FeatureFreeValue(fab);
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
    atomic_init(&(header->ref_count), 1);
    header->type = MEMORY_REF_COUNT_ONLY;
    FeatureRecordMemoryUsage(handle, header);
    return (void*)((uintptr_t)p + sizeof(FTObjHeader));
}

void* FeatureInstanceAllocProtobuf(FeatureInstanceHandle handle, const ProtobufCMessageDescriptor* desc)
{
    void* p = FeatureInstanceAlloc(handle, desc->sizeof_message);
    FTObjHeader* header = (FTObjHeader*)(uintptr_t(p) - sizeof(FTObjHeader));
    atomic_init(&(header->ref_count), 1);
    header->type = MEMORY_PROTOBUF;
    return p;
}

void* FeatureInstanceAllocType(FeatureInstanceHandle handle, size_t size, FeatureType type)
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
    atomic_init(&(header->ref_count), 1);
    header->type = MEMORY_FEATURE_TYPE;
    FeatureRecordMemoryUsage(handle, header);
    return (void*)((uintptr_t)p + sizeof(FeatureType) + sizeof(FTObjHeader));
}

void* FeatureInstanceDupValue(void* ptr)
{
    FTObjHeader* header = (FTObjHeader*)((uintptr_t)ptr - sizeof(FTObjHeader));
    atomic_fetch_add(&(header->ref_count), 1);
    return ptr;
}

void FeatureInstanceFreeValue(void* ptr)
{
    FeatureFreeValue(ptr);
}

int32_t FeatureGetValueRefCount(void* ptr)
{
    void* header_ptr = ((char*)ptr - FT_OBJ_HEADER_SIZE);
    FTObjHeader* header = (FTObjHeader*)header_ptr;
    unsigned int ret = atomic_load(&(header->ref_count));
    return ret;
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

void* FeatureSetManagerUserDataWithFreeCallback(FeatureManagerHandle handle,
    const char* name, void* data, ManagerUserdataFreeCallback free_cb)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    void* ret = manager->getUserData(name);
    manager->setUserData(name, data, free_cb);
    return ret;
}

bool FeatureManagerHasUserData(FeatureManagerHandle handle, const char* name)
{
    FEATURE_CHECK_PTR(handle, false, "handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->hasUserData(name);
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
    FEATURE_CHECK_PTR(task_cb, false, "task_cb is null !")
    manager_from_instance(handle)->addTask(handle, task_cb, data);
    return true;
}

bool FeaturePostExt(FeatureInstanceHandle handle, FeatureTaskCallbackExt task_cb_ext,
    uint64_t data)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    manager_from_instance(handle)->addTaskExt(handle, task_cb_ext, data);
    return true;
}

uv_loop_t* FeatureGetUVLoop(FeatureManagerHandle handle)
{
    FEATURE_CHECK_PTR(handle, nullptr, "manager handle is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->getUVLoop();
}

void* FeatureGetManagerUserData(FeatureManagerHandle handle, const char* name)
{
    FEATURE_CHECK_PTR(handle, nullptr, "manager handle is null !")
    FEATURE_CHECK_PTR(name, nullptr, "name is null !")
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    return manager->getUserData(name);
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
    while (regTable->data[i] != nullptr) {
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

FtEventId FeatureGetEventId(FeatureInstanceHandle handle, const char* name)
{
    FEATURE_INSTANCE_CHECK(handle, 0)
    FEATURE_CHECK_PTR(name, 0, "name is null !")
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

static void grant_permissions_task(int mode, uint64_t data, FeatureInstanceHandle handle)
{
    auto* instance = (FeatureInstance*)handle;
    if (!instance)
        return;
    if (mode == FEATURE_TASK_MODE_NORMAL && !instance->isDetached()) {
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
        manager->addTaskExt((FeatureInstanceHandle)(info->Instance()), grant_permissions_task, (uint64_t)info);
    }
}

static void reject_permissions_task(int mode, uint64_t data, FeatureInstanceHandle handle)
{
    auto* instance = (FeatureInstance*)handle;
    if (!instance)
        return;
    if (mode == FEATURE_TASK_MODE_NORMAL && !instance->isDetached()) {
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
        manager->addTaskExt((FeatureInstanceHandle)(info->Instance()), reject_permissions_task, (uint64_t)info);
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

FeatureWorkerHandle FeatureCreateWorker(FeatureInstanceHandle handle, FtPromiseId pid, size_t buf_size,
    void (*do_work)(FeatureWorkerHandle), void (*do_after_worker)(FeatureWorkerHandle),
    void (*free)(void*))
{
    FeatureInstance* pInstance = static_cast<FeatureInstance*>(handle);
    return (FeatureWorkerHandle)pInstance->workerManager()->create(pid, buf_size, do_work, do_after_worker, free);
}

bool FeatureWorkerCommit(FeatureInstanceHandle handle, FeatureWorkerHandle hworker)
{
    FeatureInstance* pInstance = static_cast<FeatureInstance*>(handle);
    return (FeatureWorkerHandle)pInstance->workerManager()->commit((FeatureWorker*)hworker);
}

void FeatureWorkerResolve(FeatureInstanceHandle handle, FeatureWorkerHandle hworker, FeatureWorkerResult result)
{
    FeatureInstance* pInstance = static_cast<FeatureInstance*>(handle);
    pInstance->workerManager()->resolve((FeatureWorker*)hworker, result);
}

void FeatureWorkerReject(FeatureInstanceHandle handle, FeatureWorkerHandle hworker, int errcode, const char* err_msg)
{
    FeatureInstance* pInstance = static_cast<FeatureInstance*>(handle);
    pInstance->workerManager()->reject((FeatureWorker*)hworker, errcode, err_msg);
}

bool FeatureWorkerIsValid(FeatureInstanceHandle handle, FeatureWorkerHandle hworker)
{
    FeatureInstance* pInstance = static_cast<FeatureInstance*>(handle);
    return pInstance->workerManager()->checkValid((FeatureWorker*)hworker);
}

int FeatureWorkerGetState(FeatureWorkerHandle hworker)
{
    return ((FeatureWorker*)hworker)->status;
}

int FeatureWorkerCancel(FeatureInstanceHandle handle, FeatureWorkerHandle hworker)
{
    if (!FeatureWorkerIsValid(handle, hworker)) {
        return FeatureWorkerCancelInvalid;
    }
    FeatureInstance* pInstance = static_cast<FeatureInstance*>(handle);
    pInstance->workerManager()->cancel((FeatureWorker*)hworker);
    return 0;
}

const char* FeatureGetJsonString(const FtJsonObject json_obj)
{
    return json_obj ? json_obj->str : NULL;
}

FtJsonObject FeatureAllocJsonObject(size_t str_len)
{
    return (FtJsonObject)FeatureMalloc(str_len, FT_JSON_OBJ);
}

FtJsonObject FeatureNewJsonObject(const char* str)
{
    FEATURE_CHECK_PTR(str, nullptr, "str is null !")
    FtJsonObject json_obj = FeatureAllocJsonObject(strlen(str) + 1);
    sprintf(json_obj->str, "%s", str);
    return json_obj;
}

void FeatureThrowError(FeatureInstanceHandle handle, const char* msg)
{
    FEATURE_CHECK_PTR(handle, ;, "handle is null !")
    FEATURE_CHECK_PTR(msg, ;, "msg is null !")
    if (strlen(msg) == 0) {
        FEATURE_LOG_ERROR("msg is empty !");
        return;
    }
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    instance->throwError(msg);
}

static FtArrayBuffer createArrayBuffer(FeatureInstanceHandle handle, uint8_t* data, size_t size,
    FeatureArrayBufferFreeFunc free_func, void* opaque, bool copy)
{
    FEATURE_INSTANCE_CHECK(handle, nullptr)
    FEATURE_CHECK_PTR(data, nullptr, "data is null !")
    FeatureManager* manager = manager_from_instance(handle);
    ArrayBufferCreateParams params;
    if (copy) {
        params.type = ArrayBufferCreateParams::kNativeCopy;
        params.copy = { data, size };
    } else {
        params.type = ArrayBufferCreateParams::kNative;
        params.native = { data, size, free_func, opaque };
    }
    return (FtArrayBuffer)(manager->createArrayBuffer(params));
}

FtArrayBuffer FeatureNewArrayBufferFromData(FeatureInstanceHandle handle, uint8_t* data, size_t size,
    FeatureArrayBufferFreeFunc free_func, void* opaque)
{
    return createArrayBuffer(handle, data, size, free_func, opaque, false);
}

FtArrayBuffer FeatureNewArrayBufferCopyData(FeatureInstanceHandle handle, uint8_t* data, size_t size)
{
    return createArrayBuffer(handle, data, size, NULL, NULL, true);
}

uint8_t* FeatureArrayBufferGetData(FtArrayBuffer buff, size_t* psize)
{
    FEATURE_CHECK_PTR(buff, nullptr, "FtArrayBuffer is null !")
    ArrayBuffer* array_buffer = (ArrayBuffer*)buff;
    FEATURE_CHECK_PTR(array_buffer, nullptr, "arraybuffer is null !")
    return array_buffer->getData(psize);
}

char* FeatureGetPathFromUri(FeatureInstanceHandle handle, const char* uri)
{
    FEATURE_CHECK_PTR(handle, nullptr, "handle is null !")
    FEATURE_CHECK_PTR(uri, nullptr, "uri is null !")
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    FeatureManager* manager = instance->featureManager();
    if (!manager->uriConvertCb()) {
        FEATURE_LOG_ERROR("uri convert callback is null !");
        return nullptr; // return null
    }
    return manager->uriConvertCb()(manager->packageName(), uri);
}

// some promise resolve functions
FtBool FeatureFtVoidPromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid)
{
    return FeaturePromiseResolve(handle, pid);
}

FtBool FeatureFtStringPromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtString val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtIntPromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtInt val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtUint32PromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtUint32 val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtInt8PromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtInt8 val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtUint8PromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtUint8 val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtInt16PromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtInt16 val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtUint16PromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtUint16 val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtInt64PromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtInt64 val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtUint64PromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtUint64 val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtFloatPromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtFloat val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtDoublePromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtDouble val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtBoolPromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtBool val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtAnyPromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtAny val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}

FtBool FeatureFtArrayPromiseResolve(
    FeatureInstanceHandle hInstance,
    FtPromiseId pid, FtArray* val)
{
    return FeaturePromiseResolve(hInstance, pid, val);
}
