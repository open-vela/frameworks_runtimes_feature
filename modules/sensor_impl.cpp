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

#include <cmath>
#include <map>
#include <nuttx/nuttx.h>
#include <sensor/accel.h>
#include <sensor/baro.h>
#include <sensor/compass.h>
#include <sensor/gnss.h>
#include <sensor/humi.h>
#include <sensor/light.h>
#include <sensor/prox.h>
#include <sensor/temp.h>

#ifdef CONFIG_MIWEAR_COMMON
#include <topics/algo_wrist_tilt.h>
#endif

#include "sensor.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] sensor_impl";
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

#define HIGH_INTERVAL 20000
#define MID_INTERVAL 50000
#define LOW_INTERVAL 200000
#define PRECISION 100000

typedef enum ErrorCode {
    GENERAL = 200,
    ARGSERROR = 202,
    SERVICEUNAVAILABLE = 203,
    IOERROR = 300,
    TIMEOUT = 204
} ErrorCode;

typedef enum sensor_magic_e {
    SENSOR_MAGIC_GNSS = 0,
    SENSOR_MAGIC_BARO,
    SENSOR_MAGIC_ACCEL,
    SENSOR_MAGIC_COMPA,
    SENSOR_MAGIC_PROX,
    SENSOR_MAGIC_LIGHT,
    SENSOR_MAGIC_STEP,
    SENSOR_MAGIC_AMBIENTTEMPERATURE,
    SENSOR_MAGIC_HUMIDITY,
#ifdef CONFIG_MIWEAR_COMMON
    SENSOR_MAGIC_WRIST_TILT,
#endif
    SENSOR_MAGIC_NUM,
} sensor_magic_t;

struct sensor_orb_t {
    int index;
    const char* sensor_name;
    orb_id_t meta;
    uv_topic_cb topic_cb;
};
struct MetaData {
    FeatureInstanceHandle instance;
    bool reserved;
    FtCallbackId callback;
    FtCallbackId fail;
    FtCallbackId complete;
    bool subscribed;
};

struct sensor_event_t {
    uv_topic_t topic;
    MetaData meta;
};

struct sensorMulti_user_t {
    sensor_event_t event;
    ft_context_ref ft_ctx;
    int type;
    bool oneshot;
};
struct sensorMulti_event_t {
    std::multimap<int, sensorMulti_user_t*> user_map;
};

struct SensorContext {
    sensor_event_t* events[SENSOR_MAGIC_NUM];
    sensorMulti_event_t* multi_events[SENSOR_MAGIC_NUM];
};

static void sensor_accel_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    int cnt = datalen / sizeof(sensor_accel);
    for (int i = 0; i < cnt; i++) {
        sensor_accel* t_r = (sensor_accel*)data + i;
        system_sensor_AccelerometerRet* accelRet = system_sensorMallocAccelerometerRet();
        accelRet->x = t_r->x;
        accelRet->y = t_r->y;
        accelRet->z = t_r->z;
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, accelRet);
        FeatureFreeValue(accelRet);
    }
}

static void sensor_prox_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    int cnt = datalen / sizeof(sensor_accel);
    for (int i = 0; i < cnt; i++) {
        sensor_prox* t_r = (sensor_prox*)data + i;
        system_sensor_ProximityRet* proxRet = system_sensorMallocProximityRet();
        proxRet->distance = t_r->proximity;
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, proxRet);
        FeatureFreeValue(proxRet);
    }
}

static void sensor_light_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    int cnt = datalen / sizeof(sensor_accel);
    for (int i = 0; i < cnt; i++) {
        sensor_light* t_r = (sensor_light*)data + i;
        system_sensor_LightRet* lightRet = system_sensorMallocLightRet();
        lightRet->intensity = t_r->light;
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, lightRet);
        FeatureFreeValue(lightRet);
    }
}

static void sensor_compass_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    int cnt = datalen / sizeof(sensor_compass);
    system_sensor_CompassRet* compassRet = system_sensorMallocCompassRet();

    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }

    for (int i = 0; i < cnt; i++) {
        sensor_compass* t_r = (sensor_compass*)data + i;
        double degree = (t_r->direction * acos(-1.0)) / 180.0;
        if (degree > acos(-1.0)) {
            degree -= 2 * acos(-1.0);
        }

        compassRet->direction = degree;
        compassRet->accuracy = t_r->cal_status;
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, compassRet);
    }
    FeatureFreeValue(compassRet);
}

static void sensor_step_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen) { }

static void sensor_baro_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    int cnt = datalen / sizeof(sensor_baro);
    for (int i = 0; i < cnt; i++) {
        sensor_baro* t_r = (sensor_baro*)data + i;
        system_sensor_BaroRet* baroRet = system_sensorMallocBaroRet();
        baroRet->pressure = round(t_r->pressure * PRECISION) / PRECISION;
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, baroRet);
        FeatureFreeValue(baroRet);
    }
}

static void sensor_temp_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    int cnt = datalen / sizeof(sensor_temp);
    for (int i = 0; i < cnt; i++) {
        sensor_temp* t_r = (sensor_temp*)data + i;
        system_sensor_TemperatureRet* tempRet = system_sensorMallocTemperatureRet();
        tempRet->temperature = round(t_r->temperature * 10) / 10;
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, tempRet);
        FeatureFreeValue(tempRet);
    }
}

static void sensor_humi_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    int cnt = datalen / sizeof(sensor_humi);
    for (int i = 0; i < cnt; i++) {
        sensor_humi* t_r = (sensor_humi*)data + i;
        system_sensor_HumidityRet* humiRet = system_sensorMallocHumidityRet();
        humiRet->humidity = round(t_r->humidity);
        INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, humiRet);
        FeatureFreeValue(humiRet);
    }
}

const static sensor_orb_t sensor_orb_table[SENSOR_MAGIC_NUM] = {
    [SENSOR_MAGIC_GNSS] = {
        .index = 0,
        .sensor_name = "GPS",
        .meta = ORB_ID(sensor_gnss),
        .topic_cb = NULL },
    [SENSOR_MAGIC_BARO] = { .index = 1, .sensor_name = "BAROMETER", .meta = ORB_ID(sensor_baro), .topic_cb = sensor_baro_topic_cb },
    [SENSOR_MAGIC_ACCEL] = {
        .index = 2,
        .sensor_name = "ACCELEROMETER",
        .meta = ORB_ID(sensor_accel),
        .topic_cb = sensor_accel_topic_cb,
    },
    [SENSOR_MAGIC_COMPA] = { .index = 3, .sensor_name = "COMPASS", .meta = ORB_ID(sensor_compass), .topic_cb = sensor_compass_topic_cb },
    [SENSOR_MAGIC_PROX] = { .index = 4, .sensor_name = "PROXIMITY", .meta = ORB_ID(sensor_prox), .topic_cb = sensor_prox_topic_cb },
    [SENSOR_MAGIC_LIGHT] = { .index = 5, .sensor_name = "LIGHT", .meta = ORB_ID(sensor_light), .topic_cb = sensor_light_topic_cb },
    [SENSOR_MAGIC_STEP] = { .index = 6, .sensor_name = "STEP_COUNTER", .meta = NULL, .topic_cb = sensor_step_topic_cb },
    [SENSOR_MAGIC_AMBIENTTEMPERATURE] = { .index = 7, .sensor_name = "AMBIENT_TEMPERATURE", .meta = ORB_ID(sensor_temp), .topic_cb = sensor_temp_topic_cb },
    [SENSOR_MAGIC_HUMIDITY] = { .index = 12, .sensor_name = "HUMIDITY", .meta = ORB_ID(sensor_humi), .topic_cb = sensor_humi_topic_cb },
#ifdef CONFIG_MIWEAR_COMMON
    [SENSOR_MAGIC_WRIST_TILT] = { .index = 22, .sensor_name = "WRIST_LIFT", .meta = ORB_ID(algo_wrist_tilt), .topic_cb = NULL },
#endif
};

static void unsubscribe(FeatureInstanceHandle feature, int magic, bool detach)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    sensor_event_t* event = th->events[magic];
    if (event == NULL || feature == nullptr) {
        return;
    }

    // 1.active unsubsribe need invoke uv_topic_unsubscribe
    // 2.page jump,if not reserved need uv_topic_unsubscribe
    if (event->meta.subscribed) {
        int ret = uv_topic_unsubscribe(&event->topic);
        if (ret < 0) {
            if (detach) {
                event->meta.instance = nullptr;
            }

            FEATURE_LOG_ERROR("%s::%s() call uv_topic_unsubscribe Failed,ret=%d", file_tag, __FUNCTION__, ret);
            goto exit;
        }

        ret = uv_topic_close(&event->topic, NULL);
        if (ret < 0) {
            if (detach) {
                event->meta.instance = nullptr;
            }

            FEATURE_LOG_ERROR("%s::%s()call uv_topic_close,ret = %d", file_tag, __FUNCTION__, ret);
            goto exit;
        }

        event->meta.subscribed = false;
    }

exit:
    FEATURE_LOG_INFO("%s unsubscribe success", __FUNCTION__);
    if (detach) {
        th->events[magic] = nullptr;
        return;
    }

    if (FeatureCheckCallbackId(feature, event->meta.callback)) {
        FeatureRemoveCallback(feature, event->meta.callback);
    }

    if (FeatureCheckCallbackId(feature, event->meta.fail)) {
        FeatureRemoveCallback(feature, event->meta.fail);
    }
}

static bool subscribe(FeatureInstanceHandle feature, SensorContext* th, sensor_magic_e magic, MetaData* meta)
{
    int ret;
    int code;
    const char* msg = "";
    sensor_event_t* event = th->events[magic];
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);

    if (!FeatureCheckCallbackId(feature, meta->callback)) {
        code = ARGSERROR;
        msg = "callback id is invalid";
        goto errout;
    }

    if (event && event->meta.subscribed) {
        REMOVE_ALL_CALLBACK(event->meta.callback, event->meta.fail);
        event->meta.fail = meta->fail;
        event->meta.callback = meta->callback;
        return true;
    }

    event = static_cast<sensor_event_t*>(malloc(sizeof(sensor_event_t)));
    if (event == NULL) {
        code = GENERAL;
        msg = "malloc error";
        goto errout;
    }

    event->meta = *meta;
    th->events[magic] = event;
    ret = uv_topic_subscribe(FeatureGetUVLoop(manager), &th->events[magic]->topic,
        sensor_orb_table[magic].meta,
        sensor_orb_table[magic].topic_cb);
    if (ret < 0) {
        code = GENERAL;
        msg = "subscribe error";
        th->events[magic] = NULL;
        free(event);
        FEATURE_LOG_ERROR("%s::%s() subscribe error:%d\n", file_tag, __FUNCTION__, ret);
        goto errout;
    }
    event->meta.subscribed = true;
    return true;
errout:
    INVOKE_FAIL_CB(feature, meta->fail, msg, code);
    REMOVE_ALL_CALLBACK(meta->callback, meta->fail);
    return false;
}

void system_sensor_wrap_subscribeAccelerometer(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Accelerometer* param)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    int interval = 0;
    if (strcmp(param->interval, "game") == 0) {
        interval = 20000;
    } else if (strcmp(param->interval, "ui") == 0) {
        interval = 50000;
    } else if (strcmp(param->interval, "normal") == 0) {
        interval = 200000;
    } else {
        FEATURE_LOG_ERROR("%s::%s() param interval is invalid:%s\n", file_tag, __FUNCTION__,
            param->interval);
        return;
    }

    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = param->fail;

    if (subscribe(feature, th, SENSOR_MAGIC_ACCEL, &meta)) {
        int ret = uv_topic_set_interval(&th->events[SENSOR_MAGIC_ACCEL]->topic, interval);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() set interval error:%d\n", file_tag, __FUNCTION__, ret);
        }
    } else {
        FEATURE_LOG_ERROR("%s::%s() accel subscibe fail:%s\n", file_tag, __FUNCTION__);
    }
}

void system_sensor_wrap_unsubscribeAccelerometer(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_ACCEL, false);
}

void system_sensor_wrap_subscribeCompass(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Compass* param)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    MetaData meta;
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = param->fail;

    if (subscribe(feature, th, SENSOR_MAGIC_COMPA, &meta)) {
        int ret = uv_topic_set_interval(&th->events[SENSOR_MAGIC_COMPA]->topic, 100000);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() set interval error:%d\n", file_tag, __FUNCTION__, ret);
        }
    } else {
        FEATURE_LOG_ERROR("%s::%s() compass subscibe fail:%s\n", file_tag, __FUNCTION__);
    }
}

void system_sensor_wrap_unsubscribeCompass(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_COMPA, false);
}

void system_sensor_wrap_subscribeProximity(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Proximity* param)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = param->fail;

    if (!subscribe(feature, th, SENSOR_MAGIC_PROX, &meta)) {
        FEATURE_LOG_ERROR("%s::%s() proximity subscibe fail:%s\n", file_tag, __FUNCTION__);
    }
}

void system_sensor_wrap_unsubscribeProximity(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_PROX, false);
}

void system_sensor_wrap_subscribeLight(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Light* param)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = 0;

    if (!subscribe(feature, th, SENSOR_MAGIC_LIGHT, &meta)) {
        FEATURE_LOG_ERROR("%s::%s() light subscibe fail:%s\n", file_tag, __FUNCTION__);
    }
}

void system_sensor_wrap_unsubscribeLight(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_LIGHT, false);
}

void system_sensor_wrap_subscribeStepCounter(FeatureInstanceHandle feature, AppendData data,
    system_sensor_StepCount* param)
{
    INVOKE_FAIL_CB(feature, param->fail, "Current device does not support pedometer sensor", 203);
    REMOVE_ALL_CALLBACK(param->callback, param->fail);
}

void system_sensor_wrap_unsubscribeStepCounter(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_STEP, false);
}

void system_sensor_wrap_subscribePressure(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Baro* param)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = param->fail;

    if (!subscribe(feature, th, SENSOR_MAGIC_BARO, &meta)) {
        FEATURE_LOG_ERROR("%s::%s() pressure subscibe fail:%s\n", file_tag, __FUNCTION__);
    }
}

void system_sensor_wrap_unsubscribePressure(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_BARO, false);
}

void system_sensor_wrap_subscribeAmbientTemperature(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Temperature* param)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = param->fail;

    if (!subscribe(feature, th, SENSOR_MAGIC_AMBIENTTEMPERATURE, &meta)) {
        FEATURE_LOG_ERROR("%s::%s() accel subscibe fail:%s\n", file_tag, __FUNCTION__);
    }
}

void system_sensor_wrap_unsubscribeAmbientTemperature(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_AMBIENTTEMPERATURE, false);
}

void system_sensor_wrap_subscribeHumidity(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Humidity* param)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = param->fail;

    if (!subscribe(feature, th, SENSOR_MAGIC_HUMIDITY, &meta)) {
        FEATURE_LOG_ERROR("%s::%s() humidity subscibe fail:%s\n", file_tag, __FUNCTION__);
    }
}

void system_sensor_wrap_unsubscribeHumidity(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_HUMIDITY, false);
}

FtAny system_sensor_get_DATA_TYPES(void* feature, AppendData append_data)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* any_ptr = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    ft_value_t ret_obj = ft_new_object(ft_ctx);
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        ft_value_t type = ft_from_int(ft_ctx, sensor_orb_table[i].index);
        ft_obj_set_property(ft_ctx, ret_obj, sensor_orb_table[i].sensor_name, type);
    }
    *any_ptr = ret_obj;
    return any_ptr;
}

static sensor_magic_t get_sensor_magic(int type)
{
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        if (sensor_orb_table[i].index == type) {
            return (sensor_magic_t)i;
        }
    }
    FEATURE_LOG_ERROR("sensor type:%d is not exist", type);
    return SENSOR_MAGIC_NUM;
}

static void sensor_topic_close_cb(uv_handle_t* handle)
{
    sensor_event_t* event = container_of(handle, sensor_event_t, topic);
    sensorMulti_user_t* user = container_of(event, sensorMulti_user_t, event);
    FeatureInstanceHandle feature = user->event.meta.instance;
    if (user->event.meta.complete > 0) {
        INVOKE_SUCCESS_CB(feature, user->event.meta.complete, "get recent data complete");
        FeatureRemoveCallback(feature, user->event.meta.complete);
    }

    free(user);
}

static void sensor_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    int ret;
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    sensorMulti_user_t* user = container_of(event, sensorMulti_user_t, event);
    ft_context_ref ft_ctx = user->ft_ctx;
    ft_value_t sensor_obj = ft_new_object(ft_ctx);
    ft_value_t ret_obj = ft_new_object(ft_ctx);
    sensor_magic_t magic = get_sensor_magic(user->type);
    switch (magic) {
    case SENSOR_MAGIC_GNSS: {
        sensor_gnss* ret_t = static_cast<sensor_gnss*>(data);
        ft_value_t latitude = ft_from_double(ft_ctx, round(ret_t->latitude * PRECISION) / PRECISION);
        ft_value_t longitude = ft_from_double(ft_ctx, round(ret_t->longitude * PRECISION) / PRECISION);
        ft_value_t altitude = ft_from_double(ft_ctx, round(ret_t->altitude * PRECISION) / PRECISION);
        ft_value_t speed = ft_from_double(ft_ctx, round(ret_t->ground_speed * PRECISION) / PRECISION);
        ft_value_t accuracy = ft_from_int(ft_ctx, static_cast<int>(ret_t->eph));
        ft_obj_set_property(ft_ctx, ret_obj, "latitude", latitude);
        ft_obj_set_property(ft_ctx, ret_obj, "longitude", longitude);
        ft_obj_set_property(ft_ctx, ret_obj, "altitude", altitude);
        ft_obj_set_property(ft_ctx, ret_obj, "speed", speed);
        ft_obj_set_property(ft_ctx, ret_obj, "accuracy", accuracy);
        ft_obj_set_property(ft_ctx, sensor_obj, "GPS", ret_obj);
        break;
    }
    case SENSOR_MAGIC_BARO: {
        sensor_baro* ret_t = static_cast<sensor_baro*>(data);
        ft_value_t pressure = ft_from_double(ft_ctx, round(ret_t->pressure * PRECISION) / PRECISION);
        ft_obj_set_property(ft_ctx, ret_obj, "pressure", pressure);
        ft_obj_set_property(ft_ctx, sensor_obj, "BAROMETER", ret_obj);
        break;
    }
    case SENSOR_MAGIC_ACCEL: {
        sensor_accel* ret_t = static_cast<sensor_accel*>(data);
        ft_value_t ret_x = ft_from_double(ft_ctx, round(ret_t->x * PRECISION) / PRECISION);
        ft_value_t ret_y = ft_from_double(ft_ctx, round(ret_t->y * PRECISION) / PRECISION);
        ft_value_t ret_z = ft_from_double(ft_ctx, round(ret_t->z * PRECISION) / PRECISION);
        ft_obj_set_property(ft_ctx, ret_obj, "x", ret_x);
        ft_obj_set_property(ft_ctx, ret_obj, "y", ret_y);
        ft_obj_set_property(ft_ctx, ret_obj, "z", ret_z);
        ft_obj_set_property(ft_ctx, sensor_obj, "ACCELEROMETER", ret_obj);
        break;
    }
    case SENSOR_MAGIC_PROX: {
        sensor_prox* ret_t = static_cast<sensor_prox*>(data);
        ft_value_t prox = ft_from_int(ft_ctx, ret_t->proximity);
        ft_obj_set_property(ft_ctx, ret_obj, "distance", prox);
        ft_obj_set_property(ft_ctx, sensor_obj, "PROXIMITY", ret_obj);
    }
    case SENSOR_MAGIC_LIGHT: {
        sensor_light* ret_t = static_cast<sensor_light*>(data);
        ft_value_t light = ft_from_int(ft_ctx, ret_t->light);
        ft_obj_set_property(ft_ctx, ret_obj, "light", light);
        ft_obj_set_property(ft_ctx, sensor_obj, "LIGHT", ret_obj);
        break;
    }
    case SENSOR_MAGIC_AMBIENTTEMPERATURE: {
        sensor_temp* ret_t = static_cast<sensor_temp*>(data);
        ft_value_t temp = ft_from_double(ft_ctx, round(ret_t->temperature * 10) / 10);
        ft_obj_set_property(ft_ctx, ret_obj, "temperature", temp);
        ft_obj_set_property(ft_ctx, sensor_obj, "AMBIENTTEMPERATURE", ret_obj);
        break;
    }
    case SENSOR_MAGIC_HUMIDITY: {
        sensor_humi* ret_t = static_cast<sensor_humi*>(data);
        ft_value_t humi = ft_from_int(ft_ctx, round(ret_t->humidity));
        ft_obj_set_property(ft_ctx, ret_obj, "humidity", humi);
        ft_obj_set_property(ft_ctx, sensor_obj, "HUMIDITY", ret_obj);
        break;
    }
    case SENSOR_MAGIC_COMPA: {
        sensor_compass* ret_t = static_cast<sensor_compass*>(data);
        double direction = (ret_t->direction * acos(-1.0)) / 180.0;
        if (direction > acos(-1.0)) {
            direction -= 2 * acos(-1.0);
        }

        ft_value_t degree = ft_from_double(ft_ctx, direction);
        ft_value_t accuracy = ft_from_int(ft_ctx, ret_t->cal_status);
        ft_obj_set_property(ft_ctx, ret_obj, "direction", degree);
        ft_obj_set_property(ft_ctx, ret_obj, "accuracy", accuracy);
        ft_obj_set_property(ft_ctx, sensor_obj, "COMPASS", ret_obj);
        break;
    }
#ifdef CONFIG_MIWEAR_COMMON
    case SENSOR_MAGIC_WRIST_TILT: {
        algo_wrist_tilt* ret_t = static_cast<algo_wrist_tilt*>(data);
        if (ret_t->event != WRIST_TILT_UP) {
            ft_free_value(ft_ctx, sensor_obj);
            ft_free_value(ft_ctx, ret_obj);
            return;
        }
        break;
    }
#endif
    default:
        break;
    }
    ft_value_t* t_r_data = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *t_r_data = sensor_obj;
    if (user->oneshot) {
        system_sensor_cb_param* t_r = system_sensorMalloccb_param();
        t_r->data = t_r_data;
        t_r->dataType = user->type;
        INVOKE_SUCCESS_CB(user->event.meta.instance, user->event.meta.callback, t_r);
        FeatureFreeValue(t_r);
    } else {
        INVOKE_SUCCESS_CB(user->event.meta.instance, user->event.meta.callback, t_r_data);
        FeatureFreeValue(t_r_data);
    }

    ft_free_value(ft_ctx, sensor_obj);
    if (user->oneshot) {
        ret = uv_topic_unsubscribe(topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe fail", file_tag, __FUNCTION__);
        }

        ret = uv_topic_close(topic, sensor_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
        }

        FeatureInstanceHandle feature = user->event.meta.instance;
        REMOVE_ALL_CALLBACK(user->event.meta.callback, user->event.meta.fail);
    }
}

static void multi_unsubscribe(sensorMulti_event_t* multi_event, FtInt type, bool detach)
{
    int ret;
    auto target = multi_event->user_map.equal_range(type);
    for (auto it = target.first; it != target.second; ++it) {
        ret = uv_topic_unsubscribe(&it->second->event.topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe failed,ret=%d\n", file_tag, __FUNCTION__,
                ret);
        }

        ret = uv_topic_close(&it->second->event.topic, sensor_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_close failed,ret=%d\n", file_tag, __FUNCTION__,
                ret);
        }

        if (!detach) {
            FeatureInstanceHandle feature = it->second->event.meta.instance;
            REMOVE_ALL_CALLBACK(it->second->event.meta.callback, it->second->event.meta.fail);
        }
    }

    if (!detach) {
        multi_event->user_map.erase(target.first, target.second);
    }
}

void system_sensor_wrap_unsubscribe(FeatureInstanceHandle feature, union AppendData append_data, FtInt type)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensorMulti context is NULL\n", file_tag, __FUNCTION__);
        return;
    }
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        if (th->multi_events[i] == NULL || th->multi_events[i]->user_map.empty()) {
            continue;
        }
        multi_unsubscribe(th->multi_events[i], type, false);
    }
}

FtInt system_sensor_wrap_subscribe(FeatureInstanceHandle feature, union AppendData append_data, system_sensor_subscribeParam* param)
{
    int fd = 0;
    int code = 0;
    const char* msg = "";
    int ret = 0;
    int interval = 0;
    sensor_magic_t magic;
    sensorMulti_user_t* user;
    sensorMulti_event_t* event;
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensorMulti context is NULL\n", file_tag, __FUNCTION__);
        return -1;
    }

    if (!FeatureCheckCallbackId(feature, param->callback)) {
        code = GENERAL;
        msg = "callback id is invalid";
        FEATURE_LOG_ERROR("%s::%s() callback id is invalid", file_tag, __FUNCTION__);
        goto errout;
    }

    magic = get_sensor_magic(param->type);
    if (magic == SENSOR_MAGIC_NUM) {
        code = SERVICEUNAVAILABLE;
        msg = "current sensor is not support";
        FEATURE_LOG_ERROR("%s::%s() current sensor is not support", file_tag, __FUNCTION__);
        goto errout;
    }

    event = th->multi_events[magic];
    if (!event) {
        event = new sensorMulti_event_t();
    }
    if (strcmp(param->interval, "low") == 0) {
        interval = LOW_INTERVAL;
    } else if (strcmp(param->interval, "mid") == 0) {
        interval = MID_INTERVAL;
    } else if (strcmp(param->interval, "high") == 0) {
        interval = HIGH_INTERVAL;
    } else {
        code = GENERAL;
        msg = "param interval is invalid";
        FEATURE_LOG_ERROR("%s::%s() param interval is invalid:%s\n", file_tag, __FUNCTION__,
            param->interval);
        goto errout;
    }

    user = static_cast<sensorMulti_user_t*>(zalloc(sizeof(sensorMulti_user_t)));
    if (!user) {
        code = GENERAL;
        msg = "malloc user failed";
        FEATURE_LOG_ERROR("%s::%s() malloc user failed", file_tag, __FUNCTION__);
        goto errout;
    }

    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->reserved;
    meta.callback = param->callback;
    meta.fail = param->fail;
    user->ft_ctx = FeatureGetContext(feature);
    user->type = param->type;
    user->oneshot = false;
    user->event.meta = meta;

    ret = uv_topic_subscribe(FeatureGetUVLoop(manager), &user->event.topic,
        sensor_orb_table[magic].meta,
        sensor_topic_cb);
    fd = user->event.topic.handle.io_watcher.fd;
    if (ret < 0) {
        code = GENERAL;
        msg = "uv_topic_subscribe error";
        FEATURE_LOG_ERROR("%s::%s() uv_topic_subscribe error:%d\n", file_tag, __FUNCTION__, ret);
        free(user);
        goto errout;
    }

    ret = uv_topic_set_interval(&user->event.topic, interval);
    if (ret < 0) {
        code = GENERAL;
        msg = "set interval error";
        FEATURE_LOG_ERROR("%s::%s() set interval error:%d\n", file_tag, __FUNCTION__, ret);
        free(user);
        goto errout;
    }

    event->user_map.insert(std::make_pair(fd, user));
    th->multi_events[magic] = event;
    return fd;

errout:
    INVOKE_FAIL_CB(feature, param->fail, msg, code);
    REMOVE_ALL_CALLBACK(param->callback, param->fail);
    return -1;
}

void system_sensor_wrap_getRecentData(FeatureInstanceHandle feature, union AppendData append_data, system_sensor_getRecentDataParam* param)
{
    int ret;
    int code;
    const char* msg = "";
    sensorMulti_user_t* recent_meta;
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    sensor_magic_t magic;
    if (!FeatureCheckCallbackId(feature, param->success)) {
        FEATURE_LOG_ERROR("%s::%s() callback id is invalid", file_tag, __FUNCTION__);
        code = GENERAL;
        msg = "success callback id is invalid";
        goto errout;
    }

    magic = get_sensor_magic(param->type);
    if (magic == SENSOR_MAGIC_NUM) {
        FEATURE_LOG_ERROR("%s::%s() sensor not exist", file_tag, __FUNCTION__);
        code = GENERAL;
        msg = "sensor not exist";
        goto errout;
    }

    recent_meta = static_cast<sensorMulti_user_t*>(zalloc(sizeof(sensorMulti_user_t)));
    if (!recent_meta) {
        FEATURE_LOG_ERROR("%s::%s() malloc user failed", file_tag, __FUNCTION__);
        code = GENERAL;
        msg = "malloc user failed";
        goto errout;
    }

    recent_meta->ft_ctx = FeatureGetContext(feature);
    recent_meta->event.meta.instance = feature;
    recent_meta->event.meta.callback = param->success;
    recent_meta->event.meta.fail = param->fail;
    recent_meta->event.meta.complete = param->complete;
    recent_meta->type = param->type;
    recent_meta->oneshot = true;

    ret = uv_topic_subscribe(FeatureGetUVLoop(manager), &recent_meta->event.topic,
        sensor_orb_table[magic].meta,
        sensor_topic_cb);
    if (ret < 0) {
        FEATURE_LOG_ERROR("%s::%s() uv_topic_subscribe fail", file_tag, __FUNCTION__);
        code = GENERAL;
        msg = "uv_topic_subscribe error";
        goto errout;
    } else {
        return;
    }
errout:
    INVOKE_FAIL_CB(feature, param->fail, msg, code);
    REMOVE_ALL_CALLBACK(param->success, param->fail);
    if (param->complete) {
        FeatureInvokeCallback(feature, param->complete, "get recent data complete");
        FeatureRemoveCallback(feature, param->complete);
    }
}

FtBool system_sensor_wrap_checkAvailable(FeatureInstanceHandle feature, union AppendData append_data, FtInt type)
{
    sensor_magic_t magic = get_sensor_magic(type);
    if (magic != SENSOR_MAGIC_NUM) {
        orb_id_t meta = sensor_orb_table[magic].meta;
        return !orb_exists(meta, 0);
    }

    return false;
}

void system_sensor_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_sensor_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    SensorContext* th = static_cast<SensorContext*>(malloc(sizeof(SensorContext)));
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        th->events[i] = NULL;
        th->multi_events[i] = NULL;
    }
    FeatureSetProtoData(handle, th);
}

void system_sensor_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_sensor_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    if (!handle) {
        return;
    }
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    SensorContext* th_multi = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        unsubscribe(handle, i, true);
        if (th_multi->multi_events[i]) {
            for (auto it = th_multi->multi_events[i]->user_map.begin(); it != th_multi->multi_events[i]->user_map.end(); it++) {
                int fd = it->first;
                multi_unsubscribe(th_multi->multi_events[i], fd, true);
            }
            th_multi->multi_events[i] = NULL;
        }
    }
}

void system_sensor_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(handle));
    if (!th) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }
    free(th);
}

void system_sensor_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}