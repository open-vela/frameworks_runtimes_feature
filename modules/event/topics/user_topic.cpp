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

#include "user_topic.h"
#include "event/topic.h"
#include "feature.h"

using namespace ft_system_event;
ORB_DEFINE(user_event_meta, struct user_event_meta, NULL);

static bool deal_user_event(FeatureInstanceHandle handle, void* pre, void* cur, ft_value_t& res)
{
    user_event_meta* data = static_cast<user_event_meta*>(cur);
    const char* pkg = FeatureGetPackageName(FeatureGetProtoHandle(handle));
    ft_context_ref ft_ctx = FeatureGetContext(handle);
    FEATURE_LOG_DEBUG("deal_user_event, params = %s", data->params);
    if (strcmp(data->pkg, pkg) == 0 || strstr(data->permissions, pkg) != NULL) {
        if (strlen(data->params) > 0) {
            res = ft_parse_json(ft_ctx, data->params, strlen(data->params), NULL);
        } else {
            res = ft_undefined(ft_ctx);
        }
        return true;
    }
    return false;
}

static const event_info_t user_event = {
    "USER_EVENT",
    deal_user_event
};

DECLARE_EXTERN_TOPIC(user_topic) = {
    ORB_ID(user_event_meta),
    1,
    &user_event
};
