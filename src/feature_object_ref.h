
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

#ifndef __FEATURE_COUNTED_H__
#define __FEATURE_COUNTED_H__

#include "feature_log.h"
#include <atomic>
#include <memory>

namespace feature_framework {

class FeatureObjectRef {
public:
    FeatureObjectRef();

    virtual ~FeatureObjectRef();

    int addRef()
    {
        return ++ref_count_;
    }

    int getRefCount() const
    {
        return ref_count_.load();
    }

    void release()
    {
        FEATURE_LOG_DEBUG("FeatureObjectRef count:%d, feature:%p", ref_count_.load() - 1, this);
        if (--ref_count_ == 0) {
            delete this;
        }
    }

private:
    std::atomic<int32_t> ref_count_;
};

struct FeatureObjectDeleter {
    void operator()(FeatureObjectRef* ref)
    {
        if (ref) {
            ref->release();
        }
    }
};

template <typename _Tp>
using FeatureObjectUniquePtr = std::unique_ptr<_Tp, FeatureObjectDeleter>;

}
#endif // __FEATURE_COUNTED_H__
