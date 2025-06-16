
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

namespace feature_framework {

class PromiseManager {
public:
    PromiseManager(JSContext* js_ctx);
    ~PromiseManager();

    FtPromiseId addPromise(FeatureType resolve_type);

    FtPromiseId addAsyncCallbacks(FeatureType resolve_type, feature_value_t success, feature_value_t fail, feature_value_t complete);

    bool removePromise(FtPromiseId pid);

    void releasePromises();

    feature_value_t getPromise(FtPromiseId pid);

    void markPromises(feature_runtime_ref rt, feature_mark_func mark_func);

    size_t promiseCount() { return promises_.size(); }

protected:
    int doResolvePromise(FtPromiseId pid, va_list& ap);

    int doRejectPromise(FtPromiseId pid, int code, const char* msg);

    int doGetPromiseType(FtPromiseId pid);

private:
    enum PromiseType {
        kPromise,
        kCallbacks
    };

    class PromiseData {
    public:
        PromiseData(JSContext* ctx, FeatureType ftype);
        PromiseData(JSContext* ctx, FeatureType ftype, feature_value_t success, feature_value_t fail, feature_value_t complete);
        ~PromiseData();
        bool init();
        feature_value_t promise();
        int resolve(va_list& ap);
        int reject(int code, const char* msg);
        void mark(feature_runtime_ref rt, feature_mark_func mark_func);
        PromiseType getPromiseType() { return promise_type; }

    private:
        FeatureType resolve_type;
        PromiseType promise_type;
        JSContext* js_ctx;
        union {
            struct {
                feature_value_t promise; // 保存promise对象, 仅promise情况下有效
                feature_value_t resolve_funcs[2]; // functions
            } promise_info;
            struct {
                feature_value_t success;
                feature_value_t fail;
                feature_value_t complete; // 记录complete, 仅callback形式有效
            } callbacks;
        };
    };

    PromiseData* getPromiseData(FtPromiseId pid);

    FtPromiseId curr_pid_ = 0;
    JSContext* js_ctx_ = nullptr;
    std::map<FtPromiseId, PromiseData*> promises_; // all promises created by native feature
};

}
#endif // __PROMISE_MANAGER_H__
