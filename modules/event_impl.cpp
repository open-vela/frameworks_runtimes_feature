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

#include "event.h"
#include "event/event_context.h"
#include "event/topics/user_topic.h"
#include "modules/modules_utils.h"
#include "uv_ext.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <uv.h>

using namespace ft_system_event;

#define EVENT_DEBUG(fmt, ...) \
    FEATURE_LOG_DEBUG(fmt, ##__VA_ARGS__)
#define EVENT_ERROR(fmt, ...) \
    FEATURE_LOG_ERROR(fmt, ##__VA_ARGS__)
#define EVENT_LIFECYCLE_DEBUG() \
    EVENT_DEBUG("[jidl_feature] Event_impl::%s()", __FUNCTION__)

void system_event_onRegister(const char* feature_name)
{
    EVENT_LIFECYCLE_DEBUG();
}
void system_event_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    EVENT_LIFECYCLE_DEBUG();
    EventManager* th = (EventManager*)FeatureGetProtoData(handle);
    if (th == nullptr) {
        th = new EventManager();
        if (!th) {
            EVENT_ERROR("malloc fail...");
            return;
        }
        FeatureSetProtoData(handle, th);
    }
}
void system_event_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    EVENT_LIFECYCLE_DEBUG();
}
void system_event_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    EVENT_LIFECYCLE_DEBUG();
    EventManager* th = (EventManager*)FeatureGetProtoData(FeatureGetProtoHandle(handle));
    if (th) {
        th->detachHandleResource(handle);
    }
}
void system_event_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    EVENT_LIFECYCLE_DEBUG();
    EventManager* th = (EventManager*)FeatureGetProtoData(handle);
    if (th) {
        delete th;
    }
}
void system_event_onUnregister(const char* feature_name)
{
    EVENT_LIFECYCLE_DEBUG();
}

#define __GET_EVENT_HANDLE_MANAGER__()                                                     \
    EventManager* th = (EventManager*)FeatureGetProtoData(FeatureGetProtoHandle(feature)); \
    if (!th) {                                                                             \
        EVENT_ERROR("get EventManager is null!");                                          \
    }

void system_event_wrap_publish(FeatureInstanceHandle feature, union AppendData append_data, system_event_publish_t* param)
{
    EVENT_DEBUG("system_event_wrap_publish, event = %s", param->eventName);
    if (!param->eventName || strlen(param->eventName) == 0) {
        EVENT_ERROR("eventName length is 0, publish fail");
        return;
    }
    if (isSystemEvent(param->eventName)) {
        EVENT_ERROR("event %s is system event, publish fail", param->eventName);
        return;
    }
    __GET_EVENT_HANDLE_MANAGER__();
    printEventInfo();
    if (!isEventSubscribed(param->eventName)) {
        EVENT_ERROR("event %s is not subscribed before, publish fail", param->eventName);
        return;
    }

    const char *pkg_name, *options_params = nullptr;
    char* pos;
    char permissions[PATH_MAX] = "";
    ft_type params_t;
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    user_event_meta* data = (user_event_meta*)FeatureMalloc(sizeof(user_event_meta), FT_CHAR);

    if (strlen(param->eventName) < sizeof(data->eventName)) {
        sprintf(data->eventName, "%s", param->eventName);
    } else {
        EVENT_ERROR("event name [%s] should shorter than %d", param->eventName, sizeof(data->eventName));
        goto free_value;
    }
    pkg_name = FeatureGetPackageName(FeatureGetProtoHandle(feature));
    if (pkg_name && strlen(pkg_name) < sizeof(data->pkg)) {
        sprintf(data->pkg, "%s", pkg_name);
    } else {
        EVENT_ERROR("package name [%s] should shorter than %d", pkg_name, sizeof(data->eventName));
        goto free_value;
    }

    if (param->options) {
        if (param->options->params) {
            params_t = ft_get_type(ft_ctx, *(param->options->params));
            if (params_t > 0) {
                if (params_t == FT_TYPE_OBJECT) {
                    options_params = ft_to_string(ft_ctx, *(param->options->params));
                    if (options_params && strlen(options_params) > 0) {
                        if (strlen(options_params) < sizeof(data->params)) {
                            sprintf(data->params, "%s", options_params);
                            ft_free_string(ft_ctx, options_params);
                        } else {
                            EVENT_ERROR("params length should shorter than %d", sizeof(data->eventName));
                            ft_free_string(ft_ctx, options_params);
                            goto free_value;
                        }
                    }
                } else {
                    EVENT_ERROR("wrong params type %d", params_t);
                    goto free_value;
                }
            }
        }

        if (param->options->permissions) {
            FTArrayHelper<const char*> permissions_array(param->options->permissions);
            pos = permissions;
            for (int32_t i = 0; i < permissions_array.size(); i++) {
                if (i > 0) {
                    sprintf(pos, "%s", ",");
                    pos++;
                }
                if (permissions_array[i]) {
                    sprintf(pos, "%s", permissions_array[i]);
                    pos += strlen(permissions_array[i]);
                }
            }
            EVENT_DEBUG("permissions = %s", permissions);
            if (strlen(permissions) < sizeof(data->permissions)) {
                snprintf(data->permissions, strlen(permissions) + 1, "%s", permissions);
            } else {
                EVENT_ERROR("permissions length should shorter than %d", sizeof(data->permissions));
                goto free_value;
            }
            EVENT_DEBUG("publish, permmisons = %s", data->permissions);
        }
    }
    if (uv_topic_publish(USER_EVENT_META, data) < 0) {
        goto free_value;
    }
    FeatureFreeValue(data);
    return;
free_value:
    EVENT_ERROR("publish user event [%s] fail", param->eventName);
    FeatureFreeValue(data);
}

FtAny system_event_wrap_subscribe(FeatureInstanceHandle feature, union AppendData append_data, system_event_sub_t* param)
{
    ft_value_t* ret_ptr = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    ft_context_ref ctx = FeatureGetContext(feature);
    if (!param->eventName || strlen(param->eventName) == 0 || !FeatureCheckCallbackId(feature, param->callback)) {
        EVENT_ERROR("parameter [%s or callback %d] error!", param->eventName, param->callback);
        *ret_ptr = ft_undefined(ctx);
        return ret_ptr;
    }
    __GET_EVENT_HANDLE_MANAGER__();
    EVENT_DEBUG("subscribe event %s", param->eventName);
    int res = th->subscribeEvent(feature, param->eventName, param->callback);
    if (res < 0) {
        EVENT_ERROR("subscribe event [%s] fail. %d", param->eventName, res);
        *ret_ptr = ft_undefined(ctx);
    } else {
        *ret_ptr = ft_from_int(ctx, res);
    }
    return ret_ptr;
}

void system_event_wrap_unsubscribe(FeatureInstanceHandle feature, union AppendData append_data, system_event_unsubs_t* param)
{
    if (instanceMatchId(feature, param->id))
        removeSubscribe(feature, param->id);
}
