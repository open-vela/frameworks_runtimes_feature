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
#include "ajs_features_init.h"
#include "feature_common.h"

#include <assert.h>
#include <memory>
#include <string.h>
#include <string>
extern "C" {
#include "ajs_cfeatures_init.h"
}
namespace ferry {

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
    FeatureRegistryHandle handle = this;
// register features
#include "ajs_features_list.h"
    return true;
}

bool FeatureRegistry::registerFeature(const FeatureDescription* description)
{
    if (!description)
        return false;

    if (description->name != nullptr) {
        registeredFeatures_[description->name] = std::pair<const FeatureDescription*, FeaturePrototype*>(description,
            nullptr);
        // invoke onRegister callback
        FEATURE_LOG_DEBUG("description->name is %s...", description->name);
        if (description->native_callbacks && description->native_callbacks->onRegister) {
            FEATURE_LOG_DEBUG("invoke onRegister callback...");
            description->native_callbacks->onRegister(description->name);
        }
        return true;
    }
    return false;
}

FeatureRegistry::FeatureRegistryPair*
FeatureRegistry::findFeature(const char* name)
{
    FEATURE_LOG_DEBUG("featureRequire for name: %s", name);
    auto pos = registeredFeatures_.find(name);
    if (pos == registeredFeatures_.end()) {
        return nullptr;
    }
    return &pos->second;
}

} // namespace ferry
