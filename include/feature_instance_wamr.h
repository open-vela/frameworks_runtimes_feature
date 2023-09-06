
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

#ifndef __FEATURE_INSTANCE_WAMR_H__
#define __FEATURE_INSTANCE_WAMR_H__

#include "feature_instance.h"
#include "wasm_export.h"
#include "gc_object.h"
#include "feature.h"

#include <map>
#include <memory>

namespace ferry {

class FeaturePrototype;
class FeatureInstanceQjs;

typedef struct WamrCallbackData {
   wasm_obj_t cb;
   CallbackType* cb_type;
} WamrCallbackData;

class FeatureInstanceWamr : public FeatureInstance {
public:
    FeatureInstanceWamr(struct FeaturePrototype* prototype);
    virtual ~FeatureInstanceWamr();

    virtual bool removeCallback(FEATURE::FeatureCallbackId id);

    virtual bool removePromise(FEATURE::FeaturePromiseHandle promiseHandle);

    virtual int settlePromise(bool resolve, FEATURE::FeaturePromiseHandle promiseHandle, va_list& ap);

    virtual int invokeCallback(int cid, va_list& ap);

    virtual int invokeCallbackCount(int cid, va_list& ap, int count);

    WamrCallbackData getCallback(FEATURE::FeatureCallbackId id);

    FEATURE::FeatureCallbackId addCallback(wasm_obj_t value, CallbackType* callbackType);

    FEATURE::FeaturePromiseHandle addPromise(FeatureType resolve_type, FeatureType reject_type);

    feature_value_t getPromise(FEATURE::FeaturePromiseHandle promiseHandle);

    void release();

private:
    FEATURE::FeatureCallbackId curr_cid_ = 0;
    std::unique_ptr<FeatureInstanceQjs> instance_qjs_;
    std::map<FEATURE::FeatureCallbackId, WamrCallbackData> callbacks_;
};

}
#endif // __FEATURE_INSTANCE_WAMR_H__

