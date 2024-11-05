
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

#ifndef __CALLBACK_MANAGER_H__
#define __CALLBACK_MANAGER_H__

#include "feature_log.h"

namespace feature_framework {

template <typename TTarget>
struct CallbackData {
    TTarget cb;
    const CallbackType* type;

    CallbackData(const TTarget& callback, CallbackType* cb_type)
        : cb(callback)
        , type(cb_type)
    {
    }

    CallbackData(const CallbackData& cd)
        : cb(cd.cb)
        , type(cd.type)
    {
    }
};

template <typename TCtx, typename TTarget, typename TInstance>
class CallbackManager {
public:
    using cb_data_t = CallbackData<TTarget>;
    using cb_map_t = std::map<FtCallbackId, cb_data_t*>;

    ~CallbackManager()
    {
        clearCallbacks();
    }

    FtCallbackId addCallback(TTarget& cb_val, CallbackType* callbackType)
    {
        auto cid = findCallbackId(cb_val, callbackType);
        if (cid > 0) {
            FEATURE_LOG_DEBUG("callback %d has already added!", cid);
            return cid;
        }

        auto cb = new cb_data_t(addRef(context(), cb_val), callbackType);
        callbacks_[++curr_cid_] = cb;
        return curr_cid_;
    }

    bool eraseCallback(FtCallbackId cid)
    {
        if (cid <= 0) {
            FEATURE_LOG_DEBUG("callback id %d not exist !", cid);
            return false;
        }
        if (!callbacks_.count(cid)) {
            FEATURE_LOG_ERROR("callback id %d not exist !", cid);
            return false;
        }
        releaseRef(context(), callbacks_[cid]->cb);
        delete callbacks_[cid];
        callbacks_.erase(cid);
        return true;
    }

    void clearCallbacks()
    {
        for (auto& it : callbacks_) {
            releaseRef(context(), it.second->cb);
            delete it.second;
        }
        callbacks_.clear();
    }

    cb_data_t* getCallbackData(FtCallbackId cid)
    {
        if (!callbacks_.count(cid)) {
            return nullptr;
        }
        return callbacks_[cid];
    }

    cb_map_t& getCallbacks()
    {
        return callbacks_;
    }

    int callCallback(cb_data_t* cb_data, va_list& va, int fixed_argc, int rest_argc)
    {
        if (!cb_data)
            return -1;

        return static_cast<TInstance*>(this)->doInvokeCallback(cb_data->type->parameters, cb_data->cb, va, fixed_argc, rest_argc);
    }

private:
    TCtx context()
    {
        return static_cast<TInstance*>(this)->getContext();
    }

    FtCallbackId findCallbackId(TTarget& cb_val, CallbackType* callbackType)
    {
        for (auto& it : callbacks_) {
            if (isSameValue(context(), it.second->cb, cb_val)) {
                FtCallbackId cid = it.first;
                if (it.second->type != callbackType) {
                    FEATURE_LOG_WARN("callback %d has different type!", cid);
                }
                return cid;
            }
        }
        return 0;
    }

    FtCallbackId curr_cid_ = 0;
    cb_map_t callbacks_;
};

}
#endif // __CALLBACK_MANAGER_H__
