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

#include "feature_prototype_wamr.h"
#include "feature_description.h"
#include "feature_instance_wamr.h"
#include "feature_log.h"

#include <cstdarg>
#include <cstdint>
#include <functional>
#include <string>

namespace feature_framework {

FeaturePrototypeWamr::FeaturePrototypeWamr(const FeatureDescription* description)
    : FeaturePrototype(description)
{
}

FeaturePrototypeWamr::~FeaturePrototypeWamr()
{
}

FeatureInstance* FeaturePrototypeWamr::createInterface(VTable* vtable)
{
    return new FeatureInstanceWamr(this, vtable);
}

FeaturePrototype* FeaturePrototypeWamr::createInterfacePrototype(const FeatureDescription* description)
{
    FeaturePrototypeWamr* proto = new FeaturePrototypeWamr(description);
    proto->setFeatureManager(featureManager());
    return proto;
}

}
