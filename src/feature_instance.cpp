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

#include "feature_instance.h"
#include "feature_context.h"
#include "feature_log.h"
#include "feature_utils.h"

#include <string.h>
using namespace FEATURE;

namespace ferry {

typedef void (*finalizer_func)(FeatureInstance*);

/////////////////////////////////////////////////
FeatureInstance::FeatureInstance(FeaturePrototype* proto)
    : instance_id_(-1)
    , is_interface_(0)
    , initialized_(0)
    , detached_(0)
    , vtable_(nullptr)
    , native_(nullptr)
    , proto_(proto)
{
}

FeatureInstance::FeatureInstance(FeaturePrototype* module_proto, const VTable* vtable)
    : instance_id_(-1)
    , is_interface_(vtable ? 1 : 0)
    , initialized_(0)
    , detached_(0)
    , vtable_(vtable)
    , native_(nullptr)
    , proto_(module_proto)
{
}

FeatureInstance::~FeatureInstance()
{
    if (vtable_ && vtable_->finalizer) {
        finalizer_func finalizer = (finalizer_func)(vtable_->finalizer);
        finalizer(this);
    }
}

void FeatureInstance::initialize()
{
    if (initialized_ == 1)
        return;

    auto proto = prototype();
    FEATURE_CHECK_NE(proto, nullptr);
    FeatureObjectUniquePtr<FeatureInstance> interf(this);
    int iid = proto->addInstance(std::move(interf));
    setInstanceId(iid);
    initialized_ = 1;
}

} // namespace ferry
