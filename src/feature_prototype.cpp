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
#include "feature_context_private.h"
#include "feature_description.h"
#include "feature_instance.h"
#include "feature_log.h"

#include <cstdarg>
#include <cstdint>
#include <string>
#include <functional>

using namespace FEATURE;

namespace ferry {
/**
 * @brief FeaturePrototype constructor
 *
 * @param description
 */
FeaturePrototype::FeaturePrototype(const FeatureDescription* feature_desc)
    : description(const_cast<FeatureDescription*>(feature_desc))
    , native_(nullptr)
{
    // default capacity as 10 element
    instances.reserve(10);
    weakref_list_initialize(&weak_ref_list);
}

FeaturePrototype::~FeaturePrototype()
{
    clearAllInstances();
}

/**
 * @brief add FeatureInstance
 *
 * @param inst
 * @return int
 */
int FeaturePrototype::addInstance(std::unique_ptr<FeatureInstance>&& inst)
{
    auto pos = std::find_if(instances.begin(), instances.end(), [](const std::unique_ptr<FeatureInstance>& target) {
        return target == nullptr;
    });
    // it's full, append at end
    if (pos == instances.end()) {
        instances.emplace_back(std::move(inst));
        return instances.size() - 1;
    }
    // insert into pos
    *pos = std::move(inst);
    return std::distance(instances.begin(), pos);
}

/**
 * @brief Remove FeatureInstance by index
 *
 * @param pos
 * @return true
 * @return false
 */
bool FeaturePrototype::removeInstance(size_t pos)
{
    if (pos >= instances.size())
        return false;
    instances[pos] = nullptr;
    return true;
}

bool FeaturePrototype::hasInstanceAlive()
{
    for (auto& inst : instances) {
        if (inst) {
            return true;
        }
    }
    return false;
}

void FeaturePrototype::clearAllInstances()
{
    instances.clear();
}

FeaturePrototype* FeaturePrototype::getChild(const char* name) {
    if (!children_.count(name))
        return nullptr;
    return children_[name];
}

void FeaturePrototype::addChild(const char* name, FeaturePrototype* child) {
    children_[name] = child;
}

FeaturePrototype* FeaturePrototype::removeChild(const char* name) {
    if (!children_.count(name))
        return nullptr;

    FeaturePrototype* child = children_[name];
    children_.erase(name);
    return child;
}

}
