
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

#ifndef __FEATURE_INSTANCE_H__
#define __FEATURE_INSTANCE_H__

#include "feature_list.h"
#include "feature_object_ref.h"
#include "feature_prototype.h"
#include "feature_types.h"
#include "worker_manager.h"

#include <map>
#include <memory>
#include <vector>

namespace feature_framework {

#define FEATURE_INSTANCE_CPP_VTABLE ((VTable*)-1)

class FeatureInstance : public FeatureObjectRef, public feature_list_node {
public:
    FeatureInstance(FeaturePrototype* proto, const FeatureDescription* description);

    FeatureInstance(FeaturePrototype* module_proto, const VTable* vtable, const FeatureDescription* description);

    virtual ~FeatureInstance();

    virtual bool removeCallback(FtCallbackId cid) = 0;

    virtual int invokeCallback(FtCallbackId cid, va_list& ap) = 0;

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count) = 0;

    virtual int resolvePromise(FtPromiseId pid, va_list& ap) = 0;

    virtual int rejectPromise(FtPromiseId pid, int code, const char* msg) = 0;

    virtual bool emitEvent(FtEventId cid, va_list& ap) = 0;

    virtual void setEventChangeListener(FeatureEventChangeListener listener) = 0;

    virtual FtEventId getEventId(const char* name) = 0;

    virtual const char* getEventName(FtEventId eid) = 0;

    virtual int getEventCallbackCount(FtEventId eid) = 0;

    virtual int getPromiseType(FtPromiseId pid) { return -1; }

    virtual bool hasException() { return !error_msg_.empty(); }

    virtual void setErrorMsg(const char* msg) { error_msg_ = msg; }

    virtual void throwError(const char* msg) { }

    virtual const char* getErrorMsg() { return error_msg_.c_str(); }

    int instanceId() { return instance_id_; }

    void setInstanceId(int instance_id) { instance_id_ = instance_id; }

    FeaturePrototype* prototype() { return proto_; }

    void setPrototype(FeaturePrototype* proto) { proto_ = proto; }

    void* native() { return native_; }

    void setNative(void* native) { native_ = native; }

    virtual void initialize();

    bool isInitialized() { return initialized_ == 1; }

    bool isInterface() { return is_interface_ == 1; }

    NativeFunc getVirtualFunction(int index) const
    {
        if (!vtable_ || index < 0 || index >= vtable_->size)
            return nullptr;
        return vtable_->members[index];
    }

    bool isDetached() { return detached_ == 1; }

    virtual void onDetached();

    const FeatureDescription* description() { return description_; }

    FeatureManager* featureManager()
    {
        return prototype()->featureManager();
    }

    WorkerManager* workerManager()
    {
        return worker_manager_;
    }

    virtual void onDumpMemory(FeatureMemoryDump* dump, void* userdata);

    bool requestPermissions(FeaturePermissionsRequestInfo* info);

    bool isBlackListed(FeaturePermissionsRequestInfo* info);

private:
    int instance_id_ : 29;
    uint32_t is_interface_ : 1;
    uint32_t initialized_ : 1;
    uint32_t detached_ : 1;
    const VTable* vtable_;
    void* native_;
    FeaturePrototype* proto_;
    WorkerManager* worker_manager_;
    const FeatureDescription* description_;
    std::string error_msg_;
};

}
#endif // __FEATURE_INSTANCE_H__
