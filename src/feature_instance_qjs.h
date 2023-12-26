
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
#include "feature_common.h"
#include "callback_manager_qjs.h"
#include "callback_manager.h"

#include <map>
#include <memory>

namespace ferry {

class FeaturePrototype;
class PromiseManager;

typedef struct WeakRef {
    ft_value_t ft_value;
    struct weakref_list_node link;
} WeakRef;

class FeatureInstanceQjs : public FeatureInstance, public CallbackManager<JSContext*, JSValue, FeatureInstanceQjs> {
public:
    FeatureInstanceQjs(FeaturePrototype* proto);
    FeatureInstanceQjs(FeaturePrototype* module_proto, VTable* vtable);
    virtual ~FeatureInstanceQjs();

    void setVmObject(feature_value_t vm_object);

    feature_value_t getVmObject() const;

    feature_value_t getFeatureJsvalue(ft_value_t ft_value);

    virtual bool removeCallback(FtCallbackId cid);

    virtual int getSameCallback(FtCallbackId cid);

    virtual int settlePromise(bool resolve, FtPromiseId pid, va_list& ap);

    virtual int invokeCallback(FtCallbackId cid, va_list& ap);

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count);

    feature_value_t createTargetInterface();

    bool checkCallback(FtCallbackId cid);

    feature_value_t getPromise(FtPromiseId pid);

    FtPromiseId addPromise(FeatureType resolve_type, FeatureType reject_type);

    void markValues(feature_runtime_ref rt, feature_mark_func mark_func);

    bool initWeakRef(feature_value_t feature_object);

    void freeWeakRef();

    FtPromiseId addWamrPromise(FeatureType resolve_type, FeatureType reject_type);

    int settleWamrPromise(bool resolve, FtPromiseId pid, va_list& ap);

    JSContext* getContext();

    int doInvokeCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int method_param_count, int rest_param_count);

private:
    int doSettlePromise(bool resolve, FtPromiseId pid, va_list& ap);
	
    bool argToTarget(va_list &ap, FeatureType ftype, JSValue& target);

    feature_value_t vm_object_;
    WeakRef weak_self_;
    PromiseManager* promise_manager_ = nullptr;
};

}
#endif // __FEATURE_INSTANCE_QJS_H__
