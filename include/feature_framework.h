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
#include "feature.h"

#include <map>
#include <memory>
#include <vector>

namespace ferry {

#define weakref_container_of(ptr, type, member) \
    ((type*)((uintptr_t)(ptr)-offsetof(type, member)))

struct weakref_list_node {
    struct weakref_list_node* prev;
    struct weakref_list_node* next;
};

#define weakref_list_initialize(list)              \
    do {                                           \
        struct weakref_list_node* __list = (list); \
        __list->prev = __list->next = __list;      \
    } while (0)

#define weakref_list_delete(item)                  \
    do {                                           \
        struct weakref_list_node* __item = (item); \
        __item->next->prev = __item->prev;         \
        __item->prev->next = __item->next;         \
        __item->prev = __item->next = NULL;        \
    } while (0)

#define weakref_list_add_tail(list, item)          \
    do {                                           \
        struct weakref_list_node* __list = (list); \
        struct weakref_list_node* __item = (item); \
        __item->prev = __list->prev;               \
        __item->next = __list;                     \
        __list->prev->next = __item;               \
        __list->prev = __item;                     \
    } while (0)

#define weakref_list_for_every_entry_safe(list, entry, temp, type, member) \
    for (entry = weakref_container_of((list)->next, type, member),         \
        temp = weakref_container_of(entry->member.next, type, member);     \
         &entry->member != (list); entry = temp,                           \
        temp = weakref_container_of(temp->member.next, type, member))

struct WeakRef {
    feature_value_t js_value; //引用一个对象的指针(纯C指针),将其指向proto
    struct weakref_list_node link;
};

struct FeatureInstance {
    WeakRef js_self; //指向feature object的弱引用
    void* native; // 绑定的实例数据
    struct FeaturePrototype* proto; //指向内部的proto信息
    int iid; // the instance id, order in instances aray.
    FEATURE::FeatureCallbackId curr_cid = 0;
    std::map<FEATURE::FeatureCallbackId, std::pair<feature_value_t, CallbackType*>> callbacks; // instance should save feature resources
    std::map<FEATURE::FeaturePromiseHandle, FeaturePromiseData*> promises;   // all promises created by native feature

    FeatureInstance(struct FeaturePrototype* featurePrototype);

    ~FeatureInstance();

    std::pair<feature_value_t, CallbackType*> getCallback(FEATURE::FeatureCallbackId id);

    /**
     * @brief add callback to instance
     *
     * @param ctx
     * @param value
     * @param callbackType
     * @return FEATURE::FeatureCallbackId
     */
    FEATURE::FeatureCallbackId addCallback(feature_value_t value, CallbackType* callbackType);

    /**
     * @brief remove callback from instance vai FeatureCallbackId
     *
     & @param ctx
     * @param id
     * @return true
     * @return false
     */
    bool removeCallback(FEATURE::FeatureCallbackId id);

    /**
     * @brief Get the Promise object
     *
     * @param promiseHandle
     * @return FEATURE::FeaturePromiseData*
     */
    FeaturePromiseData* getPromise(FEATURE::FeaturePromiseHandle promiseHandle);

    FEATURE::FeaturePromiseHandle addPromise(FeaturePromiseData* data);

    bool removePromise(FEATURE::FeaturePromiseHandle promiseHandle);

};

/**
 * @brief Feature Protoype struct
 * all informations needed by JS prototype is saved in it
 *
 */
struct FeaturePrototype {
    context_ref ctx; // js context
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
}

#endif
