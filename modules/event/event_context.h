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

#ifndef EVENT_CONTEXT_H
#define EVENT_CONTEXT_H

#include "feature_exports.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "topic.h"
#include "uORB/uORB.h"
#include "uv_ext.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace ft_system_event {
#define SYSTEM_EVENT_PACKAGE "system"
#define SYSTEM_EVENT_PREFIX "usual.event."

#define EVENT_CONTEXT_DEBUG(fmt, ...) \
    FEATURE_LOG_DEBUG(fmt, ##__VA_ARGS__)
#define EVENT_CONTEXT_ERROR(fmt, ...) \
    FEATURE_LOG_ERROR(fmt, ##__VA_ARGS__)

// Check if current user event has been subscribed
bool isEventSubscribed(const char* eventName);

bool isSystemEvent(const char* eventName);

void printEventInfo();

bool instanceMatchId(FeatureInstanceHandle handle, int id);
/**
 * unsubscribe event by id
 */
void removeSubscribe(FeatureInstanceHandle handle, int id);

class EventContext {
public:
    EventContext(const char* name, deal_func dfunc);
    ~EventContext();
    // Add event subscription information in this event
    int addEventSubsInfo(int cb);
    // Handle data from topic
    void handleEvent(FeatureInstanceHandle handle, void* pre, void* cur);

private:
    std::string event_name; // current event name subscribed by user
    deal_func func; // deal the new data from uorb topic
    struct weakref_list_node subs_linklist; // record the list of subsribe info of current event
};

/**
 * SubscriptionManager records events subscription info in a FeatureInstanceHandle
 */
class SubscriptionManager {
public:
    SubscriptionManager(FeatureInstanceHandle handle);
    ~SubscriptionManager();
    // Add subscribeable events to the current SubscriptionManager
    int addEvent(const char* eventName, int cb);
    // Handle data from topic
    void handleTopicEvent(void* pre, void* data);

private:
    FeatureInstanceHandle instance_handle;
    std::map<std::string, EventContext*> events;
};

/**
 * TopicListener records topic subscription info of uorb topic
 */
class TopicListener {
public:
    TopicListener(orb_id_t meta_id);
    // Subscribe the topic from uorb
    int subscribeTopic(uv_loop_t* loop);
    // Subscribe an event in current topic
    int subscribeEvent(FeatureInstanceHandle handle, const char* eventName, int cb);
    /**
     * receive topic data from UORB topic, and dispatch it
     * travel all the events in this topic
     */
    void handleTopicData(void* data, int datalen);
    // Detach resource that relevant with this FeatureInstanceHandle
    void detachHandleResource(FeatureInstanceHandle handle);
    // Free the resources in TopicListener
    void freeListener();

private:
    const orb_id_t meta;
    void* pre;
    uv_topic_t topic;
    std::map<FeatureInstanceHandle, SubscriptionManager*> subs_man;
};

class EventManager {
public:
    ~EventManager();
    // Create a new Topic Listener
    int createTopicListener(FeatureInstanceHandle handle, const topic_info_t* info);
    int subscribeEvent(FeatureInstanceHandle handle, const char* eventName, int cb);
    // Detach resource that relevant with this FeatureInstanceHandle
    void detachHandleResource(FeatureInstanceHandle handle);

private:
    // record all subscription topics, key of _topics_map is TOPIC_ID
    std::map<const topic_info_t*, TopicListener*> _topics_map;
};
} // namespace ft_system_event

#endif
