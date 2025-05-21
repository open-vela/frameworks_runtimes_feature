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

#include "feature_registry.h"
#include "feature_common.h"

#include <assert.h>
#include <memory>
#include <sstream>
#include <string.h>
#include <string>

#include "feature_instance.h"

namespace feature_framework {

void FeatureRegistry::setOnFeatureRegisteredCB(std::function<bool(const FeatureDescription* description)>&& onFeatureRegistered)
{
    onFeatureRegistered_ = std::move(onFeatureRegistered);
}

FeatureRegistry::~FeatureRegistry()
{
    unregisterAllFeatures();
    registeredFeatures_.clear();
}

bool FeatureRegistry::init(const char* package_name)
{
    if (package_name) {
        package_name_ = package_name;
    }

    if (package_name_.empty()) {
        FEATURE_LOG_ERROR("manifest or package_name is null");
    } else {
        FEATURE_LOG_INFO("package_name is %s!", package_name_.c_str());
    }

    return true;
}

bool FeatureRegistry::registerFeature(const FeatureDescription* description)
{
    if (!description)
        return false;

    if (description->name != nullptr) {
        registeredFeatures_[description->name] = description;
        // invoke onRegister callback
        FEATURE_LOG_DEBUG("registered feature: %s", description->name);
        if (description->native_callbacks && description->native_callbacks->onRegister) {
            FEATURE_LOG_DEBUG("invoke onRegister callback...");
            description->native_callbacks->onRegister(description->name);
        }
        // register for wamr
        if (onFeatureRegistered_)
            if (!onFeatureRegistered_(description))
                return false;
        return true;
    }
    return false;
}

void FeatureRegistry::unregisterAllFeatures()
{
    for (const auto& item : registeredFeatures_) {
        const auto& description = item.second;
        if (description && description->native_callbacks && description->native_callbacks->onUnregister) {
            description->native_callbacks->onUnregister(description->name);
        }
    }
}

const FeatureDescription* FeatureRegistry::findFeature(const char* name)
{
    FEATURE_LOG_DEBUG("find Feature: %s", name);
    auto pos = registeredFeatures_.find(name);
    if (pos == registeredFeatures_.end()) {
        FEATURE_LOG_ERROR("can not find feature: %s", name);
        return nullptr;
    }
    return pos->second;
}

} // namespace feature_framework
