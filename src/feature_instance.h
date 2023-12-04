
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

#include "feature_description.h"
#include "feature_framework.h"
#include "feature_utils.h"

#include <map>
#include <memory>
#include <queue>
#include <vector>

namespace ferry {

struct TaskData {
    FeatureTaskCallback task_cb;
    void* data;
};

class FeatureInstance {
public:
    FeatureInstance(struct FeaturePrototype* prototype, VTable* vtable);
    virtual ~FeatureInstance();

    virtual FeatureInstance* createInterface(VTable* vtable) = 0;

    /**
     * @brief remove callback from instance vai FtCallbackId
     *
     & @param ctx
     * @param id
     * @return true
     * @return false
     */
    virtual bool removeCallback(FtCallbackId cid) = 0;

    virtual int settlePromise(bool resolve, FtPromiseId pid, va_list& ap) = 0;

    virtual int invokeCallback(FtCallbackId cid, va_list& ap) = 0;

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count) = 0;

    int instanceId() { return instance_id_; }

    void setInstanceId(int instance_id) { instance_id_ = instance_id; }

    FeaturePrototype* prototype() { return proto_; }

    void setPrototype(FeaturePrototype* proto) { proto_ = proto; }

    void* native() { return native_; }

    void setNative(void* native) { native_ = native; }

    FeatureInstance* parent() { return parent_; }

    void setParent(FeatureInstance* parent) { parent_ = parent; }

    NativeFunc getVirtualFunction(int index) const
    {
        if (index < 0 || index >= vtable_->size)
            return nullptr;
        return vtable_->members[index];
    }

    typedef void (*dtor_func)(FeatureInstance*);

    void addCallback(FeatureTaskCallback task_cb, void* data);

    void runAsyncTasks(int task_run_mode);

    void sendAsnyc();

private:

    int instance_id_; // the instance id, order in instances aray.
    VTable* vtable_;
    void* native_;
    FeatureInstance* parent_;
    FeaturePrototype* proto_;
    std::queue<TaskData> task_queue_;
};

}
#endif // __FEATURE_INSTANCE_H__
