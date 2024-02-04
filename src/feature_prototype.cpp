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

#include "feature_prototype.h"
#include "feature_instance.h"
#include "feature_log.h"

#include <cstdarg>
#include <cstdint>
#include <functional>
#include <string>

namespace ferry {

FeaturePrototype::FeaturePrototype(const FeatureDescription* description)
    : native_(nullptr)
    , description_(description)
    , module_proto_(this)
{
    // default capacity as 10 element
    instances_.reserve(10);
}

FeaturePrototype::~FeaturePrototype()
{
    clearAllInstances();
    children_.clear();
}

int FeaturePrototype::addInstance(std::unique_ptr<FeatureInstance>&& inst)
{
    auto pos = std::find_if(instances_.begin(), instances_.end(), [](const std::unique_ptr<FeatureInstance>& target) {
        return target == nullptr;
    });
    // it's full, append at end
    if (pos == instances_.end()) {
        instances_.emplace_back(std::move(inst));
        return instances_.size() - 1;
    }
    // insert into pos
    *pos = std::move(inst);
    return std::distance(instances_.begin(), pos);
}

bool FeaturePrototype::removeInstance(size_t pos)
{
    if (pos >= instances_.size())
        return false;
    instances_[pos] = nullptr;
    return true;
}

bool FeaturePrototype::hasInstanceAlive()
{
    for (auto& inst : instances_) {
        if (inst) {
            return true;
        }
    }
    return false;
}

void FeaturePrototype::clearAllInstances()
{
    instances_.clear();
}

FeaturePrototype* FeaturePrototype::getInterfacePrototype(const FeatureDescription* description)
{
    const char* name = description->name;
    std::unique_ptr<FeaturePrototype>& intf_proto = children_[name];
    if (!intf_proto) {
        intf_proto.reset(createInterfacePrototype(description));
        intf_proto->setModulePrototype(this);
    }
    return intf_proto.get();
}

}
