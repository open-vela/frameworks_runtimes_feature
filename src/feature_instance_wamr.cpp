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
#include "feature_instance_wamr.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "feature_context_wamr.h"

#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>

using namespace FEATURE;

namespace ferry {

FeatureInstanceWamr::FeatureInstanceWamr(FeaturePrototype* proto)
    : FeatureInstance(proto)
{
}

FeatureInstanceWamr::~FeatureInstanceWamr()
{

}

FeatureCallbackId FeatureInstanceWamr::addCallback(ft_value_t value, CallbackType* callbackType)
{
    return curr_cid_++;
}

bool FeatureInstanceWamr::removeCallback(FeatureCallbackId id)
{
    return true;
}

bool FeatureInstanceWamr::removePromise(FeaturePromiseHandle promiseHandle)
{
    return true;
}

int FeatureInstanceWamr::settlePromise(bool resolve, FeaturePromiseHandle promiseHandle, va_list& ap)
{
    return 0;
}

int FeatureInstanceWamr::invokeCallback(int cid, va_list& ap) {
    return 0;
}

int FeatureInstanceWamr::invokeCallbackCount(int cid, va_list& ap, int count) {
    return 0;
}

}

