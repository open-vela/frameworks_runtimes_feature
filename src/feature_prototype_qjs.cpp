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

#include "feature_prototype_qjs.h"
#include "feature_context_qjs.h"
#include "feature_description.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"

#include <cstdarg>
#include <cstdint>
#include <functional>
#include <string>

namespace feature_framework {

FeaturePrototypeQjs::FeaturePrototypeQjs(const FeatureDescription* description)
    : FeaturePrototype(description)
{
    auto js_proto_ptr = FT_VAL_GET_JS_VAL_PTR(ft_proto_);
    *js_proto_ptr = FEATURE_VALUE_UNDEFINED;
    weakref_list_initialize(&weak_ref_list_);
}

FeaturePrototypeQjs::~FeaturePrototypeQjs()
{
}

FeatureInstance* FeaturePrototypeQjs::createInterface(VTable* vtable)
{
    return new FeatureInstanceQjs(this, vtable);
}

FeaturePrototype* FeaturePrototypeQjs::createInterfacePrototype(const FeatureDescription* description)
{
    FeaturePrototypeQjs* proto = new FeaturePrototypeQjs(description);
    proto->setFeatureManager(featureManager());
    return proto;
}

}
