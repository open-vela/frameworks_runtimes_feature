
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

#include <map>
#include <memory>
#include <vector>

namespace ferry {

class FeaturePrototype;

struct WeakRef {
    ft_value_t ft_value;
    struct weakref_list_node link;
};

typedef struct FeaturePromiseData {
   ft_value_t promise; // 保存promise对象
   ft_value_t resolveFuncs[2]; //functions
   FEATURE::FeatureType resolveTypes[2];
} FeaturePromiseData;

typedef struct FeatureCallbackData {
   ft_value_t cb;
   CallbackType* cb_type;
} FeatureCallbackData;

class FeatureInstance {
public:
    FeatureInstance(struct FeaturePrototype* prototype);
    virtual ~FeatureInstance();

    virtual FeatureCallbackData getCallback(FEATURE::FeatureCallbackId id) = 0;

    /**
     * @brief add callback to instance
     *
     * @param ctx
     * @param value
     * @param callbackType
     * @return FEATURE::FeatureCallbackId
     */
    virtual FEATURE::FeatureCallbackId addCallback(ft_value_t value, CallbackType* callbackType) = 0;

    /**
     * @brief remove callback from instance vai FeatureCallbackId
     *
     & @param ctx
     * @param id
     * @return true
     * @return false
     */
    virtual bool removeCallback(FEATURE::FeatureCallbackId id) = 0;

    /**
     * @brief Get the Promise object
     *
     * @param promiseHandle
     * @return FEATURE::FeaturePromiseData*
     */
    virtual FeaturePromiseData* getPromise(FEATURE::FeaturePromiseHandle promiseHandle) = 0;

    virtual FEATURE::FeaturePromiseHandle addPromise(FeaturePromiseData* data) = 0;

    virtual bool removePromise(FEATURE::FeaturePromiseHandle promiseHandle) = 0;

    virtual int settlePromise(bool resolve, FEATURE::FeaturePromiseHandle promiseHandle, va_list& ap) = 0;

    virtual int invokeCallback(
                    const ferry::CallbackType& callbackType,
                    ft_value_t callback,
                    va_list& ap,
                    int method_param_count,
                    int rest_param_count) = 0;

    void setInstanceId(int instance_id) { instance_id_ =  instance_id; }

    int instanceId() { return instance_id_; }

    FeaturePrototype* prototype() { return proto_; }

    std::map<FEATURE::FeaturePromiseHandle, FeaturePromiseData*> promises;   // all promises created by native feature
    std::map<FEATURE::FeatureCallbackId, FeatureCallbackData> callbacks; // instance should save feature resources
    void* native;
    WeakRef weak_self_;
    FEATURE::FeatureCallbackId curr_cid = 0;

private:
    FeaturePrototype* proto_;
    int instance_id_; // the instance id, order in instances aray.
};

}
#endif // __FEATURE_INSTANCE_H__