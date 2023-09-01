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
#ifndef __FEATURE_FRAMEWORK_H__
#define __FEATURE_FRAMEWORK_H__

#include "feature_exports.h"
#include "feature_utils.h"
#include "feature.h"

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
    ft_context_ref ft_ctx; // feature context
    std::vector<std::unique_ptr<FeatureInstance>> instances;
    void* native; // the native feature object instance pointer
    feature_value_t js_proto; // js prototype object, it's undefined at first
    FeatureDescription* description; // description pointer, used for feature management logic
    struct weakref_list_node weak_ref_list; // weak ref list, used to release all weak ref when prototype is destroyed
    int weak_ref_count = 0; // weak ref count

    /**
    * @brief FeaturePrototype constructor
    *
    * @param js_ctx
    * @param description
    */
    FeaturePrototype(context_ref js_ctx, FeatureDescription* feature_desc);

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
};

/**
 * @brief FeatureUnint is the register information for features
 *
 */
struct FeatureUnit {
    FeatureDescription* description;
    FeaturePrototype* proto;

    /**
    * @brief Construct a new Feature Unit object
    *
    * @param desc
    */
    FeatureUnit(const FeatureDescription* desc);

    /**
    * @brief we need the default constructor to support put into containers
    *
    */
    FeatureUnit();

    /**
    * @brief Destroy the Feature Unit object
    *
    */
    ~FeatureUnit();
};

/**
 * @brief initialize prototype
 *
 * @param ctx
 * @param unit
 * @param proto
 * @return int
 */
int initialize_prototype(context_ref ctx, FeatureUnit* unit, feature_value_t proto);


bool WeakRefInit(context_ref js_ctx, feature_value_t feature_object);
bool WeakRefFree(context_ref js_ctx, feature_value_t feature_object);
int getParamCount(const FeatureType* param, bool* hasRest = nullptr, int* optional_size = nullptr);

}

#endif
