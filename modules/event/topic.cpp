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

#include "topic.h"
#include "string.h"
#include <cstddef>

namespace ft_system_event {

#define USE_TOPIC(TOPIC) &feature_event_##TOPIC,
#define EXTERN_TOPIC(TOPIC) DECLARE_EXTERN_TOPIC(TOPIC);

TOPICS(EXTERN_TOPIC)

const topic_info_t* feature_topics[] = {
    TOPICS(USE_TOPIC)
        NULL
};

const topic_info_t* GetTopicInfoGlobal(const char* event)
{
    for (uint i = 0; feature_topics[i] != NULL; i++) {
        for (uint j = 0; j < feature_topics[i]->event_count; j++) {
            if (strcmp(event, feature_topics[i]->events[j].event_name) == 0) {
                return feature_topics[i];
            }
        }
    }
    return nullptr;
}

const topic_info_t* GetUserToopicInfo()
{
    return feature_topics[0];
}

const event_info_t* GetEventInfoGlobal(const char* event)
{
    for (uint i = 0; feature_topics[i] != NULL; i++) {
        for (uint j = 0; j < feature_topics[i]->event_count; j++) {
            if (strcmp(event, feature_topics[i]->events[j].event_name) == 0) {
                return &feature_topics[i]->events[j];
            }
        }
    }
    return nullptr;
}

const event_info_t* GetUserEventInfo()
{
    return feature_topics[0]->events;
}

bool CheckEventValidity(const char* event)
{
    if (GetTopicInfoGlobal(event) != nullptr)
        return true;
    return false;
}

} // namespace ft_system_event
