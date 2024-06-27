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
#include "feature_instance.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_main_exports.h"
#include "feature_manager.h"
#include "feature_manager_qjs.h"
#include "feature_prototype.h"
#include "feature_registry.h"
#include "feature_utils.h"

#include <cstdarg>
#include <cstdint>
#include <string.h>

using namespace ferry;

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

#define FEATURE_INSTANCE_CHECK_PTR(__ptr__, __ret__, __log__) \
    do {                                                      \
        if (!__ptr__) {                                       \
            FEATURE_LOG_ERROR(__log__);                       \
            return __ret__;                                   \
        }                                                     \
    } while (0);

void* FeatureMalloc(size_t size, FeatureType featureType)
{
    void* ptr = malloc(size + FT_OBJ_HEADER_SIZE);
    if (!ptr) {
        FEATURE_LOG_ERROR("malloc failed !");
        return nullptr;
    }
    FTObjHeader* objHeader = (FTObjHeader*)ptr;
    objHeader->ref_count = 1;
    objHeader->featureType = featureType;
    ptr = (char*)ptr + FT_OBJ_HEADER_SIZE;
    memset(ptr, 0, size);
    return ptr;
}

void* FeatureDupValue(void* ptr)
{
    FEATURE_INSTANCE_CHECK_PTR(ptr, nullptr, "ptr is null !")
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
        // free
        return;
    }
    FeatureType featureType = header->featureType;
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
                if (FT_NEED_FREE(member_type)) {
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
        default: {
            FEATURE_LOG_ERROR("unsupported type !");
        } break;
        }
    }
    // finally, free header
    // NOTE: it's user's responsibility to avoid free unmanaged pointer
    free(header);
}

static inline FeatureManager* manager_from_instance(FeatureInstanceHandle handle)
{
    return static_cast<FeatureInstance*>(handle)->featureManager();
}

FeatureProtoHandle FeatureGetProtoHandle(FeatureInstanceHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    return (FeatureProtoHandle) static_cast<FeatureInstance*>(handle)
        ->prototype();
}

void* FeatureGetProtoData(FeatureProtoHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->native();
}

void FeatureSetProtoData(FeatureProtoHandle handle, void* data)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, ;, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    proto->setNative(data);
}

void* FeatureGetObjectData(FeatureInstanceHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    return static_cast<FeatureInstance*>(handle)->native();
}

void FeatureSetObjectData(FeatureInstanceHandle handle, void* data)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, ;, "handle is null !")
    auto instance = static_cast<FeatureInstance*>(handle);
    instance->setNative(data);
}

ft_context_ref FeatureGetContext(FeatureInstanceHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
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
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->featureManager()->packageName();
}

const char* FeatureGetPackageVersion(FeatureProtoHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->featureManager()->packageVesion();
}

const char* FeatureGetEnvironmentName(FeatureProtoHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeaturePrototype* proto = static_cast<FeaturePrototype*>(handle);
    return proto->featureManager()->envName();
}

void* FeatureInstanceGetManagerUserData(FeatureInstanceHandle handle,
    const char* name)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
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

int FeatureGetSameCallback(FeatureInstanceHandle handle, FtCallbackId cid)
{
    FEATURE_INSTANCE_CHECK(handle, 0)
    auto instance = static_cast<FeatureInstance*>(handle);
    return instance->getSameCallback(cid);
}

bool FeaturePromiseResolve(FeatureInstanceHandle handle, FtPromiseId pid, ...)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    va_list ap;
    va_start(ap, pid);
    int ret = instance->settlePromise(true, pid, ap);
    va_end(ap);
    return ret == 0;
}

bool FeaturePromiseReject(FeatureInstanceHandle handle, FtPromiseId pid, ...)
{
    FEATURE_INSTANCE_CHECK(handle, false)
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
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeatureInstance* instance = static_cast<FeatureInstance*>(handle);
    FeaturePrototype* module_proto = instance->prototype()->modulePrototype();
    return module_proto->createInterface(vtable);
}

bool FeaturePost(FeatureInstanceHandle handle, FeatureTaskCallback task_cb,
    void* data)
{
    FEATURE_INSTANCE_CHECK(handle, false)
    manager_from_instance(handle)->addTask(handle, task_cb, data);
    return true;
}

FeatureManagerHandle FeatureCreateManager(const char* package_name)
{
    FeatureRegistry* registry = new ferry::FeatureRegistry();
    registry->init(package_name);
    FeatureManagerQjs* manager = new ferry::FeatureManagerQjs(registry);

    return manager;
}

void FeatureSetArgsErrorCb(FeatureManagerHandle handle, ArgsErrorCb cb, void* data)
{
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    if (manager) {
        manager->setArgsErrorCb(cb, data);
    }
}

void FeatureSetPackageVersion(FeatureManagerHandle handle, const char* package_version)
{
    FeatureManager* manager = static_cast<FeatureManager*>(handle);
    if (manager) {
        manager->setPackageVesion(package_version);
    }
}

void FeatureFreeManager(FeatureManagerHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, ;, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    delete manager;
}

void FeatureSetUVLoop(FeatureManagerHandle handle, uv_loop_t* loop)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, ;, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    manager->setUVLoop(loop);
}

void FeatureUnsetUVLoop(FeatureManagerHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, ;, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    manager->unsetUVLoop();
}

uv_loop_t* FeatureGetUVLoop(FeatureManagerHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->getUVLoop();
}

void FeatureSetManagerUserData(FeatureManagerHandle handle, const char* name, void* data)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, ;, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    manager->setUserData(name, data);
}

void* FeatureGetManagerUserData(FeatureManagerHandle handle, const char* name)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->getUserData(name);
}

void FeatureUninit(FeatureManagerHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, ;, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->uninit();
}

JSValue FeatureRequire(FeatureManagerHandle handle, void* ctx, JSValue binding_object, const char* name)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, JS_UNDEFINED, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->featureRequire(ctx, binding_object, name);
}

JSValue FeatureFindFeature(FeatureManagerHandle handle, feature_context_ref ctx, const char* module_name)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, JS_UNDEFINED, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->findFeature(ctx, module_name);
}

JSValue FeatureCreateFeature(FeatureManagerHandle handle, feature_context_ref ctx, JSValue prototype, JSValue vm_object)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, JS_UNDEFINED, "handle is null !")
    FeatureManagerQjs* manager = static_cast<FeatureManagerQjs*>(handle);
    return manager->createFeature(ctx, prototype, vm_object);
}

FeatureManagerHandle FeatureGetManagerHandleFromInstance(FeatureInstanceHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
    return manager_from_instance(handle);
}

FeatureManagerHandle FeatureGetManagerHandleFromProto(FeatureProtoHandle handle)
{
    FEATURE_INSTANCE_CHECK_PTR(handle, nullptr, "handle is null !")
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
    FEATURE_INSTANCE_CHECK_PTR(description, false, "description is null !")
    FeatureRegistry* registry = static_cast<FeatureRegistry*>(handle);
    if (!registry) {
        FEATURE_LOG_ERROR("Failed to get FeatureRegistry instance!");
        return false;
    }
    return registry->registerFeature(description);
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
