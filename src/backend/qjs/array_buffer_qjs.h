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

#ifndef __ARRAY_BUFFER_QJS_H__
#define __ARRAY_BUFFER_QJS_H__

#include "array_buffer.h"
#include "feature.h"
#include "feature_common.h"

#include <list>

namespace feature_framework {

class FeatureManagerQjs;

class ArrayBufferQjs : public ArrayBuffer, public Clearable {
public:
    ArrayBufferQjs(FeatureManagerQjs* manager, const ArrayBufferCreateParams& param);
    ~ArrayBufferQjs();

    uint8_t* getData(size_t* psize) override;

    void destroy() override;

    void clear() override;

    JSValue getTarget()
    {
        return val_;
    }

    FeatureManagerQjs* getFeatureManager() { return manager_; }

private:
    void release();

    FeatureManagerQjs* manager_ = nullptr;
    JSValue val_;
    std::list<Clearable*>::iterator entry_;
};

}

#endif // __ARRAY_BUFFER_QJS_H__