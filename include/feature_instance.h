
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

#include "feature_exports.h"
#include "feature_utils.h"
#include "feature.h"

#include <map>
#include <memory>
#include <vector>

namespace ferry {

class FeaturePrototype;

struct WeakRef {
    feature_value_t js_value;
    struct weakref_list_node link;
};

typedef struct FeaturePromiseData {
   feature_value_t promise; // 保存promise对象
   feature_value_t resolveFuncs[2]; //functions
   FEATURE::FeatureType resolveTypes[2];
} FeaturePromiseData;

typedef struct FeatureCallbackData {
   feature_value_t cb;
   CallbackType* cb_type;
} FeatureCallbackData;

class FeatureInstance {
public:
    FeatureInstance(struct FeaturePrototype* prototype);
    virtual ~FeatureInstance();

    FeatureCallbackData getCallback(FEATURE::FeatureCallbackId id);

    /**
     * @brief add callback to instance
     *
     * @param ctx
     * @param value
     * @param callbackType
     * @return FEATURE::FeatureCallbackId
     */
    FEATURE::FeatureCallbackId addCallback(feature_value_t value, CallbackType* callbackType);

    /**
     * @brief remove callback from instance vai FeatureCallbackId
     *
     & @param ctx
     * @param id
     * @return true
     * @return false
     */
    bool removeCallback(FEATURE::FeatureCallbackId id);

    /**
     * @brief Get the Promise object
     *
     * @param promiseHandle
     * @return FEATURE::FeaturePromiseData*
     */
    FeaturePromiseData* getPromise(FEATURE::FeaturePromiseHandle promiseHandle);

    FEATURE::FeaturePromiseHandle addPromise(FeaturePromiseData* data);

    bool removePromise(FEATURE::FeaturePromiseHandle promiseHandle);

    FeaturePrototype* prototype() { return proto_; }

    void setInstanceId(int instance_id) { instance_id_ =  instance_id; }

    int instanceId() { return instance_id_; }

    virtual int invokeFeatureCallback(
                    const ferry::CallbackType& callbackType,
                    feature_value_t callback,
                    va_list& ap,
                    int method_param_count,
                    int rest_param_count) = 0;

    std::map<FEATURE::FeaturePromiseHandle, FeaturePromiseData*> promises;   // all promises created by native feature
    std::map<FEATURE::FeatureCallbackId, FeatureCallbackData> callbacks; // instance should save feature resources
    void* native;
    WeakRef weak_self_;

private:
    FeaturePrototype* proto_;
    int instance_id_; // the instance id, order in instances aray.
    FEATURE::FeatureCallbackId curr_cid = 0;
};

}
#endif // __FEATURE_INSTANCE_H__