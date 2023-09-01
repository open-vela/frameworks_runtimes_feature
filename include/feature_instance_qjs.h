
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

namespace ferry {

class FeaturePrototype;

class FeatureInstanceQjs : public FeatureInstance {
public:
    FeatureInstanceQjs(struct FeaturePrototype* prototype);
    virtual ~FeatureInstanceQjs();

    virtual FeatureCallbackData getCallback(FEATURE::FeatureCallbackId id);

    virtual FEATURE::FeatureCallbackId addCallback(ft_value_t value, CallbackType* callbackType);

    virtual bool removeCallback(FEATURE::FeatureCallbackId id);

    virtual FeaturePromiseData* getPromise(FEATURE::FeaturePromiseHandle promiseHandle);

    virtual FEATURE::FeaturePromiseHandle addPromise(FeaturePromiseData* data);

    virtual bool removePromise(FEATURE::FeaturePromiseHandle promiseHandle);

    virtual int settlePromise(bool resolve, FEATURE::FeaturePromiseHandle promiseHandle, va_list& ap);

    virtual int invokeCallback(
                    const ferry::CallbackType& callbackType,
                    ft_value_t callback,
                    va_list& ap,
                    int method_param_count,
                    int rest_param_count);

private:
};

}
#endif // __FEATURE_INSTANCE_QJS_H__

