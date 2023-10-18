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
#include "feature_framework.h"
#include "feature_instance.h"
#include "feature_context_private.h"
#include "feature_log.h"

#include <cstdarg>
#include <cstdint>
#include <string.h>

using namespace FEATURE;

namespace ferry {

int getParamCount(const FeatureType* param, bool* hasRest, int* optional_size)
{
    int count = 0;
    if (optional_size) {
        *optional_size = 0;
    }
    while (param && FT_GET_VALUE(*param)) {
        count++;
        if (optional_size && FT_IS_COMPLEX(*param)) {
            ComplexTypeHeader* complexHeader = (ComplexTypeHeader*)FT_GET_COMPLEX(*param);
            if (complexHeader->type == COMPLEX_OPTIONAL) {
                *optional_size = *optional_size + 1;
            }
        }
        param++;
    }
    if (hasRest) {
        *hasRest = param ? (*param == FT_PARAM_REST_END) : false;
    }
    return count;
}

int countMember(ObjectMember* member)
{
    int count = 0;
    while (member->name) {
        count++;
        member++;
    }
    return count;
}

int getValueSize(FeatureType featureType)
{
    if (FT_IS_REFERENCE(featureType)) {
        // alloc pointer pointed memory space
        return sizeof(uintptr_t);
    }
    if (FT_IS_PRIMITIVE(featureType)) {
        switch (FT_GET_VALUE(featureType)) {
            case FT_VOID: {
                return 0;
            } break;
            case FT_INT: {
                return sizeof(int);
            } break;
            case FT_INT8: {
                return sizeof(int8_t);
            } break;
            case FT_UINT8: {
                return sizeof(uint8_t);
            } break;
            case FT_INT16: {
                return sizeof(int16_t);
            } break;
            case FT_UINT16: {
                return sizeof(uint16_t);
            } break;
            case FT_INT32: {
                return sizeof(int32_t);
            } break;
            case FT_UINT32: {
                return sizeof(uint32_t);
            } break;
            case FT_INT64: {
                return sizeof(int64_t);
            } break;
            case FT_UINT64: {
                return sizeof(uint64_t);
            } break;
            case FT_DOUBLE: {
                return sizeof(double);
            } break;
            case FT_FLOAT: {
                return sizeof(float);
            } break;
            case FT_BOOLEAN: {
                return sizeof(bool);
            } break;
            case FT_CHAR: {
            // return 0 for string buffer size.
                return 0;
            } break;
            case FT_ANY: {
                return sizeof(ft_value_t);
            } break;
            default: {
                FEATURE_LOG_WARN("unsupported type detected !");
                return 0;
            }
        }
    } else if (FT_IS_COMPLEX(featureType)) {
        // allocate complex type
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        return complexType->size;
    } else {
        return 0;
    }
}

/**
 * @brief FeaturePrototype constructor
 *
 * @param description
 */
FeaturePrototype::FeaturePrototype(ft_context_ref ctx, const FeatureDescription* feature_desc)
    : ft_ctx(ctx)
    , native(nullptr)
    , description(const_cast<FeatureDescription*>(feature_desc))
{
    // default capacity as 10 element
    instances.reserve(10);
    weakref_list_initialize(&weak_ref_list);
}

FeaturePrototype::~FeaturePrototype()
{
    clearAllInstances();
    ft_ctx = nullptr;
}

/**
 * @brief add FeatureInstance
 *
 * @param inst
 * @return int
 */
int FeaturePrototype::addInstance(std::unique_ptr<FeatureInstance>&& inst)
{
    auto pos = std::find_if(instances.begin(), instances.end(), [](const std::unique_ptr<FeatureInstance>& target) {
        return target == nullptr;
    });
    // it's full, append at end
    if (pos == instances.end()) {
        instances.emplace_back(std::move(inst));
        return instances.size() - 1;
    }
    // insert into pos
    *pos = std::move(inst);
    return std::distance(instances.begin(), pos);
}

/**
 * @brief Remove FeatureInstance by index
 *
 * @param pos
 * @return true
 * @return false
 */
bool FeaturePrototype::removeInstance(size_t pos)
{
    if (pos >= instances.size())
        return false;
    instances[pos] = nullptr;
    return true;
}

bool FeaturePrototype::hasInstanceAlive()
{
    for (auto& inst : instances) {
        if (inst) {
            return true;
        }
    }
    return false;
}

void FeaturePrototype::clearAllInstances()
{
    instances.clear();
}

void FeaturePrototype::setPackageName(char* package_name)
{
    package_name_ = package_name;
}

char* FeaturePrototype::getPackageName() const
{
    return package_name_;
}

}
