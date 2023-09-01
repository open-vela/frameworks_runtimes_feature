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
#include "feature_registry.h"
#include "feature_framework.h"
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_utils.h"
#if defined(CONFIG_QUICKAPP)
#include "aiotjs.h"
#endif
#include "feature.h"
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
static bool createJsInstanceClass(context_ref ctx, const char* class_name);
static context_ref getContext(feature_runtime_ref rt);

FeatureManager::FeatureManager(FeatureRegistry* registry)
    : registry_(registry)
{
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
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_INFO("instance or prototype is null...");
        return;
    }
    auto proto = instance->prototype();

    feature_set_opaque(val, nullptr);
    // get proto pointer, it may not be deleted at this time

    //遍历proto->weak_ref_list链表，将其中的js_value设置为JSE_UNDEFINED
    WeakRef* node;
    WeakRef* node_temp;
    weakref_list_for_every_entry_safe(&proto->weak_ref_list, node, node_temp, WeakRef, link)
    {
        node->js_value = FEATURE_VALUE_UNDEFINED;
    }

    auto iid = instance->instanceId();
    // invoke callback
    if (proto->description->native_callbacks->onDetached) {
        FEATURE_LOG_DEBUG("invoke onDettached callback...");
        proto->description->native_callbacks->onDetached(getContext(rt), instance);
    }
    // delete instance by removing it from FeaturePrototype.
    bool ret = proto->removeInstance(iid);
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
    if (!instance || !instance->prototype()) {
        FEATURE_LOG_INFO("instance or prototype is null...");
        return;
    }

    auto proto = instance->prototype();
    // mark callbacks
    for (auto& pair : instance->callbacks) {
        feature_mark_value(rt, pair.second.cb, mark_func);
    }
    // mark promies
    for(auto& pair : instance->promises) {
        feature_mark_value(rt, pair.second->promise, mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[0], mark_func);
        feature_mark_value(rt, pair.second->resolveFuncs[1], mark_func);
    }
    // should mark prototype object.
    feature_mark_value(rt, proto->js_proto, mark_func);
}

static bool createJsInstanceClass(context_ref ctx, const char* class_name)
{
    FEATURE_CHECK_NE(class_name, nullptr);
    FEATURE_LOG_DEBUG("create PrototypeClass: %s.", class_name);
    // FEATURE_CHECK_EQ(featurePrototype->class_id, 0);
    JS_NewClassID(&class_id);
    FEATURE_CHECK_NE(class_id, 0); // it must not 0 now
    // fill the class_def structure
    class_def = { .class_name = class_name, .finalizer = __feature_finalizer, .gc_mark = __feature_mark };
    // create native feature prototype class defination
    JS_NewClass(feature_get_runtime(static_cast<feature_context_ref>(ctx)), class_id, &class_def);
    return true;
}

static FeaturePrototype* createFeaturePrototype(context_ref ctx, FeatureDescription* description)
{
    FEATURE_CHECK_NE(description, nullptr);
    FEATURE_LOG_DEBUG("create FeaturePrototype for description: %s.", description->name);
    if (!createJsInstanceClass(ctx, "FeatureInstanceObject")) {
        FEATURE_LOG_ERROR("create js Instance class for feature %s.", description->name);
        return nullptr;
    }
    return new FeaturePrototype(ctx, description);
}

feature_value_t FeatureManager::featureRequire(context_ref ctx, const char* name)
{
    FEATURE_LOG_DEBUG("featureRequire for '%s'", name);
    FeatureUnit* unit = registry_->findFeature(name);
    if (!unit || !unit->description) {
        FEATURE_LOG_WARN("can't find native feature '%s', fallback to original JS module load!", name);
        return FEATURE_VALUE_UNDEFINED;
    }

    if (!unit->proto) {
        // create proto
        unit->proto = createFeaturePrototype(ctx, unit->description);
        if (!unit->proto) {
            FEATURE_LOG_ERROR("createFeaturePrototype failed !");
            return JS_UNDEFINED;
        }
    }

    auto proto = unit->proto;
    // create prototype class instance
    if (feature_is_undefined(proto->js_proto)) {
        feature_value_t js_proto_obj = feature_object(static_cast<feature_context_ref>(ctx));
        if (feature_is_exception(js_proto_obj)) {
            feature_dump_error(static_cast<feature_context_ref>(ctx));
            return FEATURE_VALUE_UNDEFINED;
        }
        // TODO: initialize js_proto using description
        initialize_prototype(ctx, unit, js_proto_obj);
        proto->js_proto = js_proto_obj;
        if (proto->description->native_callbacks->onCreate) {
            FEATURE_LOG_DEBUG("invoke onCreate callback...");
            proto->description->native_callbacks->onCreate(ctx, proto);
        }
    }

    // create feature instance for the required object
    auto featureInstance = std::make_unique<FeatureInstanceQjs>(proto);
    // create object with proto and set opaque refers to FeatureInstance
    feature_value_t feature_object = JS_NewObjectProtoClass(static_cast<feature_context_ref>(ctx), proto->js_proto, class_id);
    feature_set_opaque(feature_object, featureInstance.get());

    // setup featureInstance WeakRef, refers to feature_object
    // proto->instances[iid]->js_self = WreakRef(feature_object);

    WeakRefInit(ctx, feature_object);
    // insert into instances array, update iid
    int iid = proto->addInstance(std::move(featureInstance));
    proto->instances[iid]->setInstanceId(iid);
    if (proto->description->native_callbacks->onRequired) {
        FEATURE_LOG_DEBUG("invoke onRequired callback...");
        proto->description->native_callbacks->onRequired(ctx, proto->instances[iid].get());
    }

    return feature_object;
}
}
