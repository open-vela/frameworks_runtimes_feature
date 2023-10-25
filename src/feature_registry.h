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

#include <map>
#include <string>
#include <vector>
#include <uv.h>

struct FeatureDescription;

namespace ferry {

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
    using FeatureRegistryPair = std::pair<const FeatureDescription*, FeaturePrototype*>;
    FeatureRegistry() = default;
    /**
     * @brief initialie FeatureRegistry
     *
     * @param manifest      manifest content to check
     * @return true
     * @return false
     */
    bool init(char* manifest, uv_loop_t* loop);

    /**
     * @brief register FeatureRegistry
     *
     * @param features      feature name array pointer
     * @param description   registered feature description array pointer
     * @return true
     * @return false
     */
    bool registerFeature(std::vector<std::string>&features, const FeatureDescription* description);

    FeatureRegistryPair* findFeature(const char* name);
    const char* getFeaturePackageName() const { return package_name_.data(); }
    /**
     * @brief Get the Registered Features object
     * 
     * @return const std::map<std::string, FeatureUnit*>& 
     */
    const std::map<std::string, FeatureRegistryPair>& getRegisteredFeatures() const { return registeredFeatures_; }
    uv_loop_t* getFeatureUVLoop() const { return loop_; }
private:
    std::map<std::string, FeatureRegistryPair> registeredFeatures_; // 已注册features
    bool manifest_check_enable = true;
    std::string package_name_;
    uv_loop_t* loop_ = nullptr;

};// class FeatureRegistry

}

#endif // __FEATURE_REGISTRY_H__
