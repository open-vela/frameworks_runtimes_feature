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
#ifndef __FEATURE_MANAGER_H__
#define __FEATURE_MANAGER_H__
#include "feature.h"
#include "feature_framework.h"

#include <rapidjson/document.h>
#include <map>
#include <string>

typedef rapidjson::Document JSONDocument;

namespace ferry {

struct FeatureUnit;

/**
 * @brief manifest reader
 *
 */
class ManifestReader {
public:
    /**
     * @brief Construct a new Manifest Reader object
     *
     * @param manifest
     */
    ManifestReader() = default;
    /**
     * @brief parse json string
     *
     * @param json
     * @return true
     * @return false
     */
    bool parse(char* json);

    /**
     * @brief
     *
     * @return size_t
     */
    size_t getFeaturesCount();

    /**
     * @brief Get feature name via index
     *
     * @param index
     * @return const char*
     */
    const char* getFeatureName(size_t index);

private:
    JSONDocument doc_;
};

// feature require的实现，直接走FeatureManager，失败之后fallback回JS require
// 需要考虑，无需require的全局函数怎么处理？如setInterval这类

/**
 * @brief Feature Manager, Manage all feature instance.
 *
 * All features and it's related resources(such as file descriptor, callbacks,
 * network connections and so on) should have a definitely life cycle:
 * Application level or Page level.
 * ApplicationManager manages all feature instance and it's life cycles.
 */
class FeatureManager {
public:
    FeatureManager(class IApplication* app);
    /**
     * @brief initialie FeatureManager
     *
     * @param manifest      manifest content to check
     * @return true
     * @return false
     */
    bool init_feature(char* manifest);

    /**
     * @brief register FeatureManager
     *
     * @param features      feature name array pointer
     * @param description   registered feature description array pointer
     * @return true
     * @return false
     */
    bool registerFeature(std::vector<std::string>&features, const FeatureDescription* description);

    /**
     * @brief un-initialize manager
     *
     */
    void uninit();

    /**
     * @brief featureRequire, return feature object by name
     *
     * @param name
     * @param ctx
     * @return JSValue
     */
    feature_value_t featureRequire(context_ref ctx, const char* name);
    std::map<std::string, FeatureUnit*> registeredFeatures_; // 已注册features
    bool manifest_check_enable = true;
private:
    IApplication* app_;
    std::vector<std::string> feature_names_;

};// class FeatureManager

}

#endif
