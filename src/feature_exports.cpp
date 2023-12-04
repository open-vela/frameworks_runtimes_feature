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
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_manager.h"
#include "feature_manager_qjs.h"
#include "feature_utils.h"

#include <cstdarg>
#include <cstdint>
#include <string.h>

using namespace ferry;

void* FeatureMalloc(size_t size, FeatureType featureType)
{
    void* ptr = malloc(size + FT_OBJ_HEADER_SIZE);
    FTObjHeader* objHeader = (FTObjHeader*)ptr;
    objHeader->ref_count = 1;
    objHeader->featureType = featureType;
    ptr = (char*)ptr + FT_OBJ_HEADER_SIZE;
    memset(ptr, 0, size);
    return ptr;
}

void* FeatureDupValue(void* ptr)
{
    FTObjHeader* header = (FTObjHeader*)((char*)ptr - FT_OBJ_HEADER_SIZE);
    header->ref_count++;
    return ptr;
}

void FeatureFreeValue(void* ptr)
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
        // only free non raw pointer
        if (FT_RAWPOINTER != featureType) {
            FeatureFreeValue(*(void**)ptr);
        }
        // free object header
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
                auto member_type = member->type;
                TRY_GET_REAL_TYPE(member_type);
                if (FT_IS_REFERENCE(member_type)) {
                    void* member_ptr = (void*)((char*)ptr + member->offset);
                    FeatureFreeValue(*(void**)member_ptr);
                }
            }
            free(header);
        } break;
        case COMPLEX_OPTIONAL: {
            FeatureFreeValue(ptr);
        } break;
        case COMPLEX_CALLBACK: {

        } break;
        case COMPLEX_ARRAY: {
            // free array elements and ptr
            ArrayType& arrayType = *(ArrayType*)complexType1;
            auto element_type = arrayType.element_type;
            FtArray* arrayData = (FtArray*)ptr;
            // free elements one by one if it's reference.
            if (FT_IS_REFERENCE(element_type)) {
                size_t element_size = sizeof(uintptr_t);
                for (int32_t i = 0; i < arrayData->_size; i++) {
                    void* element_ptr = (char*)arrayData->_element + element_size * i;
                    if (element_ptr) {
                        // free element.
                        FeatureFreeValue(*(void**)element_ptr);
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

FeatureProtoHandle FeatureGetProtoHandle(FeatureInstanceHandle handle)
{
    return (FeatureProtoHandle) static_cast<FeatureInstance*>(handle)
        ->prototype();
}

void* FeatureGetProtoData(FeatureProtoHandle handle)
{
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->native;
}

void FeatureSetProtoData(FeatureProtoHandle handle, void* data)
{
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    proto->native = data;
}

void* FeatureGetObjectData(FeatureInstanceHandle handle)
{
    return static_cast<FeatureInstance*>(handle)->native;
}

void FeatureSetObjectData(FeatureInstanceHandle handle, void* data)
{
    auto instance = static_cast<FeatureInstance*>(handle);
    instance->native = data;
}

ft_context_ref FeatureGetContext(FeatureInstanceHandle handle)
{
    return static_cast<FeatureInstance*>(handle)->prototype()->getFeatureManager()->getFeatureContext();
}

JSValue FeatureGetBindingObject(FeatureInstanceHandle handle)
{
    FeatureInstanceQjs* instance = static_cast<FeatureInstanceQjs*>(handle);
    return (JSValue)instance->getVmObject();
}

const char* FeatureGetPackageName(FeatureProtoHandle handle)
{
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->getFeatureManager()->getPackageName();
}

const char* FeatureGetEnvironmentName(FeatureProtoHandle handle)
{
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->getFeatureManager()->getEnvironmentName();
}

void* FeatureInstanceGetUserData(FeatureInstanceHandle handle,
    const char* name)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->prototype()->getFeatureManager()->getUserData(name);
}

bool FeatureInvokeCallback(FeatureInstanceHandle handle, FtCallbackId cid,
    ...)
{
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
    auto instance = static_cast<FeatureInstance*>(handle);

    va_list ap;
    va_start(ap, count);
    int ret = instance->invokeCallbackCount(cid, ap, count);
    va_end(ap);
    return ret == 0;
}

bool FeatureRemoveCallback(FeatureInstanceHandle handle, FtCallbackId cid)
{
    auto instance = static_cast<FeatureInstance*>(handle);
    return instance->removeCallback(cid);
}

bool FeaturePromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, ...)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, pid);
    int ret = instance->settlePromise(true, pid, ap);
    va_end(ap);
    return ret == 0;
}

bool FeaturePromiseReject(FeatureInstanceHandle handle, FtPromiseId pid, ...)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, pid);
    int ret = instance->settlePromise(false, pid, ap);
    va_end(ap);
    return ret == 0;
}

FeatureInterfaceHandle FeatureCreateInterface(FeatureInstanceHandle handle,
    VTable* vtable)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->createInterface(vtable);
}

void FeaturePost(FeatureInstanceHandle handle, FeatureTaskCallback task_cb,
    void* data)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    instance->addCallback(task_cb, data);
    instance->sendAsnyc();
}

FeatureManagerHandle FeatureCreateManager(char* manifest)
{
    FeatureRegistry* registry = new ferry::FeatureRegistry();
    registry->init(manifest);
    FeatureManagerQjs* manager = new ferry::FeatureManagerQjs(registry);

    return manager;
}

void FeatureFreeManager(FeatureManagerHandle handle)
{
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    delete manager;
}

void FeatureSetUVLoop(FeatureManagerHandle handle, uv_loop_t* loop)
{
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    manager->setUVLoop(loop);
}

uv_loop_t* FeatureGetUVLoop(FeatureManagerHandle handle)
{
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->getUVLoop();
}

void FeatureSetUserData(FeatureManagerHandle handle, const char* name, void* data)
{
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    manager->setUserData(name, data);
}

void* FeatureGetUserData(FeatureManagerHandle handle, const char* name)
{
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->getUserData(name);
}

void FeatureUninit(FeatureManagerHandle handle)
{
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->uninit();
}

JSValue FeatureRequire(FeatureManagerHandle handle, void* ctx, JSValue binding_object, const char* name)
{
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->featureRequire(ctx, binding_object, name);
}

FeatureManagerHandle FeatureGetManagerHandleFromInstance(FeatureInstanceHandle handle)
{
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    return instance->prototype()->getFeatureManager();
}

FeatureManagerHandle FeatureGetManagerHandleFromProto(FeatureProtoHandle handle)
{
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->getFeatureManager();
}

bool FeatureCheckCallbackId(FeatureInstanceHandle handle, FtCallbackId cid)
{
    FeatureInstanceQjs* instance = static_cast<FeatureInstanceQjs*>(handle);
    return instance->checkCallback(cid);
}
