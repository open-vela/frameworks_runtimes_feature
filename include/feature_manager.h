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
#include "feature_framework.h"

namespace ferry {

class FeatureRegistry;

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
    FeatureManager(FeatureRegistry* registry);
    /**
     * @brief featureRequire, return feature object by name
     *
     * @param name
     * @param ctx
     * @return JSValue
     */
    feature_value_t featureRequire(context_ref ctx, const char* name);
private:
    FeatureRegistry* registry_;
};// class FeatureManager

}

#endif
