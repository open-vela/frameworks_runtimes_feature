
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

#ifndef __PROMISE_MANAGER_H__
#define __PROMISE_MANAGER_H__

#include "feature.h"
#include "feature_description.h"

#include <map>
#include <memory>

namespace ferry {

class PromiseManager {
public:
    PromiseManager(JSContext* js_ctx);
    ~PromiseManager();

    FtPromiseId addPromise(FeatureType resolve_type, FeatureType reject_type);

    bool removePromise(FtPromiseId pid);

    void releasePromises();

    feature_value_t getPromise(FtPromiseId pid);

    void markPromises(feature_runtime_ref rt, feature_mark_func mark_func);

protected:
    int doSettlePromise(bool resolve, FtPromiseId pid, va_list& ap);

    int invokeJsCallback(const CallbackType* callbackType, feature_value_t callback, va_list& ap, int fixed_argc, int rest_argc);

private:
    typedef struct PromiseData {
        feature_value_t promise; // 保存promise对象
        feature_value_t resolve_funcs[2]; //functions
        FeatureType resolve_types[2];
    } PromiseData;

    PromiseData* getPromiseData(FtPromiseId pid);

    FtPromiseId curr_pid_ = 0;
    JSContext* js_ctx_ = nullptr;
    std::map<FtPromiseId, PromiseData*> promises_;   // all promises created by native feature
};

}
#endif // __PROMISE_MANAGER_H__