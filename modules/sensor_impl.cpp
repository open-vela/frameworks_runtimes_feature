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
#include <sensor/accel.h>
#include <sensor/baro.h>
#include <sensor/humi.h>
#include <sensor/light.h>
#include <sensor/prox.h>
#include <sensor/temp.h>

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

#define UV_HANDLE_INTERNAL 0x00000010

typedef enum sensor_magic_e {
    SENSOR_MAGIC_ACCEL = 0,
    SENSOR_MAGIC_COMPA,
    SENSOR_MAGIC_PROX,
    SENSOR_MAGIC_LIGHT,
    SENSOR_MAGIC_STEP,
    SENSOR_MAGIC_BARO,
    SENSOR_MAGIC_AMBIENTTEMPERATURE,
    SENSOR_MAGIC_HUMIDITY,
    SENSOR_MAGIC_NUM,
} sensor_magic_t;

struct sensor_orb_t {
    orb_id_t meta;
    uv_topic_cb topic_cb;
};

struct MetaData {
    FeatureInstanceHandle instance;
    bool reserved;
    FtCallbackId callback;
    FtCallbackId fail;
};

struct sensor_event_t {
    uv_topic_t topic;
    MetaData meta;
};

struct SensorContext {
    sensor_event_t* events[SENSOR_MAGIC_NUM];
};

static void sensor_accel_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    sensor_accel* t_r = static_cast<sensor_accel*>(data);
    system_sensor_AccelerometerRet* accelRet = system_sensorMallocAccelerometerRet();
    accelRet->x = t_r->x;
    accelRet->y = t_r->y;
    accelRet->z = t_r->z;
    INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, accelRet);
    FeatureFreeValue(accelRet);
}

static void sensor_prox_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    sensor_prox* t_r = static_cast<sensor_prox*>(data);
    system_sensor_ProximityRet* proxRet = system_sensorMallocProximityRet();
    proxRet->distance = t_r->proximity;
    INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, proxRet);
    FeatureFreeValue(proxRet);
}

static void sensor_light_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    sensor_light* t_r = static_cast<sensor_light*>(data);
    system_sensor_LightRet* lightRet = system_sensorMallocLightRet();
    lightRet->intensity = t_r->light;
    INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, lightRet);
    FeatureFreeValue(lightRet);
}

static void sensor_compa_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen) { }

static void sensor_step_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen) { }

static void sensor_baro_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    sensor_baro* t_r = static_cast<sensor_baro*>(data);
    system_sensor_BaroRet* baroRet = system_sensorMallocBaroRet();
    baroRet->pressure = t_r->pressure;
    INVOKE_SUCCESS_CB(event->meta.instance, event->meta.callback, baroRet);
    FeatureFreeValue(baroRet);
}

static void sensor_temp_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    sensor_temp* t_r = static_cast<sensor_temp*>(data);
    system_sensor_TemperatureRet* tempRet = system_sensorMallocTemperatureRet();
    tempRet->temperature = t_r->temperature;
    FeatureInvokeCallback(event->meta.instance, event->meta.callback, tempRet);
    FeatureFreeValue(tempRet);
}

static void sensor_humi_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    sensor_humi* t_r = static_cast<sensor_humi*>(data);
    system_sensor_HumidityRet* humiRet = system_sensorMallocHumidityRet();
    humiRet->humidity = t_r->humidity;
    FeatureInvokeCallback(event->meta.instance, event->meta.callback, humiRet);
    FeatureFreeValue(humiRet);
}

const static sensor_orb_t sensor_orb_table[SENSOR_MAGIC_NUM] = { 
    [SENSOR_MAGIC_ACCEL] = { .meta = ORB_ID(sensor_accel), .topic_cb = sensor_accel_topic_cb, },
    [SENSOR_MAGIC_COMPA] = { .meta = NULL, .topic_cb = sensor_compa_topic_cb },
    [SENSOR_MAGIC_PROX] = { .meta = ORB_ID(sensor_prox), .topic_cb = sensor_prox_topic_cb },
    [SENSOR_MAGIC_LIGHT] = { .meta = ORB_ID(sensor_light), .topic_cb = sensor_light_topic_cb },
    [SENSOR_MAGIC_STEP] = { .meta = NULL, .topic_cb = sensor_step_topic_cb },
    [SENSOR_MAGIC_BARO] = { .meta = ORB_ID(sensor_baro), .topic_cb = sensor_baro_topic_cb },
    [SENSOR_MAGIC_AMBIENTTEMPERATURE] = { .meta = ORB_ID(sensor_temp), .topic_cb = sensor_temp_topic_cb },
    [SENSOR_MAGIC_HUMIDITY] = { .meta = ORB_ID(sensor_humi), .topic_cb = sensor_humi_topic_cb },
};

static void uv_topic_close_cb(uv_handle_t* handle)
{
    uv_topic_t* topic = container_of(handle, uv_topic_t, handle);
    sensor_event_t* event = container_of(topic, sensor_event_t, topic);
    free(event);
}

/****************************************************************************
 * Name: uv_topic_close_internal
 *
 * Description:
 * Becasue event->topic->handle will be accessed in `uv__run_closing_handles`, event should
 * be freed in `uv_topic_close_cb`
 ****************************************************************************/
static int uv_topic_close_internal(uv_topic_t* topic)
{
    topic->handle.flags |= UV_HANDLE_INTERNAL;
    uv_close((uv_handle_t*)&topic->handle, uv_topic_close_cb);
    return 0;
}

static void unsubscribe(FeatureInstanceHandle feature, int magic, bool is_active)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext* th = static_cast<SensorContext*>(FeatureGetProtoData(proto_handle));
    sensor_event_t* event = th->events[magic];
    if (event == NULL) {
        return;
    }
    // 1.active unsubsribe need invoke uv_topic_unsubscribe
    // 2.page jump,if not reserved need uv_topic_unsubscribe
    if (!is_active && event->meta.reserved) {
        return;
    }

    int ret = uv_topic_unsubscribe(&event->topic);
    if (ret < 0) {
        FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe failed,ret=%d\n", file_tag, __FUNCTION__,
            ret);
    }
    // Because uv_topic_subscribe will call uv_poll_init and create a new handle, the old hanle should be closed
    ret = uv_topic_close_internal(&event->topic);
    if (ret < 0) {
        FEATURE_LOG_ERROR("%s::%s() uv_topic_close failed,ret=%d\n", file_tag, __FUNCTION__,
            ret);
    }
    REMOVE_ALL_CALLBACK(event->meta.callback, event->meta.fail);
    th->events[magic] = NULL;
}

static bool subscribe(FeatureInstanceHandle feature, SensorContext* th, sensor_magic_e sensor_type, MetaData* meta_data)
{
    sensor_event_t* event = th->events[sensor_type];
    if (event) {
        unsubscribe(feature, sensor_type, true);
    }
    event = static_cast<sensor_event_t*>(malloc(sizeof(sensor_event_t)));
    event->meta = *meta_data;

    th->events[sensor_type] = event;
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    int ret = uv_topic_subscribe(FeatureGetUVLoop(manager), &th->events[sensor_type]->topic,
        sensor_orb_table[sensor_type].meta,
        sensor_orb_table[sensor_type].topic_cb);
    if (ret < 0) {
        th->events[sensor_type] = NULL;
        free(event);
        FEATURE_LOG_ERROR("%s::%s() sensor type %d subscribe error\n", file_tag, __FUNCTION__, sensor_type);
        return false;
    }
    return true;
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
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        unsubscribe(handle, i, false);
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
        interval = 60000;
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
    meta.fail = 0;

    if (subscribe(feature, th, SENSOR_MAGIC_ACCEL, &meta)) {
        uv_topic_set_interval(&th->events[SENSOR_MAGIC_ACCEL]->topic, interval);
    }
}

void system_sensor_wrap_unsubscribeAccelerometer(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_ACCEL, true);
}

void system_sensor_wrap_subscribeCompass(FeatureInstanceHandle feature, AppendData data,
    system_sensor_Compass* param) { }

void system_sensor_wrap_unsubscribeCompass(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_COMPA, true);
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
    meta.fail = 0;

    if (!subscribe(feature, th, SENSOR_MAGIC_PROX, &meta)) {
        INVOKE_FAIL_CB(feature, param->fail,
            "The current device does not support the distance sensor", 203);
    }
}

void system_sensor_wrap_unsubscribeProximity(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_PROX, true);
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

    subscribe(feature, th, SENSOR_MAGIC_LIGHT, &meta);
}

void system_sensor_wrap_unsubscribeLight(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_LIGHT, true);
}

void system_sensor_wrap_subscribeStepCounter(FeatureInstanceHandle feature, AppendData data,
    system_sensor_StepCount* param)
{
    INVOKE_FAIL_CB(feature, param->fail, "Current device does not support pedometer sensor", 1000);
    REMOVE_ALL_CALLBACK(param->callback, param->fail);
}

void system_sensor_wrap_unsubscribeStepCounter(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_STEP, true);
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
        INVOKE_FAIL_CB(feature, param->fail,
            "The current device does not support the barometer sensor", -1);
        REMOVE_ALL_CALLBACK(param->callback, param->fail);
    }
}

void system_sensor_wrap_unsubscribePressure(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_BARO, true);
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

    if(!subscribe(feature, th, SENSOR_MAGIC_AMBIENTTEMPERATURE, &meta)) {
        FeatureInvokeCallback(feature, param->fail,
            "The current device does not support the temperature sensor", -1);
        REMOVE_ALL_CALLBACK(param->callback, param->fail);
    }
}

void system_sensor_wrap_unsubscribeAmbientTemperature(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_AMBIENTTEMPERATURE, true);
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

    if(!subscribe(feature, th, SENSOR_MAGIC_HUMIDITY, &meta)) {
        FeatureInvokeCallback(feature, param->fail,
            "The current device does not support the humidity sensor", -1);
        REMOVE_ALL_CALLBACK(param->callback, param->fail);
    }
}

void system_sensor_wrap_unsubscribeHumidity(FeatureInstanceHandle feature, AppendData data)
{
    unsubscribe(feature, SENSOR_MAGIC_HUMIDITY, true);
}