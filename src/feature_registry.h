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
#ifndef __FEATURE_REGISTRY_H__
#define __FEATURE_REGISTRY_H__

#include "feature_main_exports.h"
#include <functional>
#include <map>
#include <string>
#include <uv.h>
#include <vector>

struct FeatureDescription;

namespace feature_framework {

struct FeatureUnit;
class FeaturePrototype;

/**
 * @brief Feature Registry, Manage all feature instance.
 *
 * All features and it's related resources(such as file descriptor, callbacks,
 * network connections and so on) should have a definitely life cycle:
 * Application level or Page level.
 * ApplicationManager manages all feature instance and it's life cycles.
 */
class FeatureRegistry {
public:
    FeatureRegistry() = default;
    ~FeatureRegistry();
    /**
     * @brief initialie FeatureRegistry
     *
     * @param manifest      manifest content to check
     * @return true
     * @return false
     */
    bool init(const char* package_name = nullptr);

    /**
     * @brief register FeatureRegistry
     *
     * @param features      feature name array pointer
     * @param description   registered feature description array pointer
     * @return true
     * @return false
     */
    bool registerFeature(const FeatureDescription* description);
    void setOnFeatureRegisteredCB(std::function<bool(const FeatureDescription* description)>&& onFeatureRegistered);
    void unregisterAllFeatures();
    const FeatureDescription* findFeature(const char* name);
    const char* getFeaturePackageName() const { return package_name_.data(); }
    /**
     * @brief Get the Registered Features object
     *
     * @return const std::map<std::string, FeatureUnit*>&
     */
    const std::map<std::string, const FeatureDescription*>& getRegisteredFeatures() const { return registeredFeatures_; }

private:
    std::map<std::string, const FeatureDescription*> registeredFeatures_; // 已注册features
    std::string package_name_;
    std::function<bool(const FeatureDescription* description)> onFeatureRegistered_;

}; // class FeatureRegistry

}

#endif // __FEATURE_REGISTRY_H__
