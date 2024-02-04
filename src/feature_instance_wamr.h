
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
// clang-format off
#include "callback_manager_wamr.h"
#include "callback_manager.h"
// clang-format on
#include "promise_manager.h"

#include <map>
#include <memory>

namespace ferry {

class FeaturePrototype;

class FeatureInstanceWamr : public FeatureInstance, public PromiseManager, public CallbackManager<wasm_exec_env_t, wasm_obj_t, FeatureInstanceWamr> {
public:
    FeatureInstanceWamr(FeaturePrototype* proto);

    FeatureInstanceWamr(FeaturePrototype* module_proto, VTable* vtable);

    virtual ~FeatureInstanceWamr();

    virtual int getSameCallback(FtCallbackId cid);

    virtual bool removeCallback(FtCallbackId cid);

    virtual int settlePromise(bool resolve, FtPromiseId pid, va_list& ap);

    virtual int invokeCallback(FtCallbackId cid, va_list& ap);

    virtual int invokeCallbackCount(FtCallbackId cid, va_list& ap, int count);

    void release();

    wasm_exec_env_t getContext();

    int doInvokeCallback(const CallbackType* callbackType, wasm_obj_t callback, va_list& ap, int method_param_count, int rest_param_count);

private:
    bool argToTarget(va_list& ap, FeatureType ftype, uint64_t& target);

    bool variArgToTarget(void* arg, wasm_value_t& target);
};

}
#endif // __FEATURE_INSTANCE_WAMR_H__
