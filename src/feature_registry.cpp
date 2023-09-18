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
#include "feature_utils.h"
#include "ajs_features_init.h"
#include <assert.h>
#include <memory>
#include <rapidjson/error/en.h>
#include <rapidjson/document.h>
#include <string.h>
#include <string>

typedef rapidjson::Document JSONDocument;

namespace ferry {

class ManifestParser {
public:
    ManifestParser() = default;
    bool parse(char* json);
    size_t getFeaturesCount();
    const char* getFeatureName(size_t index);
private:
    JSONDocument doc_;
};

bool ManifestParser::parse(char* manifest)
{
    doc_.ParseInsitu(manifest);
    if (doc_.HasParseError()) {
        FEATURE_LOG_ERROR("%s: parse json failed: %s", __func__, GetParseError_En(doc_.GetParseError()));
        return false;
    }
    return true;
}

size_t ManifestParser::getFeaturesCount()
{
    if (!doc_.HasMember("features")) {
        FEATURE_LOG_WARN("manifest do not have features variable !");
        return 0;
    }
    const auto& features = doc_.GetObject()["features"];
    if (!features.IsArray()) {
        FEATURE_LOG_WARN("manifest.features is not array !");
        return 0;
    }
    const auto& featuresArray = features.GetArray();
    return featuresArray.Size();
}

const char* ManifestParser::getFeatureName(size_t index)
{
    const auto& count = getFeaturesCount();
    if (!count) {
        FEATURE_LOG_WARN("features count is 0 !");
        return "";
    }
    if (index >= count) {
        FEATURE_LOG_WARN("features count: %u, index %u out of bound !", count, index);
        return "";
    }
    const auto& featureObj = doc_.GetObject()["features"].GetArray()[index].GetObject();
    if (!featureObj.HasMember("name")) {
        FEATURE_LOG_WARN("featureObj do not have name property !");
        return "";
    }
    return featureObj["name"].GetString();
}

FeatureRegistry::FeatureRegistry(IApplication* app)
    : app_(app), observer_(nullptr) {
}

bool FeatureRegistry::init(char* manifest)
{
    // register features
    ManifestParser parser;
    std::vector<std::string> features;

    if (manifest != NULL) {
        FEATURE_LOG_DEBUG("manifest is %s!", manifest);
        if (!parser.parse(manifest)) {
            FEATURE_LOG_ERROR("parse manifest failed !");
            return false;
        }
        FEATURE_LOG_DEBUG("parser.getFeaturesCount() is %d!", parser.getFeaturesCount());
        for (size_t i = 0; i < parser.getFeaturesCount(); i++) {
            const char* featureName = parser.getFeatureName(i);
            if (featureName && strlen(featureName)) {
                features.emplace_back(featureName);
            }
        }
    } else {
        FEATURE_LOG_DEBUG("manifest is null!");
        manifest_check_enable = false;
    }

    // register features
    #include "ajs_features_list.h"

    return true;
}

bool FeatureRegistry::registerFeature(std::vector<std::string>&features, const FeatureDescription* description)
{
    if (!description)
        return false;

    if (manifest_check_enable) {
        for (const auto& feature_name : features) {
            if (feature_name == description->name) {
                auto unit = new FeatureUnit(description);
                registeredFeatures_[description->name] = unit;

                if (observer_) {
                    observer_->onFeatureParsed(description->name);
                }

                // invoke onRegister callback
                FEATURE_LOG_DEBUG("description->name is %s...", description->name);
                if (description->native_callbacks->onRegister) {
                    FEATURE_LOG_DEBUG("invoke onRegister callback...");
                    description->native_callbacks->onRegister(const_cast<FeatureDescription*>(description));
                }
                return true;
            }
        }
    } else {
        auto unit = new FeatureUnit(description);
        registeredFeatures_[description->name] = unit;
        // invoke onRegister callback
        FEATURE_LOG_DEBUG("description->name is %s...", description->name);
        if (description->native_callbacks->onRegister) {
            FEATURE_LOG_DEBUG("invoke onRegister callback...");
            description->native_callbacks->onRegister(const_cast<FeatureDescription*>(description));
        }
        return true;
    }
    return false;
}

FeatureUnit* FeatureRegistry::findFeature(const char* name)
{
    FEATURE_LOG_DEBUG("featureRequire for name: %s", name);
    auto pos = registeredFeatures_.find(name);
    if (pos == registeredFeatures_.end()) {
        FEATURE_LOG_WARN("can't find %s in FeatureManager, fallback to original JS module load", name);
        return nullptr;
    }
    return pos->second;
}

}
