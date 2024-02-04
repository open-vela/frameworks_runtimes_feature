
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

#include "feature_prototype.h"
#include "feature_types.h"

#include <map>
#include <memory>
#include <vector>

namespace ferry {

class FeatureInstance {
public:
    FeatureInstance(FeaturePrototype* proto);

    FeatureInstance(FeaturePrototype* module_proto, const VTable* vtable);

    virtual ~FeatureInstance();

    virtual bool removeCallback(FtCallbackId cid) = 0;

    virtual int getSameCallback(FtCallbackId cid) = 0;

    virtual int invokeCallback(FtCallbackId cid, va_list& ap) = 0;

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count) = 0;

    virtual int settlePromise(bool resolve, FtPromiseId pid, va_list& ap) = 0;

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

private:
    int instance_id_ : 30;
    uint32_t is_interface_ : 1;
    uint32_t initialized_ : 1;
    const VTable* vtable_;
    void* native_;
    FeaturePrototype* proto_;
};

}
#endif // __FEATURE_INSTANCE_H__
