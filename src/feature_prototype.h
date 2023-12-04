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

#ifndef __FEATURE_PROTOTYPE_H__
#define __FEATURE_PROTOTYPE_H__

#include "feature_description.h"
#include "feature_manager.h"
#include "feature_utils.h"

#include <map>
#include <memory>
#include <vector>

namespace ferry {

class FeatureInstance;

/**
 * @brief Feature Protoype struct
 * all informations needed by JS prototype is saved in it
 *
 */
class FeaturePrototype {
public:
    std::vector<std::unique_ptr<FeatureInstance>> instances;
    ft_value_t ft_proto; // ft prototype object, it's undefined at first
    FeatureDescription* description; // description pointer, used for feature management logic
    struct weakref_list_node weak_ref_list; // weak ref list, used to release all weak ref when prototype is destroyed
    int weak_ref_count = 0; // weak ref count

    /**
     * @brief FeaturePrototype constructor
     *
     * @param description
     */
    FeaturePrototype(const FeatureDescription* feature_desc);

    /**
     * @brief Destroy the Feature Prototype object
     *
     */
    ~FeaturePrototype();

    /**
     * @brief add FeatureInstance
     *
     * @param inst
     * @return int
     */
    int addInstance(std::unique_ptr<FeatureInstance>&& inst);

    /**
     * @brief Remove FeatureInstance by index
     *
     * @param pos
     * @return true
     * @return false
     */
    bool removeInstance(size_t pos);

    /**
     * @brief if there has any instance alive
     *
     * @return true
     * @return false
     */
    bool hasInstanceAlive();

    /**
     * @brief free instance that hold by this class
     *
     */
    void clearAllInstances();

    void setFeatureManager(FeatureManager* manager) { feature_manager_ = manager; }

    FeatureManager* getFeatureManager() const { return feature_manager_; }

    void setNative(void* native) { native_ = native; }

    void* native() { return native_; }

    FeaturePrototype* getChild(const char* name);

    void addChild(const char* name, FeaturePrototype* child);

    FeaturePrototype* removeChild(const char* name);

    std::map<const char*, FeaturePrototype*>& children() { return children_; }

private:
    void* native_ = nullptr;
    FeatureManager* feature_manager_ = nullptr;
    std::map<const char*, FeaturePrototype*> children_; // all interface instance prototype
};

}
#endif // __FEATURE_PROTOTYPE_H__