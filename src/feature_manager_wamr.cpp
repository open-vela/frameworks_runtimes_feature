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

#include "feature_manager_wamr.h"
#include "feature_registry.h"
#include "feature_framework.h"
#include "feature_instance_wamr.h"
#include "feature_context_wamr.h"
#include "feature_log.h"
#include "feature_utils.h"
#include <assert.h>
#include <ffi.h>
#include <memory>
#include <string.h>
#include <string>

using namespace FEATURE;
namespace ferry {

FeatureManagerWamr::FeatureManagerWamr(FeatureRegistry* registry)
    : registry_(registry)
    , ft_ctx_(nullptr)
{
}

void FeatureManagerWamr::featureRelease()
{
    for (int i = 0; i < required_features_.size(); ++i) {
        FeatureUnit* unit = registry_->findFeature(required_features_[i].data());
        if (!unit)
            continue;
        auto description = unit->description;
    }
    required_features_.clear();

    if (ft_ctx_) {
        ReleaseFeatureContext(ft_ctx_);
        ft_ctx_ = nullptr;
    }
}

}

