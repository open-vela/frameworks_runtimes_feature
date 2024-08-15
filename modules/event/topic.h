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

#ifndef FEATURE_TOPIC_H
#define FEATURE_TOPIC_H

#include "feature_exports.h"
#include "feature_utils.h"
#include "uORB/uORB.h"
#include <cstdlib>

namespace ft_system_event {
#define TOPICS(V) \
    V(user_topic) \
    V(battery_topic)

#define DECLARE_EXTERN_TOPIC(TOPIC) extern "C" const topic_info_t feature_event_##TOPIC

typedef bool (*deal_func)(FeatureInstanceHandle handle, void* pre, void* cur, ft_value_t& res);

struct event_info_t {
    const char* event_name;
    deal_func func;
};

struct topic_info_t {
    orb_id_t meta;
    const uint event_count;
    const event_info_t* events;
};

const topic_info_t* GetTopicInfoGlobal(const char* event);
const topic_info_t* GetUserToopicInfo();
const event_info_t* GetEventInfoGlobal(const char* event);
const event_info_t* GetUserEventInfo();
bool CheckEventValidity(const char* event);
} // namespace ft_system_event

#endif // FEATURE_TOPIC_H