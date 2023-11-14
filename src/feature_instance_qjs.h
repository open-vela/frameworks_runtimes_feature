
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

#ifndef __FEATURE_INSTANCE_QJS_H__
#define __FEATURE_INSTANCE_QJS_H__

#include "feature.h"
#include "feature_instance.h"

#include <map>
#include <memory>

namespace ferry {

class FeaturePrototype;
class PromiseManager;

typedef struct WeakRef {
    ft_value_t ft_value;
    struct weakref_list_node link;
} WeakRef;

typedef struct FeatureCallbackData {
    feature_value_t cb;
    CallbackType* cb_type;
} FeatureCallbackData;

class FeatureInstanceQjs : public FeatureInstance {
public:
    FeatureInstanceQjs(FeaturePrototype* prototype, VTable* vtable);
    virtual ~FeatureInstanceQjs();

    void setVmObject(feature_value_t vm_object);

    feature_value_t getVmObject() const;

    feature_value_t getFeatureJsvalue(ft_value_t ft_value);

    virtual FeatureInstance* createInterface(VTable* vtable);

    virtual bool removeCallback(FtCallbackId cid);

    virtual int settlePromise(bool resolve, FtPromiseId pid, va_list& ap);

    virtual int invokeCallback(FtCallbackId cid, va_list& ap);

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count);

    FtCallbackId addCallback(feature_value_t value, CallbackType* callbackType);

    bool checkCallback(FtCallbackId cid);

    feature_value_t getPromise(FtPromiseId pid);

    FtPromiseId addPromise(FeatureType resolve_type, FeatureType reject_type);

    void markValues(feature_runtime_ref rt, feature_mark_func mark_func);

    bool initWeakRef(feature_value_t feature_object);

    void freeWeakRef();

    FeaturePrototype* getInterfacePrototype(const char* name)
    {
        if (!prototypes_.count(name)) {
            return nullptr;
        }
        return prototypes_[name];
    }

    void addInterfacePrototype(const char* name, FeaturePrototype* featurePrototype)
    {
        FEATURE_CHECK_EQ(prototypes_.count(name), 0);
        prototypes_[name] = featurePrototype;
    }

    FeaturePrototype* removeInterfacePrototype(const char* name)
    {
        if (!prototypes_.count(name)) {
            return nullptr;
        }
        FeaturePrototype* featurePrototype = prototypes_[name];
        prototypes_.erase(name);
        return featurePrototype;
    }

private:
    FeatureCallbackData getCallback(FtCallbackId cid);

    int doInvokeCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int method_param_count, int rest_param_count);

    feature_value_t vm_object_;
    WeakRef weak_self_;
    FtCallbackId curr_cid_ = 0;
    PromiseManager* promise_manager_ = nullptr;

    std::map<FtCallbackId, FeatureCallbackData> callbacks_; // instance should save feature resources
    std::map<const char*, FeaturePrototype*> prototypes_; // all interface instance prototype
};

}
#endif // __FEATURE_INSTANCE_QJS_H__
