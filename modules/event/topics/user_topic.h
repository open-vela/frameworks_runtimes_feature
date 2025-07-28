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

#ifndef FEATURE_USER_TOPIC_H
#define FEATURE_USER_TOPIC_H

#include "uORB/uORB.h"
/**
 * Struct meta of system.event.publish
 * maximum size is 264
 */
struct user_event_meta {
    char eventName[32];
    char pkg[32];
    char params[128];
    char permissions[64];
};

ORB_DECLARE(user_event_meta);

#define USER_EVENT_META ORB_ID(user_event_meta)

#endif // FEATURE_USER_TOPIC_H