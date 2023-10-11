
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
#include "feature_types.h"

#include <map>
#include <memory>

namespace ferry {

typedef struct FeaturePromiseData {
    feature_value_t promise; // 保存promise对象
    feature_value_t resolveFuncs[2]; //functions
    FeatureType resolveTypes[2];
} FeaturePromiseData;

class PromiseManager {
public:
    PromiseManager(JSContext* js_ctx);
    virtual ~PromiseManager();

    FtPromiseId addPromise(FeatureType resolve_type, FeatureType reject_type);

    bool removePromise(FtPromiseId pid);

    void releasePromises();

    FeaturePromiseData* getPromiseData(FtPromiseId pid);

    feature_value_t getPromise(FtPromiseId pid);

    void markValues(feature_runtime_ref rt, feature_mark_func mark_func);

private:

    FtPromiseId curr_pid_ = 0;
    JSContext* js_ctx_ = nullptr;
    std::map<FtPromiseId, FeaturePromiseData*> promises_;   // all promises created by native feature
};

}
#endif // __PROMISE_MANAGER_H__