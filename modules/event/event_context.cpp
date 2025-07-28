/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "event_context.h"
#include "event.h"
#include "topics/user_topic.h"
#include "uv.h"
#include <cstring>

namespace ft_system_event {
struct event_subs_info_t {
    int cb;
    int subs_id;
    struct weakref_list_node node;
    void init(int callback, int id)
    {
        cb = callback;
        subs_id = id;
        weakref_list_initialize(&node);
    }
    void uinit(FeatureInstanceHandle handle)
    {
        weakref_list_delete(&node);
        if (handle)
            FeatureRemoveCallback(handle, cb);
    }
};

static std::map<int, event_subs_info_t*> __subs_map = {};
static bool __add_subs_info_global(event_subs_info_t* info)
{
    if (!__subs_map.empty() && __subs_map.find(info->subs_id) != __subs_map.end()) {
        EVENT_CONTEXT_ERROR("addSubsInfo fail! %s already exists", info->subs_id);
        return false;
    }
    __subs_map[info->subs_id] = info;
    return true;
}

bool instanceMatchId(FeatureInstanceHandle handle, int id)
{
    if (!__subs_map.empty()) {
        auto it = __subs_map.find(id);
        if (it != __subs_map.end()) {
            return FeatureCheckCallbackId(handle, it->second->cb);
        }
    }
    EVENT_CONTEXT_ERROR("current instance not match id");
    return false;
}

std::map<std::string, int> __event_subs_info = {};

void printEventInfo()
{
    for (auto it = __event_subs_info.begin(); it != __event_subs_info.end(); it++) {
        EVENT_CONTEXT_DEBUG("eventName = %s, subs_count = %d", it->first.c_str(), it->second);
    }
}

void addEventSubsInfoGlobal(const char* eventName)
{
    if (!__event_subs_info.empty() && __event_subs_info.find(eventName) != __event_subs_info.end()) {
        __event_subs_info[eventName]++;
    } else {
        __event_subs_info[eventName] = 1;
    }
    EVENT_CONTEXT_DEBUG("addEventSubsInfoGlobal, eventName = %s, subs_count = %d", eventName, __event_subs_info[eventName]);
}

void removeEventSubsInfoGlobal(const char* eventName)
{
    if (!__event_subs_info.empty() && __event_subs_info.find(eventName) != __event_subs_info.end()) {
        __event_subs_info[eventName]--;
        EVENT_CONTEXT_DEBUG("removeEventSubsInfoGlobal, eventName = %s, subs_count = %d", eventName, __event_subs_info[eventName]);
        if (__event_subs_info[eventName] == 0) {
            __event_subs_info.erase(eventName);
        }
    }
}

bool isEventSubscribed(const char* eventName)
{
    if (!__event_subs_info.empty()) {
        if (__event_subs_info.find(eventName) != __event_subs_info.end() && __event_subs_info[eventName] != 0) {
            return true;
        }
    }
    return false;
}

void removeSubscribe(FeatureInstanceHandle handle, int id)
{
    EVENT_CONTEXT_DEBUG("removeSubscribe, id = %d, handle = %p", id, handle);
    if (!__subs_map.empty()) {
        auto it = __subs_map.find(id);
        if (it != __subs_map.end()) {
            it->second->uinit(handle);
            free(it->second);
            __subs_map.erase(it);
            return;
        }
    }
    EVENT_CONTEXT_ERROR("no such subscription info");
}

bool isSystemEvent(const char* eventName)
{
    return strncmp(eventName, SYSTEM_EVENT_PREFIX, strlen(SYSTEM_EVENT_PREFIX)) == 0;
}

static void __event_topic_cb(uv_topic_t* topic, int status, void* data,
    size_t datalen)
{
    EVENT_CONTEXT_DEBUG("__event_topic_cb, status = %d", status);
    if (status == 0 && data != NULL) {
        TopicListener* listener = static_cast<TopicListener*>(topic->handle.data);
        listener->handleTopicData(data, datalen);
    } else {
        EVENT_CONTEXT_ERROR("__event_topic_cb get wrong status: %s", status);
    }
}

static int __eventSubsId = 0;
static int __get_new_event_subs_id()
{
    int tmp = __eventSubsId;
    __eventSubsId++;
    return tmp;
}

EventContext::EventContext(const char* name, deal_func dfunc)
{
    event_name.assign(name);
    func = dfunc;
    weakref_list_initialize(&subs_linklist);
}

EventContext::~EventContext()
{
    event_subs_info_t *subs_info, *temp = NULL;
    weakref_list_for_every_entry_safe(&subs_linklist, subs_info, temp, event_subs_info_t, node)
    {
        removeSubscribe(NULL, subs_info->subs_id);
        removeEventSubsInfoGlobal(event_name.c_str());
    }
}

int EventContext::addEventSubsInfo(int cb)
{
    event_subs_info_t* subs_info = (event_subs_info_t*)malloc(sizeof(event_subs_info_t));
    subs_info->init(cb, __get_new_event_subs_id());
    if (__add_subs_info_global(subs_info)) {
        addEventSubsInfoGlobal(event_name.c_str());
        weakref_list_add_tail(&subs_linklist, &subs_info->node);
    } else {
        subs_info->uinit(nullptr);
        free(subs_info);
        return -1;
    }

    return subs_info->subs_id;
}

void EventContext::handleEvent(FeatureInstanceHandle handle, void* pre, void* cur)
{
    EVENT_CONTEXT_DEBUG("EventContext::handleEvent, pkg = %s", FeatureGetPackageName(FeatureGetProtoHandle(handle)));
    EVENT_CONTEXT_DEBUG("event_name = %s", event_name.c_str());
    if (!isSystemEvent(event_name.c_str()) && strcmp(event_name.c_str(), ((user_event_meta*)cur)->eventName) != 0) {
        EVENT_CONTEXT_DEBUG("user event_name [%s] not match", ((user_event_meta*)cur)->eventName);
        return;
    }

    ft_value_t res;
    // if the corresponding event value has changed, the call its cb_id
    if (!FeatureInstanceIsDetached(handle) && func(handle, pre, cur, res)) {
        EVENT_CONTEXT_DEBUG("call js callback");
        system_event_subs_cb_t* arg = system_eventMallocsubs_cb_t();
        arg->params = &res;
        char* pkgName = (char*)FeatureMalloc(PATH_MAX, FT_CHAR);
        if (isSystemEvent(event_name.c_str())) {
            sprintf(pkgName, "%s", SYSTEM_EVENT_PACKAGE);
        } else {
            sprintf(pkgName, "%s", ((user_event_meta*)cur)->pkg);
        }
        arg->package = pkgName;
        event_subs_info_t *subs_info, *temp = NULL;
        weakref_list_for_every_entry_safe(&subs_linklist, subs_info, temp, event_subs_info_t, node)
        {
            EVENT_CONTEXT_DEBUG("call js cb, id =  %d", subs_info->cb);
            FeatureInvokeCallback(handle, subs_info->cb, arg);
        }
        ft_free_value(FeatureGetContext(handle), *arg->params);
        arg->params = NULL;
        FeatureFreeValue(arg);
    }
}

SubscriptionManager::SubscriptionManager(FeatureInstanceHandle handle)
{
    instance_handle = FeatureDupInstanceHandle(handle);
}
SubscriptionManager::~SubscriptionManager()
{
    EVENT_CONTEXT_DEBUG("free SubscriptionManager, handle = %p", instance_handle);
    for (auto it = events.begin(); it != events.end(); it++) {
        delete it->second;
    }
    events.clear();
    FeatureFreeInstanceHandle(instance_handle);
}

int SubscriptionManager::addEvent(const char* eventName, int cb)
{
    EVENT_CONTEXT_DEBUG("SubscriptionManager::addEvent, eventName = %s, %p", eventName, instance_handle);
    if (events.empty() || events.find(std::string(eventName)) == events.end()) {
        const event_info_t* event_info = GetEventInfoGlobal(eventName);
        EventContext* event_ctx;
        if (event_info != nullptr) {
            event_ctx = new EventContext(eventName, event_info->func);
        } else if (!isSystemEvent(eventName)) {
            event_ctx = new EventContext(eventName, GetUserEventInfo()->func);
        } else {
            EVENT_CONTEXT_ERROR("current topic dont support event [%s]", eventName);
            return -1;
        }
        events[std::string(eventName)] = event_ctx;
    }
    EventContext* event_ctx = events[std::string(eventName)];
    return event_ctx->addEventSubsInfo(cb);
}

void SubscriptionManager::handleTopicEvent(void* pre, void* data)
{
    EVENT_CONTEXT_DEBUG("handleTopicEvent, instance_handle = %p", instance_handle);
    if (events.empty()) {
        EVENT_CONTEXT_DEBUG("events in cur SubscriptionManager is empty.");
        return;
    }
    for (auto it = events.begin(); it != events.end(); it++) {
        it->second->handleEvent(instance_handle, pre, data);
    }
}

void __free_topic_resource(uv_handle_t* handle)
{
    EVENT_CONTEXT_DEBUG("%p handle is closed", handle);
    TopicListener* listener = static_cast<TopicListener*>(handle->data);
    delete listener;
}

#define UV_HANDLE_INTERNAL 0x00000010
static int uv_topic_close_internal(uv_topic_t* topic)
{
    topic->handle.flags |= UV_HANDLE_INTERNAL;
    uv_close((uv_handle_t*)&topic->handle, __free_topic_resource);
    return 0;
}

TopicListener::TopicListener(orb_id_t meta_id)
    : meta(meta_id)
    , pre(nullptr)
{
}

int TopicListener::subscribeTopic(uv_loop_t* loop)
{
    topic.handle.data = this;
    return uv_topic_subscribe(
        loop,
        &topic,
        meta,
        __event_topic_cb);
}

int TopicListener::subscribeEvent(FeatureInstanceHandle handle, const char* eventName, int cb)
{
    if (subs_man.empty() || subs_man.find(handle) == subs_man.end()) {
        // create one SubscriptionManager
        SubscriptionManager* man = new SubscriptionManager(handle);
        subs_man[handle] = man;
    }
    return subs_man[handle]->addEvent(eventName, cb);
}

void TopicListener::handleTopicData(void* data, int datalen)
{
    if (!subs_man.empty()) {
        for (auto it = subs_man.begin(); it != subs_man.end(); it++) {
            it->second->handleTopicEvent(pre, data);
        }
    }
    if (pre == nullptr) {
        pre = malloc(datalen);
    }
    memcpy(pre, data, datalen);
}

void TopicListener::detachHandleResource(FeatureInstanceHandle handle)
{
    EVENT_CONTEXT_DEBUG("TopicListener::detachHandleResource, handle = %p", handle);
    if (!subs_man.empty()) {
        auto it = subs_man.find(handle);
        if (it != subs_man.end()) {
            // it->second->freeSubscriptionManager();
            delete it->second;
            subs_man.erase(it);
        }
    }
}

void TopicListener::freeListener()
{
    if (uv_topic_unsubscribe(&topic) != 0) {
        EVENT_CONTEXT_ERROR("topic unsubscribe fail... ");
    }
    if (pre != nullptr)
        free(pre);

    if (!subs_man.empty()) {
        for (auto it = subs_man.begin(); it != subs_man.end(); it++) {
            delete it->second;
        }
        subs_man.clear();
    }
    uv_topic_close_internal(&topic);
}

int EventManager::createTopicListener(FeatureInstanceHandle handle, const topic_info_t* info)
{
    EVENT_CONTEXT_DEBUG("createTopicListener %p", info->meta);
    // create a new topic context and subscribe the topic
    TopicListener* listener = new TopicListener(info->meta);
    int res = listener->subscribeTopic(FeatureGetUVLoop(FeatureGetManagerHandleFromInstance(handle)));
    if (res < 0) {
        // subscribe fail
        EVENT_CONTEXT_ERROR("subscribe fail... %p", info->meta);
        delete listener;
        return res;
    }
    EVENT_CONTEXT_DEBUG("subscribe success! %p", info->meta);
    _topics_map[info] = listener;
    return 0;
}

int EventManager::subscribeEvent(FeatureInstanceHandle handle, const char* eventName, int cb)
{
    EVENT_CONTEXT_DEBUG("subscribeEvent, event = %s, cb = %d", eventName, cb);
    const topic_info_t* topic_info;
    if (isSystemEvent(eventName)) {
        topic_info = GetTopicInfoGlobal(eventName);
    } else {
        topic_info = GetUserToopicInfo();
    }
    if (topic_info != nullptr) {
        if (_topics_map.empty() || _topics_map.find(topic_info) == _topics_map.end()) {
            int res = createTopicListener(handle, topic_info);
            if (res < 0) {
                return res;
            }
        }
        return _topics_map[topic_info]->subscribeEvent(handle, eventName, cb);
    } else {
        EVENT_CONTEXT_ERROR("unsupport event %s", eventName);
    }
    return -1;
}

void EventManager::detachHandleResource(FeatureInstanceHandle handle)
{
    EVENT_CONTEXT_DEBUG("EventManager::detachHandleResource, handle = %p", handle);
    if (_topics_map.empty())
        return;
    for (auto it = _topics_map.begin(); it != _topics_map.end(); it++) {
        it->second->detachHandleResource(handle);
    }
}
EventManager::~EventManager()
{
    EVENT_CONTEXT_DEBUG("EventManager::~EventManager");
    if (_topics_map.empty())
        return;
    for (auto it = _topics_map.begin(); it != _topics_map.end(); it++) {
        it->second->freeListener();
        // the TopicListener object should be released in __free_topic_resource
        // free(it->second);
    }
    _topics_map.clear();
    if (!__subs_map.empty()) {
        EVENT_CONTEXT_ERROR("__subs_map is not empty!");
    }
}
} // namespace ft_system_event
