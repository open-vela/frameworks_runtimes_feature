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

#include "feature_manager.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "feature.h"
#if defined(CONFIG_QUICKAPP)
#include "aiotjs.h"
#endif
#include "ajs_features_init.h"
#include <assert.h>
#include <ffi.h>
#include <memory>
#include <rapidjson/error/en.h>
#include <string.h>
#include <string>
#include <vector>

using namespace FEATURE;
#if defined(CONFIG_QUICKAPP)
using namespace AIOTJS;
#endif
namespace ferry {

static thread_local feature_classid_t class_id; // prototype class id
static thread_local feature_classdef_t class_def; // prototype class defination, contains finalizer

// some static functions used by FeatureManager
static bool createFeaturePrototype(context_ref ctx, FeatureUnit* unit);
static bool createPrototypeClass(context_ref ctx, FeaturePrototype* featurePrototype);
static context_ref getContext(feature_runtime_ref rt);

bool ManifestReader::parse(char* manifest)
{
    doc_.ParseInsitu(manifest);
    if (doc_.HasParseError()) {
        FEATURE_LOG_ERROR("%s: parse json failed: %s", __func__, GetParseError_En(doc_.GetParseError()));
        return false;
    }
    return true;
}

size_t ManifestReader::getFeaturesCount()
{
    if (!doc_.HasMember("features")) {
        FEATURE_LOG_WARN("manifest do not have features variable !");
        return 0;
    }
    const auto& features = doc_.GetObject()["features"];
    if (!features.IsArray()) {
        FEATURE_LOG_WARN("manifest.features is not array !");
        return 0;
    }
    const auto& featuresArray = features.GetArray();
    return featuresArray.Size();
}

const char* ManifestReader::getFeatureName(size_t index)
{
    const auto& count = getFeaturesCount();
    if (!count) {
        FEATURE_LOG_WARN("features count is 0 !");
        return "";
    }
    if (index >= count) {
        FEATURE_LOG_WARN("features count: %u, index %u out of bound !", count, index);
        return "";
    }
    const auto& featureObj = doc_.GetObject()["features"].GetArray()[index].GetObject();
    if (!featureObj.HasMember("name")) {
        FEATURE_LOG_WARN("featureObj do not have name property !");
        return "";
    }
    return featureObj["name"].GetString();
}

FeatureManager::FeatureManager(IApplication* app)
    : app_(app)
{
}

bool FeatureManager::init_feature(char* manifest)
{
    // register features
    ManifestReader reader;
    std::vector<std::string> features;

    if (manifest != NULL) {
        FEATURE_LOG_DEBUG("manifest is %s!", manifest);
        if (!reader.parse(manifest)) {
            FEATURE_LOG_ERROR("parse manifest failed !");
            return false;
        }
        FEATURE_LOG_DEBUG("reader.getFeaturesCount() is %d!", reader.getFeaturesCount());
        for (size_t i = 0; i < reader.getFeaturesCount(); i++) {
            const char* featureName = reader.getFeatureName(i);
            if (featureName && strlen(featureName)) {
                features.emplace_back(featureName);
            }
        }
    } else {
        FEATURE_LOG_DEBUG("manifest is null!");
        manifest_check_enable = false;
    }

    // register features
    #include "ajs_features_list.h"

    return true;
}

bool FeatureManager::registerFeature(std::vector<std::string>&features, const FeatureDescription* description)
{
    if (!description)
        return false;

    if (manifest_check_enable) {
        for (const auto& feature_name : features) {
            if (feature_name == description->name) {
                auto unit = new FeatureUnit(description);
                registeredFeatures_[description->name] = unit;
                // invoke onRegister callback
                FEATURE_LOG_DEBUG("description->name is %s...", description->name);
                if (description->native_callbacks->onRegister) {
                    FEATURE_LOG_DEBUG("invoke onRegister callback...");
                    description->native_callbacks->onRegister(const_cast<FeatureDescription*>(description));
                }
                return true;
            }
        }
    } else {
        auto unit = new FeatureUnit(description);
        registeredFeatures_[description->name] = unit;
        // invoke onRegister callback
        FEATURE_LOG_DEBUG("description->name is %s...", description->name);
        if (description->native_callbacks->onRegister) {
            FEATURE_LOG_DEBUG("invoke onRegister callback...");
            description->native_callbacks->onRegister(const_cast<FeatureDescription*>(description));
        }
        return true;
    }
    return false;
}

void FeatureManager::uninit()
{
    // delete all registered FeatureUnit
    for (auto& unit : registeredFeatures_) {
        delete unit.second;
    }
    registeredFeatures_.clear();
}

FeatureInstance* getInstance(feature_value_t val)
{
    auto instance = static_cast<FeatureInstance*>(feature_get_opaque(val, class_id));
    return instance;
}

static context_ref getContext(feature_runtime_ref rt)
{
#if defined(CONFIG_QUICKAPP)
//how to get context in nuttx? need check yaozong
    auto qrt = static_cast<AIOTJS::RuntimeContext*>(JS_GetRuntimeOpaque(rt));
    FEATURE_CHECK_NE(qrt, nullptr);
    return qrt->env.ctx;
#else
    auto ctx = static_cast<context_ref>(JS_GetRuntimeOpaque(rt));
    return ctx;
#endif
}

static void __feature_finalizer(feature_runtime_ref rt, feature_value_t val)
{
    auto instance = getInstance(val);
    if (!instance) {
        FEATURE_LOG_INFO("instance is null...");
        return;
    }
    feature_set_opaque(val, nullptr);
    // get proto pointer, it may not be deleted at this time
    auto proto = instance->proto;
    //遍历proto->weak_ref_list链表，将其中的js_value设置为JSE_UNDEFINED
    WeakRef* node;
    WeakRef* node_temp;
    weakref_list_for_every_entry_safe(&proto->weak_ref_list, node, node_temp, WeakRef, link)
    {
        node->js_value = FEATURE_VALUE_UNDEFINED;
    }

    auto iid = instance->iid;
    // invoke callback
    if (proto->description->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto->description->native_callbacks->onDetached(getContext(rt), instance);
    }
    // delete instance by removing it from FeaturePrototype.
    bool ret = instance->proto->removeInstance(instance->iid);
    FEATURE_LOG_DEBUG("deleting instance %p with iid %d ret %d", instance, iid, ret);
    if (!ret) {
        FEATURE_LOG_ERROR("delete iid %d failed !", iid);
    }
    FEATURE_CHECK_EQ(ret, true);
    // may have other clean operation.
    // check if all instance be deleted and we can delete the FeaturePrototype
}

static void __feature_mark(feature_runtime_ref rt, feature_value_t val, feature_mark_func mark_func)
{
    // TODO: for now，FeatureInstance saves callbacks only, mark it directly.
    // but it maybe insufficient, instance may manage other resource type, change it according to implementation.
    FeatureInstance* instance = getInstance(val);
    if (!instance) {
        FEATURE_LOG_INFO("instance is null...");
        return;
    }
    // mark callbacks
    for (auto& pair : instance->callbacks) {
        feature_mark_value(rt, pair.second.first, mark_func);
    }
    // mark promies
    for(auto& pair : instance->promises) {
        feature_mark_value(rt, pair.second->promise, mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[0], mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[1], mark_func);
    }
    // should mark prototype object.
    feature_mark_value(rt, instance->proto->js_proto, mark_func);
}

/**
 * @brief Create a Prototype Class Defination in C
 *
 * @param ctx
 * @param featurePrototype
 * @return true
 * @return false
 */
static bool createPrototypeClass(context_ref ctx, FeaturePrototype* featurePrototype)
{
    FEATURE_LOG_DEBUG("createPrototypeClass with featurePrototype %p", featurePrototype);
    FEATURE_CHECK_NE(featurePrototype, nullptr);
    // FEATURE_CHECK_EQ(featurePrototype->class_id, 0);
    JS_NewClassID(&class_id);
    FEATURE_CHECK_NE(class_id, 0); // it must not 0 now
    // fill the class_def structure
    class_def = { .class_name = "FeatureInstanceObject", .finalizer = __feature_finalizer, .gc_mark = __feature_mark };
    // create native feature prototype class defination
    JS_NewClass(feature_get_runtime(static_cast<feature_context_ref>(ctx)), class_id, &class_def);
    return true;
}

/**
 * @brief create prototype from FeatureUnit
 *
 * @param ctx
 * @param unit
 * @return true
 * @return false
 */
static bool createFeaturePrototype(context_ref ctx, FeatureUnit* unit)
{
    FEATURE_CHECK_NE(unit, nullptr);
    FEATURE_LOG_DEBUG("unit: %p, description: %p, description->name: %s", unit, unit->description, unit->description->name);
    FEATURE_CHECK_EQ(unit->proto, nullptr);
    unit->proto = new FeaturePrototype(ctx, unit->description);
    bool ret = createPrototypeClass(ctx, unit->proto);
    if (!ret) {
        FEATURE_LOG_ERROR("createPrototypeClass for feature %s with description %p failed !", unit->description->name, unit->description);
        return false;
    }
    FEATURE_LOG_DEBUG("createPrototypeClass for feature %s with description %p success.", unit->description->name, unit->description);
    return true;
}

feature_value_t FeatureManager::featureRequire(context_ref ctx, const char* name)
{
    FEATURE_LOG_DEBUG("featureRequire for name: %s", name);
    auto pos = registeredFeatures_.find(name);
    if (pos == registeredFeatures_.end()) {
        FEATURE_LOG_WARN("can't find %s in FeatureManager, fallback to original JS module load", name);
        // TODO: fallback to classic js module loader
        return FEATURE_VALUE_UNDEFINED;
    }

    auto& unit = *pos->second;
    if (!unit.proto) {
        // create proto
        if (!createFeaturePrototype(ctx, &unit)) {
            FEATURE_LOG_ERROR("createFeaturePrototype failed !");
            return JS_UNDEFINED;
        }
    }
    // create prototype class instance
    if (feature_is_undefined(unit.proto->js_proto)) {
        feature_value_t js_proto_obj = feature_object(static_cast<feature_context_ref>(ctx));
        if (feature_is_exception(js_proto_obj)) {
            feature_dump_error(static_cast<feature_context_ref>(ctx));
            return FEATURE_VALUE_UNDEFINED;
        }
        // TODO: initialize js_proto using description
        initialize_prototype(ctx, &unit, js_proto_obj);
        unit.proto->js_proto = js_proto_obj;
        if (unit.proto->description->native_callbacks->onCreate) {
            FEATURE_LOG_DEBUG("invoke onCreate callback...");
            unit.proto->description->native_callbacks->onCreate(ctx, unit.proto);
        }
    }

    // create feature instance for the required object
    auto featureInstance = std::make_unique<FeatureInstance>(unit.proto);
    // create object with proto and set opaque refers to FeatureInstance
    feature_value_t feature_object = JS_NewObjectProtoClass(static_cast<feature_context_ref>(ctx), unit.proto->js_proto, class_id);
    feature_set_opaque(feature_object, featureInstance.get());

    // setup featureInstance WeakRef, refers to feature_object
    // unit.proto->instances[iid]->js_self = WreakRef(feature_object);

    WeakRefInit(ctx, feature_object);
    // insert into instances array, update iid
    int iid = unit.proto->addInstance(std::move(featureInstance));
    unit.proto->instances[iid]->iid = iid;
    if (unit.proto->description->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        unit.proto->description->native_callbacks->onRequired(ctx, unit.proto->instances[iid].get());
    }

    return feature_object;
}
}
