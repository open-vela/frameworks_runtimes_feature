/*
 * Copyright (C) 2025 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <list>
#include <math.h>
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

struct GnssMetaData {
    FeatureInstanceHandle instance = nullptr;
    FtCallbackId callback = 0;
    FtCallbackId fail = 0;
    FtPromiseId pid = 0;
    timeval time = { 0, 0 };
    int timeout = 0;
};

struct location_context {
    std::queue<uv_topic_t*> topic_list;
    uv_timer_t timer;
    std::list<GnssMetaData> getList;
    std::list<GnssMetaData> subList;
    ft_context_ref ft_ctx;
    int ref_count;
};

static bool geolocation_is_active(location_context* context)
{
    return !context->getList.empty() || !context->subList.empty();
}

static void geolocation_topic_close_cb(uv_handle_t* handle)
{
    FEATURE_LOG_ERROR("%s::%s() topic close", file_tag, __FUNCTION__);
    uv_topic_t* topic = (uv_topic_t*)handle;
    location_context* context = static_cast<location_context*>(topic->user_data);
    delete (topic);
    if (--context->ref_count == 0) {
        FEATURE_LOG_ERROR("%s::%s() delete context", file_tag, __FUNCTION__);
        delete context;
    }
}

static std::list<GnssMetaData>::iterator geolocation_find_meta(location_context* context, FeatureInstanceHandle instance)
{
    for (auto it = context->subList.begin(); it != context->subList.end(); ++it) {
        if (it->instance == instance) {
            return it;
        }
    }
    return context->subList.end();
}

static void gnss_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    int res;
    location_context* context = static_cast<location_context*>(topic->user_data);

    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }

    system_geolocation_getLocationRet ret;
    ft_context_ref ft_ctx = context->ft_ctx;
    size_t cnt = datalen / sizeof(sensor_gnss);

    for (size_t i = 0; i < cnt; i++) {
        sensor_gnss* ret_t = (sensor_gnss*)data + i;
        if (isnan(ret_t->altitude) || isnan(ret_t->latitude) || isnan(ret_t->longitude)) {
            for (auto it = context->subList.begin(); it != context->subList.end(); it++) {
                FEATURE_LOG_ERROR("%s::%s() data is invalid", file_tag, __FUNCTION__);
                break;
            }
        } else {
            ft_value_t accuracyInfo = ft_new_object(ft_ctx);
            ft_value_t horizontal = ft_from_double(ft_ctx, round(ret_t->eph * PRECISION) / PRECISION);
            ft_value_t vertical = ft_from_double(ft_ctx, round(ret_t->epv * PRECISION) / PRECISION);
            ft_obj_set_property(ft_ctx, accuracyInfo, "horizontal", horizontal);
            ft_obj_set_property(ft_ctx, accuracyInfo, "vertical", vertical);
            ret.latitude = round(ret_t->latitude * PRECISION) / PRECISION;
            ret.longitude = round(ret_t->longitude * PRECISION) / PRECISION;
            ret.altitude = round(ret_t->altitude * PRECISION) / PRECISION;
            ret.accuracy = int(ret_t->eph);
            ret.speed = round(ret_t->ground_speed * PRECISION) / PRECISION;
            ret.accuracyInfo = &accuracyInfo;
            if (i == cnt - 1 && !context->getList.empty()) {
                GnssMetaData get_meta = context->getList.front();
                FeaturePromiseResolve(get_meta.instance, get_meta.pid, &ret);
                context->getList.pop_front();
            }

            for (auto it = context->subList.begin(); it != context->subList.end(); it++) {
                INVOKE_SUCCESS_CB(it->instance, it->callback, &ret);
            }
            ft_free_value(ft_ctx, accuracyInfo);
        }

        if (!geolocation_is_active(context) && !context->topic_list.empty()) {
            FEATURE_LOG_ERROR("%s::%s() close sub topic", file_tag, __FUNCTION__);
            uv_topic_t* free_topic = context->topic_list.front();
            context->topic_list.pop();
            res = uv_topic_unsubscribe(free_topic);
            if (res < 0) {
                FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe fail", file_tag, __FUNCTION__);
            }

            res = uv_topic_close(free_topic, geolocation_topic_close_cb);
            if (res < 0) {
                FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
            }
            return;
        }
    }
}

void system_geolocation_wrap_getLocation(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_geolocation_getParam* param)
{
    int ret;
    int code;
    const char* msg = "";
    GnssMetaData getMeta;
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(proto_handle));
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);

    FEATURE_LOG_ERROR("%s::%s() get location", file_tag, __FUNCTION__);

    if (context->getList.size() > 100) {
        code = GENERAL;
        msg = "Reject: too many requests";
        goto errout;
    }

    if (!geolocation_is_active(context)) {
        FEATURE_LOG_ERROR("%s::%s() get sub", file_tag, __FUNCTION__);
        uv_topic_t* topic = new uv_topic_t();
        ret = uv_topic_subscribe(FeatureGetUVLoop(manager), topic,
            ORB_ID(sensor_gnss),
            gnss_topic_cb);
        if (ret < 0) {
            code = GENERAL;
            msg = "subscribe error";
            delete (topic);
            goto errout;
        }
        topic->user_data = (void*)context;
        context->topic_list.push(topic);
        context->ref_count++;
    }

    getMeta.instance = feature;
    getMeta.pid = pid;
    gettimeofday(&getMeta.time, NULL);
    getMeta.timeout = param->timeout;
    context->getList.push_back(getMeta);
    return;
errout:
    FeaturePromiseReject(feature, pid, code, msg);
}

void system_geolocation_wrap_subscribe(FeatureInstanceHandle feature, AppendData append_data, system_geolocation_subscribeParam* param)
{
    int ret;
    GnssMetaData meta;
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(proto_handle));
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);

    if (!FeatureCheckCallbackId(feature, param->callback)) {
        INVOKE_FAIL_CB(feature, param->fail, "invalid callback id", GENERAL);
    }

    auto it = geolocation_find_meta(context, feature);
    if (it != context->subList.end()) {
        REMOVE_ALL_CALLBACK(it->callback, it->fail);
        it->callback = param->callback;
        it->fail = param->fail;
        return;
    }

    if (!geolocation_is_active(context)) {
        uv_topic_t* topic = new uv_topic_t();
        FEATURE_LOG_ERROR("%s::%s() sub sub\n", file_tag, __FUNCTION__);
        ret = uv_topic_subscribe(FeatureGetUVLoop(manager), topic,
            ORB_ID(sensor_gnss),
            gnss_topic_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() subscribe error:%d\n", file_tag, __FUNCTION__, ret);
            delete (topic);
            goto errout;
        }
        topic->user_data = (void*)context;
        context->topic_list.push(topic);
        context->ref_count++;
    }

    meta.instance = feature;
    meta.callback = param->callback;
    meta.fail = param->fail;
    context->subList.push_back(meta);
    return;
errout:
    if (param->fail) {
        INVOKE_FAIL_CB(feature, param->fail, "subsrcibe error", GENERAL);
    }
    REMOVE_ALL_CALLBACK(param->callback, param->fail);
}

void geolocation_timer_handler(uv_timer_t* timer)
{
    int ret;
    location_context* context = container_of(timer, location_context, timer);
    while (!context->getList.empty()) {
        GnssMetaData meta = context->getList.front();
        timeval current;
        gettimeofday(&current, NULL);
        if (current.tv_sec - meta.time.tv_sec > meta.timeout / 1000) {
            FEATURE_LOG_ERROR("%s::%s() time out", file_tag, __FUNCTION__);
            FeaturePromiseReject(meta.instance, meta.pid, TIMEOUT, "getLocation time out");
            context->getList.pop_front();
            continue;
        }

        break;
    }

    if (!geolocation_is_active(context) && !context->topic_list.empty()) {
        FEATURE_LOG_ERROR("%s::%s() close sub topic", file_tag, __FUNCTION__);
        uv_topic_t* free_topic = context->topic_list.front();
        context->topic_list.pop();
        ret = uv_topic_unsubscribe(free_topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s topic unsubscribe failed", __FUNCTION__);
        }

        ret = uv_topic_close(free_topic, geolocation_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
        }
    }
}

static void unsubscribe(FeatureInstanceHandle feature)
{
    int ret;
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(proto_handle));
    auto it = geolocation_find_meta(context, feature);
    if (it == context->subList.end()) {
        return;
    }

    FEATURE_LOG_ERROR("%s::%s() ", file_tag, __FUNCTION__);
    REMOVE_ALL_CALLBACK(it->callback, it->fail);
    context->subList.erase(it);
    if (!geolocation_is_active(context) && !context->topic_list.empty()) {
        FEATURE_LOG_ERROR("%s::%s() close sub topic", file_tag, __FUNCTION__);
        uv_topic_t* free_topic = context->topic_list.front();
        context->topic_list.pop();
        ret = uv_topic_unsubscribe(free_topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s topic unsubscribe failed", __FUNCTION__);
        }

        ret = uv_topic_close(free_topic, geolocation_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
        }
    }
}

void system_geolocation_wrap_unsubscribe(FeatureInstanceHandle feature, AppendData append_data)
{
    unsubscribe(feature);
}

void system_geolocation_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_geolocation_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    location_context* context = new location_context();
    FeatureManagerHandle manger = FeatureGetManagerHandleFromProto(handle);
    if (!context) {
        FEATURE_LOG_ERROR("%s::%s() malloc error", file_tag, __FUNCTION__);
        return;
    }
    uv_timer_init(FeatureGetUVLoop(manger), &context->timer);
    uv_timer_start(&context->timer, geolocation_timer_handler, 0, 1000);
    context->ref_count++;
    FeatureSetProtoData(handle, context);
}

void system_geolocation_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FeatureProtoHandle protohandle = FeatureGetProtoHandle(handle);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(protohandle));
    if (!context->ft_ctx) {
        context->ft_ctx = FeatureGetContext(handle);
    }

    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_geolocation_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(proto_handle));
    context->getList.remove_if([&](const auto& elem) {
        return elem.instance == handle;
    });

    unsubscribe(handle);
}

static void geolocation_timer_close_cb(uv_handle_t* handle)
{
    FEATURE_LOG_ERROR("%s::%s() timer close", file_tag, __FUNCTION__);
    uv_timer_t* timer = (uv_timer_t*)handle;
    location_context* context = container_of(timer, location_context, timer);
    if (--context->ref_count == 0) {
        FEATURE_LOG_ERROR("%s::%s() delete context", file_tag, __FUNCTION__);
        delete context;
    }
}

void system_geolocation_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    int ret;
    location_context* context = static_cast<location_context*>(FeatureGetProtoData(handle));
    uv_timer_stop(&context->timer);
    uv_close((uv_handle_t*)&context->timer, geolocation_timer_close_cb);

    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);

    context->getList.clear();
    while (!context->topic_list.empty()) {
        uv_topic_t* topic = context->topic_list.front();
        context->topic_list.pop();
        ret = uv_topic_unsubscribe(topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("Failed to unsubscribe topic\n");
        }

        ret = uv_topic_close(topic, geolocation_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("Failed to close topic\n");
        }
    }
}

void system_geolocation_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
