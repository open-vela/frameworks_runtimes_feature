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
#ifndef __FEATURE_MANAGER_QJS_H__
#define __FEATURE_MANAGER_QJS_H__

#include "feature.h"
#include "feature_manager.h"

namespace ferry {

class FeatureRegistry;
class FeatureInstance;
class FeaturePrototypeQjs;

/**
 * @brief Feature Manager, Manage all feature instance.
 *
 * All features and it's related resources(such as file descriptor, callbacks,
 * network connections and so on) should have a definitely life cycle:
 * Application level or Page level.
 * ApplicationManager manages all feature instance and it's life cycles.
 */
class FeatureManagerQjs : public FeatureManager {
public:
    FeatureManagerQjs(FeatureRegistry* registry);
    virtual ~FeatureManagerQjs();
    /**
     * @brief featureRequire, return feature object by name
     *
     * @param name
     * @param ctx
     * @return JSValue
     */
    feature_value_t featureRequire(context_ref ctx, feature_value_t vm_object, const char* name);

    void uninit();

    feature_value_t findFeature(feature_context_ref ctx, const char* name);

    feature_value_t createFeature(feature_context_ref ctx, feature_value_t proto, feature_value_t vm_object);

    feature_value_t createJsInstance(FeaturePrototypeQjs* prototype, FeatureInstance* interface);

    feature_value_t createTargetInterface(FeatureInstance* instance);

    static feature_classid_t jsClassId() { return js_class_id_; }

private:

    bool ensureJsPrototype(FeaturePrototypeQjs* prototype);

    static bool ensureJsClass(feature_context_ref ctx);

    static feature_classid_t js_class_id_;
    static feature_classdef_t js_class_def_;
    static uv_mutex_t js_class_mutex_;
};

}
#endif // __FEATURE_MANAGER_QJS_H__
