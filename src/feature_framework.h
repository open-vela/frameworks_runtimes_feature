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

#include "feature_description.h"
#include "feature_utils.h"

#include <map>
#include <memory>
#include <vector>

#define TRY_GET_REAL_TYPE(featureType)                                                                  \
    if (FT_IS_COMPLEX(featureType)) {                                                                   \
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType); \
        if (complexType->type == COMPLEX_OPTIONAL) {                                             \
            featureType = ((OptionalType*)complexType)->type;                                    \
        }                                                                                               \
    }

#define IS_INTERFACE_TYPE(featureType, ret)                                                             \
    if (FT_IS_COMPLEX(featureType)) {                                                                   \
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType); \
        ret = complexType->type == COMPLEX_INTERFACE;                                            \
    } else {                                                                                            \
        ret = false;                                                                                    \
    }

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
    ft_value_t ft_proto; // ft prototype object, it's undefined at first
    FeatureDescription* description; // description pointer, used for feature management logic
    struct weakref_list_node weak_ref_list; // weak ref list, used to release all weak ref when prototype is destroyed
    int weak_ref_count = 0; // weak ref count

    /**
     * @brief FeaturePrototype constructor
     *
     * @param js_ctx
     * @param description
     */
    FeaturePrototype(ft_context_ref ctx, const FeatureDescription* feature_desc);

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

    void setPackageName(char* package_name);

    char* getPackageName() const;

    void setEnvironmentName(const char* environment_name);

    char* getEnvironmentName() const { return (char*)environment_name_; }

private:
    char* package_name_ = nullptr;
    const char* environment_name_ = nullptr;
};

/**
 * @brief initialize prototype
 *
 * @param ctx
 * @param unit
 * @param proto
 * @return int
 */
int getParamCount(const FeatureType* param, bool* hasRest = nullptr, int* optional_size = nullptr);

int getValueSize(FeatureType featureType);

int countMember(ObjectMember* member);

}

#endif
