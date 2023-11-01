
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

#include "feature.h"
#include "feature_instance.h"
#include "gc_object.h"

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
    FeatureInstanceWamr(struct FeaturePrototype* prototype, VTable* vtable);
    virtual ~FeatureInstanceWamr();

    virtual FeatureInstance* createInterface(VTable* vtable);

    virtual bool removeCallback(FtCallbackId cid);

    virtual int settlePromise(bool resolve, FtPromiseId pid, va_list& ap);

    virtual int invokeCallback(FtCallbackId cid, va_list& ap);

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count);

    WamrCallbackData getCallback(FtCallbackId cid);

    FtCallbackId addCallback(wasm_obj_t value, CallbackType* callbackType);

    FtPromiseId addPromise(FeatureType resolve_type, FeatureType reject_type);

    void addPromise_wamr(feature_value_t data);
    feature_value_t getPromise(FtPromiseId pid);

    std::vector<feature_value_t> promises_wamr;
    void release();

private:
    FtCallbackId curr_cid_ = 0;
    std::unique_ptr<FeatureInstanceQjs> instance_qjs_;
    std::map<FtCallbackId, WamrCallbackData> callbacks_;
};

}
#endif // __FEATURE_INSTANCE_WAMR_H__

