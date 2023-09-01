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
#include "feature_framework.h"

#include <map>
#include <string>

namespace ferry {

struct FeatureUnit;

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
    FeatureRegistry(class IApplication* app);
    /**
     * @brief initialie FeatureRegistry
     *
     * @param manifest      manifest content to check
     * @return true
     * @return false
     */
    bool init(char* manifest);

    /**
     * @brief register FeatureRegistry
     *
     * @param features      feature name array pointer
     * @param description   registered feature description array pointer
     * @return true
     * @return false
     */
    bool registerFeature(std::vector<std::string>&features, const FeatureDescription* description);

    FeatureUnit* findFeature(const char* name);

    /**
     * @brief un-initialize manager
     *
     */
    void uninit();

private:
    IApplication* app_;
    std::map<std::string, FeatureUnit*> registeredFeatures_; // 已注册features
    bool manifest_check_enable = true;
};// class FeatureRegistry

}

#endif // __FEATURE_REGISTRY_H__
