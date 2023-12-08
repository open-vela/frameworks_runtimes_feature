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
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <string.h>
#include <string>

typedef rapidjson::Document JSONDocument;

namespace ferry {

class ManifestParser {
public:
    ManifestParser() = default;
    bool parse(char* json);
    const char* getPackageName();

private:
    JSONDocument doc_;
};

bool ManifestParser::parse(char* manifest)
{
    doc_.ParseInsitu(manifest);
    if (doc_.HasParseError()) {
        FEATURE_LOG_ERROR("%s: parse json failed: %s", __func__,
            GetParseError_En(doc_.GetParseError()));
        return false;
    }
    return true;
}

const char* ManifestParser::getPackageName()
{
    if (!doc_.HasMember("package")) {
        FEATURE_LOG_WARN("manifest do not have package variable !");
        return "";
    }
    const auto& package = doc_.GetObject()["package"];
    if (!package.IsString()) {
        FEATURE_LOG_WARN("manifest.package is not string !");
        return "";
    }
    return package.GetString();
}

bool FeatureRegistry::init(char* manifest, const char* package_name)
{
    // register features
    ManifestParser parser;
    std::vector<std::string> features;

    if (manifest) {
        FEATURE_LOG_DEBUG("manifest is %s!", manifest);
        if (!parser.parse(manifest)) {
            FEATURE_LOG_ERROR("parse manifest failed !");
            return false;
        }

        package_name_ = parser.getPackageName();
    }

    if (package_name) {
        package_name_ = package_name;
    }

    if (package_name_.empty()) {
        FEATURE_LOG_ERROR("manifest or package_name is null");
    } else {
        FEATURE_LOG_INFO("package_name is %s!", package_name_.c_str());
    }

// register features
#include "ajs_features_list.h"

    return true;
}

bool FeatureRegistry::registerFeature(std::vector<std::string>& features,
    const FeatureDescription* description)
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
