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

#include <nuttx/nuttx.h>
#include <queue>
#include <sensor/gnss.h>

#include "geolocation.h"
#include "uv.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] geolocation_impl";
#define INVOKE_SUCCESS_CB(feature, cb, ...)                        \
    do {                                                           \
        if (!FeatureInvokeCallback(feature, cb, ##__VA_ARGS__)) {  \
            FEATURE_LOG_ERROR("invoke success callback failed !"); \
        }                                                          \
    } while (0)

#define INVOKE_FAIL_CB(feature, cb, msg, code)                  \
    do {                                                        \
        if (!FeatureInvokeCallback(feature, cb, msg, code)) {   \
            FEATURE_LOG_ERROR("invoke fail callback failed !"); \
        }                                                       \
    } while (0)

#define REMOVE_ALL_CALLBACK(__succ__, __fail__)   \
    do {                                          \
        FeatureRemoveCallback(feature, __succ__); \
        FeatureRemoveCallback(feature, __fail__); \
    } while (0)

#define PRECISION 100000

typedef enum ErrorCode {
    GENERAL = 200,
    ARGSERROR = 202,
    SERVICEUNAVAILABLE = 203,
    IOERROR = 300,
    TIMEOUT = 204
} ErrorCode;

struct GeoMetaData {
    FeatureInstanceHandle instance;
    FtCallbackId callback;
    FtCallbackId fail;
    FtCallbackId complete;
    int timeout;
    bool oneshot;
    bool subscribed;
};

struct gnss_event_t {
    uv_topic_t topic;
    GeoMetaData meta;
    uv_timer_t timer;
    bool isTimeout;
};

struct location_context {
    gnss_event_t event;
    std::queue<gnss_event_t*> getLocationQueue;
    system_geolocation_subscribeRet* subRet;
    system_geolocation_getLocationRet* getRet;
};

static void sensor_event_close_cb(uv_handle_t* handle)
{
    gnss_event_t* event = container_of(handle, gnss_event_t, timer);
    free(event);
}

static void sensor_topic_close_cb(uv_handle_t* handle)
{
    uv_topic_t* topic = container_of(handle, uv_topic_t, handle);
    gnss_event_t* event = container_of(topic, gnss_event_t, topic);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(FeatureGetProtoHandle(event->meta.instance)));
    if (event->isTimeout && event->meta.fail) {
        INVOKE_FAIL_CB(event->meta.instance, event->meta.fail, "get location timeout", TIMEOUT);
    }

    if (event->meta.complete) {
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.complete, "get location data complete");
        FeatureRemoveCallback(event->meta.instance, event->meta.complete);
    }

    FeatureRemoveCallback(event->meta.instance, event->meta.callback);
    FeatureRemoveCallback(event->meta.instance, event->meta.fail);
    if (event->meta.timeout > 0) {
        uv_timer_stop(&event->timer);
        uv_close((uv_handle_t*)&event->timer, sensor_event_close_cb);
        if (!context->getLocationQueue.empty()) {
            context->getLocationQueue.pop();
        }

        return;
    }
    free(event);
}

static void sensor_gnss_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    int cnt;
    int ret;
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }

    gnss_event_t* event = container_of(topic, gnss_event_t, topic);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(FeatureGetProtoHandle(event->meta.instance)));

    if (event->meta.oneshot) {
        sensor_gnss* ret_t = (sensor_gnss*)data;
        ft_context_ref ft_ctx = FeatureGetContext(event->meta.instance);
        ft_value_t* accuracyInfo = static_cast<ft_value_t*>(FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF));
        *accuracyInfo = ft_new_object(ft_ctx);

        ft_value_t horizontal = ft_from_double(ft_ctx, round(ret_t->hdop * PRECISION) / PRECISION);
        ft_value_t vertical = ft_from_double(ft_ctx, round(ret_t->vdop * PRECISION) / PRECISION);
        if (!isnormal(ret_t->altitude) || !isnormal(ret_t->latitude) || !isnormal(ret_t->longitude) || !isnormal(ret_t->ground_speed) || !isnormal(ret_t->eph) || !isnormal(ret_t->epv)) {
            FEATURE_LOG_ERROR("%s::%s() data is invalid", file_tag, __FUNCTION__);
            if (event->meta.fail) {
                INVOKE_FAIL_CB(event->meta.instance, event->meta.fail, "getLocation data invalid", GENERAL);
            }
            goto out;
        }

        ft_obj_set_property(ft_ctx, *accuracyInfo, "horizontal", horizontal);
        ft_obj_set_property(ft_ctx, *accuracyInfo, "vertical", vertical);
        context->getRet->latitude = round(ret_t->latitude * PRECISION) / PRECISION;
        context->getRet->longitude = round(ret_t->longitude * PRECISION) / PRECISION;
        context->getRet->altitude = round(ret_t->latitude * PRECISION) / PRECISION;
        context->getRet->accuracy = int(ret_t->eph);
        context->getRet->speed = round(ret_t->ground_speed * PRECISION) / PRECISION;
        context->getRet->accuracyInfo = accuracyInfo;
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, context->getRet);
    out:
        ft_free_value(ft_ctx, *accuracyInfo);
        FeatureFreeValue(accuracyInfo);
        ret = uv_topic_unsubscribe(topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe fail", file_tag, __FUNCTION__);
        }

        ret = uv_topic_close(topic, sensor_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
        }

        return;
    }

    if (context == nullptr) {
        FEATURE_LOG_ERROR("%s::%s() context is null", file_tag, __FUNCTION__);
        return;
    }
    cnt = datalen / sizeof(sensor_gnss);
    for (int i = 0; i < cnt; i++) {
        sensor_gnss* ret_t = (sensor_gnss*)data + i;
        if (!isnormal(ret_t->altitude) || !isnormal(ret_t->latitude) || !isnormal(ret_t->longitude) || !isnormal(ret_t->ground_speed) || !isnormal(ret_t->eph) || !isnormal(ret_t->epv)) {
            FEATURE_LOG_ERROR("%s::%s() data is invalid", file_tag, __FUNCTION__);
            if (event->meta.fail) {
                INVOKE_FAIL_CB(event->meta.instance, event->meta.fail, "getLocation data invalid", GENERAL);
            }
            return;
        }

        context->subRet->latitude = round(ret_t->latitude * PRECISION) / PRECISION;
        context->subRet->longitude = round(ret_t->longitude * PRECISION) / PRECISION;
        context->subRet->altitude = round(ret_t->latitude * PRECISION) / PRECISION;
        context->subRet->accuracy = int(ret_t->eph);
        context->subRet->speed = round(ret_t->ground_speed * PRECISION) / PRECISION;
        INVOKE_SUCCESS_CB(context->event.meta.instance, context->event.meta.callback, context->subRet);
    }
}

static void sensor_timer_cb(uv_timer_t* timer)
{
    int ret;
    gnss_event_t* event = container_of(timer, gnss_event_t, timer);
    event->isTimeout = true;
    ret = uv_topic_unsubscribe(&event->topic);
    if (ret < 0) {
        FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe fail", file_tag, __FUNCTION__);
    }

    ret = uv_topic_close(&event->topic, sensor_topic_close_cb);
    if (ret < 0) {
        FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
    }
}

void system_geolocation_wrap_getLocation(FeatureInstanceHandle feature, AppendData append_data, system_geolocation_LocationParam* param)
{
    int ret;
    int code;
    const char* msg = "";
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(proto_handle));

    gnss_event_t* event = (gnss_event_t*)(zalloc(sizeof(gnss_event_t)));
    if (!event) {
        FEATURE_LOG_ERROR("%s::%s() malloc error", file_tag, __FUNCTION__);
        goto errout;
    }

    event->meta.instance = feature;
    event->meta.callback = param->success;
    event->meta.fail = param->fail;
    event->meta.complete = param->complete;
    event->meta.oneshot = true;
    event->meta.timeout = param->timeout;
    event->isTimeout = false;

    ret = uv_topic_subscribe(FeatureGetUVLoop(manager), &event->topic,
        ORB_ID(sensor_gnss),
        sensor_gnss_topic_cb);
    if (ret < 0) {
        code = GENERAL;
        msg = "subscribe error";
        FEATURE_LOG_ERROR("%s::%s() subscribe error:%d\n", file_tag, __FUNCTION__, ret);
        goto errout;
    }

    if (event->meta.timeout > 0) {
        uv_timer_init(FeatureGetUVLoop(manager), &event->timer);
        uv_timer_start(&event->timer, sensor_timer_cb, event->meta.timeout, 0);
        context->getLocationQueue.push(event);
    }

    return;

errout:
    if (param->fail) {
        INVOKE_FAIL_CB(feature, param->fail, msg, code);
    }
    REMOVE_ALL_CALLBACK(param->success, param->fail);
    if (param->complete) {
        FeatureInvokeCallback(feature, param->complete, "getLocation complete");
        FeatureRemoveCallback(feature, param->complete);
    }
}

void system_geolocation_wrap_subscribe(FeatureInstanceHandle feature, AppendData append_data, system_geolocation_subscribeParam* param)
{
    int ret;
    GeoMetaData meta;

    meta.instance = feature;
    meta.callback = param->callback;
    meta.fail = param->fail;
    meta.oneshot = false;

    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(proto_handle));

    if (context->event.meta.subscribed && FeatureCheckCallbackId(feature, meta.callback) && FeatureCheckCallbackId(feature, meta.fail)) {
        REMOVE_ALL_CALLBACK(context->event.meta.callback, context->event.meta.fail);
        context->event.meta.callback = meta.callback;
        context->event.meta.fail = meta.fail;
        return;
    }

    context->event.meta = meta;
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    ret = uv_topic_subscribe(FeatureGetUVLoop(manager), &context->event.topic,
        ORB_ID(sensor_gnss),
        sensor_gnss_topic_cb);
    if (ret < 0) {
        FEATURE_LOG_ERROR("%s::%s() subscribe error:%d\n", file_tag, __FUNCTION__, ret);
        goto errout;
    }

    context->event.meta.subscribed = true;
    return;
errout:
    if (param->fail) {
        INVOKE_FAIL_CB(feature, param->fail, "subsrcibe error", GENERAL);
    }
    REMOVE_ALL_CALLBACK(meta.callback, meta.fail);
}

static void unsubscribe(FeatureInstanceHandle feature, bool detach)
{
    int ret;
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(proto_handle));
    if (context == NULL || feature == nullptr) {
        return;
    }

    if (context->event.meta.subscribed) {
        ret = uv_topic_unsubscribe(&context->event.topic);
        if (ret < 0) {
            if (detach) {
                context->event.meta.instance = nullptr;
            }

            FEATURE_LOG_ERROR("%s::%s() call uv_topic_unsubscribe Failed,ret=%d", file_tag, __FUNCTION__, ret);
            goto exit;
        }

        ret = uv_topic_close(&context->event.topic, NULL);
        if (ret < 0) {
            if (detach) {
                context->event.meta.instance = nullptr;
            }

            FEATURE_LOG_ERROR("%s::%s()call uv_topic_close,ret = %d", file_tag, __FUNCTION__, ret);
            goto exit;
        }

        context->event.meta.subscribed = false;
    }

exit:
    FEATURE_LOG_DEBUG("unsubscribe gnss success");
    if (detach) {
        context = NULL;
        return;
    }

    REMOVE_ALL_CALLBACK(context->event.meta.callback, context->event.meta.fail);
}

void system_geolocation_wrap_unsubscribe(FeatureInstanceHandle feature, AppendData append_data)
{
    unsubscribe(feature, false);
}

void system_geolocation_onRegister(const char* feature_name)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_geolocation_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    location_context* context = (location_context*)zalloc(sizeof(location_context));
    if (!context) {
        FEATURE_LOG_ERROR("%s::%s() malloc error", file_tag, __FUNCTION__);
        return;
    }

    context->subRet = system_geolocationMallocsubscribeRet();
    context->getRet = system_geolocationMallocgetLocationRet();
    FeatureSetProtoData(handle, context);
}

void system_geolocation_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_geolocation_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    if (!handle) {
        return;
    }

    unsubscribe(handle, true);
}

void system_geolocation_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(handle));
    if (!context) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }
    free(context);
}

void system_geolocation_onUnregister(const char* feature_name)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}
