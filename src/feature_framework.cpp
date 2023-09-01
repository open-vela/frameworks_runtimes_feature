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
#include "feature_utils.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>
#include <strings.h>
#include <tuple>

using namespace FEATURE;

namespace ferry {
extern struct FeatureInstance* getInstance(feature_value_t val);

int getParamCount(const FeatureType* param, bool* hasRest, int* optional_size)
{
    int count = 0;
    if (optional_size) {
        *optional_size = 0;
    }
    while (param && FT_GET_VALUE(*param)) {
        count++;
        if (optional_size && FT_IS_COMPLEX(*param)) {
            ferry::ComplexTypeHeader* complexHeader = (ferry::ComplexTypeHeader*)FT_GET_COMPLEX(*param);
            if (complexHeader->type == ferry::COMPLEX_OPTIONAL) {
                *optional_size = *optional_size + 1;
            }
        }
        param++;
    }
    if (hasRest) {
        *hasRest = param ? FT_IS_REST(*param) : false;
    }
    return count;
}

/**
 * @brief FeaturePrototype constructor
 *
 * @param description
 */
FeaturePrototype::FeaturePrototype(void* ctx, FeatureDescription* feature_desc)
    : ft_ctx(CreateFeatureContext(ctx))
    , native(nullptr)
    , description(feature_desc)
{
    // default capacity as 10 element
    instances.reserve(10);
}

FeaturePrototype::~FeaturePrototype()
{
    instances.clear();
    ReleaseFeatureContext(ft_ctx);
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

FeatureUnit::FeatureUnit(const FeatureDescription* desc)
    : description(const_cast<FeatureDescription*>(desc))
    , proto(nullptr)
{
}

/**
 * @brief we need the default constructor to support put into containers
 *
 */
FeatureUnit::FeatureUnit()
    : description(nullptr)
    , proto(nullptr)
{
}

FeatureUnit::~FeatureUnit()
{
    if (proto) {
        delete proto;
    }
}

}
