
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

#include "feature_instance.h"
#include "feature.h"

#include <map>
#include <memory>

namespace ferry {

class FeaturePrototype;

typedef struct WeakRef {
    ft_value_t ft_value;
    struct weakref_list_node link;
} WeakRef;

typedef struct FeatureCallbackData {
   feature_value_t cb;
   CallbackType* cb_type;
} FeatureCallbackData;

typedef struct FeaturePromiseData {
   feature_value_t promise; // 保存promise对象
   feature_value_t resolveFuncs[2]; //functions
   FEATURE::FeatureType resolveTypes[2];
} FeaturePromiseData;

class FeatureInstanceQjs : public FeatureInstance {
public:
    FeatureInstanceQjs(struct FeaturePrototype* prototype, NativeFunc* vtable, int vtable_size);
    virtual ~FeatureInstanceQjs();

    virtual bool removeCallback(FEATURE::FeatureCallbackId id);

    virtual bool removePromise(FEATURE::FeaturePromiseHandle promiseHandle);

    virtual int settlePromise(bool resolve, FEATURE::FeaturePromiseHandle promiseHandle, va_list& ap);

    virtual int invokeCallback(int cid, va_list& ap);

    virtual int invokeCallbackCount(int cid, va_list& ap, int count);

    FEATURE::FeatureCallbackId addCallback(feature_value_t value, CallbackType* callbackType);

    feature_value_t getPromise(FEATURE::FeaturePromiseHandle promiseHandle);

    FEATURE::FeaturePromiseHandle addPromise(FeatureType resolve_type, FeatureType reject_type);

    void releasePromises();

    void markValues(feature_runtime_ref rt, feature_mark_func mark_func);

    bool initWeakRef(feature_value_t feature_object);

private:
    FeatureCallbackData getCallback(FEATURE::FeatureCallbackId id);
    FeaturePromiseData* getPromiseData(FEATURE::FeaturePromiseHandle promiseHandle);

    int doInvokeCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int method_param_count, int  rest_param_count);

    WeakRef weak_self_;
    FEATURE::FeatureCallbackId curr_cid_ = 0;
    std::map<FEATURE::FeatureCallbackId, FeatureCallbackData> callbacks_; // instance should save feature resources
    std::map<FEATURE::FeaturePromiseHandle, FeaturePromiseData*> promises_;   // all promises created by native feature
};

}
#endif // __FEATURE_INSTANCE_QJS_H__

