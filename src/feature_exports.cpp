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
#include "feature_framework.h"
#include "feature_instance.h"
#include "feature_log.h"
#include "feature_utils.h"

#include <cstdarg>
#include <cstdint>
#include <string.h>

using namespace ferry;

namespace FEATURE {

void* FTMalloc(size_t size, FeatureType featureType)
{
    void* ptr = malloc(size + FT_OBJ_HEADER_SIZE);
    FTObjHeader* objHeader = (FTObjHeader*)ptr;
    objHeader->ref_count = 1;
    objHeader->featureType = featureType;
    ptr = (char*)ptr + FT_OBJ_HEADER_SIZE;
    memset(ptr, 0, size);
    return ptr;
}

void DupFeatureValue(void* ptr)
{
    FTObjHeader* header = (FTObjHeader*)((char*)ptr - FT_OBJ_HEADER_SIZE);
    header->ref_count++;
}

void FreeFeatureValue(void* ptr)
{
    if (!ptr)
        return;
    void* header_ptr = ((char*)ptr - FT_OBJ_HEADER_SIZE);
    FTObjHeader* header = (FTObjHeader*)header_ptr;
    if (--header->ref_count > 0) {
        // free
        return;
    }
    FeatureType featureType = header->featureType;

    // free pointer refers memory
    if (FT_IS_REFERENCE(featureType)) {
        // we do not support reference reference.
        FreeFeatureValue(*(void**)ptr);
        // ptr space is allocated outside, it's callers responsibility to free it
        free(header);
        return;
    }
    if (FT_IS_COMPLEX(featureType)) {
        ComplexTypeHeader* complexType1 = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        switch (complexType1->type) {
        case COMPLEX_STRUCT_MAP: {
            ObjectMapType& objMapType = *(ObjectMapType*)complexType1;
            auto member_count = countMember(objMapType.members);
            for (int i = 0; i < member_count; i++) {
                ObjectMember* member = &objMapType.members[i];
                //FeatureType member_type = member->type;
                auto member_type = member->type;
                TRY_GET_REAL_TYPE(member_type);
                if (FT_IS_REFERENCE(member_type)) {
                    void* member_ptr = (void*)((char*)ptr + member->offset);
                    FreeFeatureValue(*(void**)member_ptr);
                }
            }
            // TODO: if we can free the ptr? it may not be allocated by malloc().
            // Maybe we can check the last bit of the pointer to determinte if it's allocated by us.
            free(header);
        } break;
        case COMPLEX_OPTIONAL: {
            FreeFeatureValue(ptr);
        } break;
        case COMPLEX_CALLBACK: {

        } break;
        case COMPLEX_ARRAY: {
            // free array elements and ptr
            ArrayType& arrayType = *(ArrayType*)complexType1;
            auto element_type = arrayType.element_type;
            FTArray* arrayData = (FTArray*)ptr;
            // only support reference as element
            if (FT_IS_REFERENCE(element_type)) {
                size_t element_size = sizeof(uintptr_t);
                for (int32_t i = 0; i < arrayData->_size; i++) {
                    void* element_ptr = (char*)arrayData->_element + element_size * i;
                    if (element_ptr) {
                        // free it.
                        FreeFeatureValue(*(void**)element_ptr);
                    }
                }
            }
            free(arrayData->_element);
            free(header);
        } break;
        case COMPLEX_PROMISE: {

        } break;
        default: {
            FEATURE_LOG_ERROR("unsupported type !");
        } break;
        }
    } else {
        free(header);
    }
}

void* GetFeatureProtoData(FeatureProtoHandle handle)
{
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->native;
}

void SetFeatureProtoData(FeatureProtoHandle handle, void* data)
{
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    proto->native = data;
}

/**
 * @brief get the native object pointer bind to feature instance
 *
 * @param handle
 * @return void*
 */
void* GetFeatureObjectData(FeatureInstanceHandle handle)
{
    return static_cast<FeatureInstance*>(handle)->native;
}

void SetFeatureObjectData(FeatureInstanceHandle handle, void* data)
{
    auto instance = static_cast<FeatureInstance*>(handle);
    instance->native = data;
}

ft_context_ref GetFeatureContext(FeatureInstanceHandle handle)
{
    return static_cast<FeatureInstance*>(handle)->prototype()->ft_ctx;
}

//int InvokeFeatureCallback(FeatureRuntimeContext ctx, FeatureInstanceHandle handle, void** ret_value, int cid, ...)
int InvokeFeatureCallback(FeatureInstanceHandle handle, int cid, ...)
{
    auto instance = static_cast<FeatureInstance*>(handle);

    va_list ap;
    va_start(ap, cid);
    int ret = instance->invokeCallback(cid, ap);
    va_end(ap);
    return ret;
}

// int InvokeFeatureCallbackCount(FeatureRuntimeContext ctx, FeatureInstanceHandle handle, void** ret_value, FeatureCallbackId cid, int count, ...)
int InvokeFeatureCallbackCount(FeatureInstanceHandle handle, FeatureCallbackId cid, int count, ...)
{
    auto instance = static_cast<FeatureInstance*>(handle);

    va_list ap;
    va_start(ap, count);
    int ret = instance->invokeCallbackCount(cid, ap, count);
    va_end(ap);
    return ret;
}

bool RemoveCallback(FeatureInstanceHandle handle, FeatureCallbackId id)
{
    auto instance = static_cast<FeatureInstance*>(handle);
    return instance->removeCallback(id);
}

int FeaturePromiseResolve(FeatureInstanceHandle handle, FeaturePromiseHandle promiseHandle, ...)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, promiseHandle);
    int ret = instance->settlePromise(true, promiseHandle, ap);
    va_end(ap);
    // remove
    if (!instance->removePromise(promiseHandle)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", promiseHandle);
        ret = -2;
    }
    return ret;
}

int FeaturePromiseReject(FeatureInstanceHandle handle, FeaturePromiseHandle promiseHandle, ...)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, promiseHandle);
    int ret = instance->settlePromise(false, promiseHandle, ap);
    if (!instance->removePromise(promiseHandle)) {
        FEATURE_LOG_ERROR("remove promise:%" PRId32 " failed !", promiseHandle);
        ret = -2;
    }
    va_end(ap);
    return ret;
}

}
