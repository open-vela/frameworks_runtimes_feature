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
#include <sensor/light.h>
#include <sensor/prox.h>

#include "sensor.h"
#include "uv_ext.h"

static const char *file_tag = "[jidl_feature] sensor_impl";

typedef enum sensor_magic_e {
    SENSOR_MAGIC_ACCEL = 0,
    SENSOR_MAGIC_COMPA,
    SENSOR_MAGIC_PROX,
    SENSOR_MAGIC_LIGHT,
    SENSOR_MAGIC_STEP,
    SENSOR_MAGIC_NUM,
} sensor_magic_t;

struct sensor_orb_t {
    orb_id_t meta;
    uv_topic_cb topic_cb;
};

struct MetaData {
    FeatureInstanceHandle instance;
    bool reserved;
    FtCallbackId id;
};

struct sensor_event_t {
    uv_topic_t topic;
    MetaData meta;
};

struct SensorContext {
    sensor_event_t *events[SENSOR_MAGIC_NUM];
};

static void sensor_accel_topic_cb(uv_topic_t *topic, int status, void *data, size_t datalen) {
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t *event = container_of(topic, sensor_event_t, topic);
    sensor_accel *t_r = static_cast<sensor_accel *>(data);
    sensor_AccelerometerRet *accelRet = sensorMallocAccelerometerRet();
    accelRet->_x = t_r->x;
    accelRet->_y = t_r->y;
    accelRet->_z = t_r->z;
    FeatureInvokeCallback(event->meta.instance, event->meta.id, accelRet);
    FeatureFreeValue(accelRet);
}

static void sensor_prox_topic_cb(uv_topic_t *topic, int status, void *data, size_t datalen) {
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t *event = container_of(topic, sensor_event_t, topic);
    sensor_prox *t_r = static_cast<sensor_prox *>(data);
    sensor_ProximityRet *proxRet = sensorMallocProximityRet();
    proxRet->_distance = t_r->proximity;
    FeatureInvokeCallback(event->meta.instance, event->meta.id, proxRet);
    FeatureFreeValue(proxRet);
}

static void sensor_light_topic_cb(uv_topic_t *topic, int status, void *data, size_t datalen) {
    if (!topic || !data) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    sensor_event_t *event = container_of(topic, sensor_event_t, topic);
    sensor_light *t_r = static_cast<sensor_light *>(data);
    sensor_LightRet *lightRet = sensorMallocLightRet();
    lightRet->_intensity = t_r->light;
    FeatureInvokeCallback(event->meta.instance, event->meta.id, lightRet);
    FeatureFreeValue(lightRet);
}

static void sensor_compa_topic_cb(uv_topic_t *topic, int status, void *data, size_t datalen) {}

static void sensor_step_topic_cb(uv_topic_t *topic, int status, void *data, size_t datalen) {}

const static sensor_orb_t sensor_orb_table[SENSOR_MAGIC_NUM] =
        {[SENSOR_MAGIC_ACCEL] =
                 {
                         .meta = ORB_ID(sensor_accel),
                         .topic_cb = sensor_accel_topic_cb,
                 },
         [SENSOR_MAGIC_COMPA] = {.meta = NULL, .topic_cb = sensor_compa_topic_cb},
         [SENSOR_MAGIC_PROX] = {.meta = ORB_ID(sensor_prox), .topic_cb = sensor_prox_topic_cb},
         [SENSOR_MAGIC_LIGHT] = {.meta = ORB_ID(sensor_light), .topic_cb = sensor_light_topic_cb},
         [SENSOR_MAGIC_STEP] = {.meta = NULL, .topic_cb = sensor_step_topic_cb}};

static void unsubscribe(FeatureInstanceHandle handle, int magic, bool is_active) {
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    SensorContext *th = static_cast<SensorContext *>(FeatureGetProtoData(proto_handle));
    sensor_event_t *event = th->events[magic];
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
    free(event);
    th->events[magic] = NULL;
}

void sensor_onRegister(const char *feature_name) {
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void sensor_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    SensorContext *th = static_cast<SensorContext *>(malloc(sizeof(SensorContext)));
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        th->events[i] = NULL;
    }
    FeatureSetProtoData(handle, th);
}

void sensor_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void sensor_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        unsubscribe(handle, i, false);
    }
}

void sensor_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    SensorContext *th = static_cast<SensorContext *>(FeatureGetProtoData(handle));
    if (!th) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }
    for (int i = 0; i < SENSOR_MAGIC_NUM; i++) {
        sensor_event_t *event = th->events[i];
        if (event) {
            int ret = uv_topic_unsubscribe(&event->topic);
            if (ret < 0) {
                FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe failed,ret=%d\n", file_tag,
                                  __FUNCTION__, ret);
            }
            ret = uv_topic_close(&event->topic);
            if (ret < 0) {
                FEATURE_LOG_ERROR("%s::%s() uv_topic_close failed,ret=%d\n", file_tag, __FUNCTION__,
                                  ret);
            }
            free(event);
            th->events[i] = NULL;
        }
    }
    free(th);
}

void sensor_onUnregister(const char *feature_name) {
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void sensor_wrap_subscribeAccelerometer(FeatureInstanceHandle feature, AppendData data,
                                        sensor_Accelerometer *param) {
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext *th = static_cast<SensorContext *>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    int interval = 0;
    if (strcmp(param->_interval, "game") == 0) {
        interval = 20000;
    } else if (strcmp(param->_interval, "ui") == 0) {
        interval = 60000;
    } else if (strcmp(param->_interval, "normal") == 0) {
        interval = 200000;
    } else {
        FEATURE_LOG_ERROR("%s::%s() param interval is invalid:%s\n", file_tag, __FUNCTION__,
                          param->_interval);
        return;
    }

    sensor_event_t *event = th->events[SENSOR_MAGIC_ACCEL];
    if (event) {
        uv_topic_unsubscribe(&th->events[SENSOR_MAGIC_ACCEL]->topic);
        free(event);
        th->events[SENSOR_MAGIC_ACCEL] = NULL;
    }
    event = static_cast<sensor_event_t *>(malloc(sizeof(sensor_event_t)));
    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->_reserved;
    meta.id = param->_cb;
    event->meta = meta;

    th->events[SENSOR_MAGIC_ACCEL] = event;
    int ret = uv_topic_subscribe(uv_default_loop(), &th->events[SENSOR_MAGIC_ACCEL]->topic,
                                 sensor_orb_table[SENSOR_MAGIC_ACCEL].meta,
                                 sensor_orb_table[SENSOR_MAGIC_ACCEL].topic_cb);
    if (ret < 0) {
        th->events[SENSOR_MAGIC_ACCEL] = NULL;
        free(event);
        FEATURE_LOG_ERROR("%s::%s() subscribe error:%d\n", file_tag, __FUNCTION__, ret);
    }
    uv_topic_set_interval(&th->events[SENSOR_MAGIC_ACCEL]->topic, interval);
}

void sensor_wrap_unsubscribeAccelerometer(FeatureInstanceHandle feature, AppendData data) {
    unsubscribe(feature, SENSOR_MAGIC_ACCEL, true);
}

void sensor_wrap_subscribeCompass(FeatureInstanceHandle feature, AppendData data,
                                  sensor_Compass *param) {}

void sensor_wrap_unsubscribeCompass(FeatureInstanceHandle feature, AppendData data) {
    unsubscribe(feature, SENSOR_MAGIC_COMPA, true);
}

void sensor_wrap_subscribeProximity(FeatureInstanceHandle feature, AppendData data,
                                    sensor_Proximity *param) {
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext *th = static_cast<SensorContext *>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    sensor_event_t *event = th->events[SENSOR_MAGIC_PROX];
    if (event) {
        uv_topic_unsubscribe(&event->topic);
        free(event);
        th->events[SENSOR_MAGIC_PROX] = NULL;
    }

    event = static_cast<sensor_event_t *>(malloc(sizeof(sensor_event_t)));
    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->_reserved;
    meta.id = param->_cb;
    event->meta = meta;

    th->events[SENSOR_MAGIC_PROX] = event;
    int ret = uv_topic_subscribe(uv_default_loop(), &th->events[SENSOR_MAGIC_PROX]->topic,
                                 sensor_orb_table[SENSOR_MAGIC_PROX].meta,
                                 sensor_orb_table[SENSOR_MAGIC_PROX].topic_cb);
    if (ret < 0) {
        th->events[SENSOR_MAGIC_PROX] = NULL;
        free(event);
        FeatureInvokeCallback(feature, param->_fail,
                              "The current device does not support the distance sensor", 203);
        FEATURE_LOG_ERROR("%s::%s() subscribe error:%d\n", file_tag, __FUNCTION__, ret);
    }
}

void sensor_wrap_unsubscribeProximity(FeatureInstanceHandle feature, AppendData data) {
    unsubscribe(feature, SENSOR_MAGIC_PROX, true);
}

void sensor_wrap_subscribeLight(FeatureInstanceHandle feature, AppendData data,
                                sensor_Light *param) {
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    SensorContext *th = static_cast<SensorContext *>(FeatureGetProtoData(proto_handle));
    if (th == NULL) {
        FEATURE_LOG_ERROR("%s::%s() sensor context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    sensor_event_t *event = th->events[SENSOR_MAGIC_LIGHT];
    if (event) {
        uv_topic_unsubscribe(&event->topic);
        free(event);
        th->events[SENSOR_MAGIC_LIGHT] = NULL;
    }

    event = static_cast<sensor_event_t *>(malloc(sizeof(sensor_event_t)));
    MetaData meta;
    meta.instance = feature;
    meta.reserved = param->_reserved;
    meta.id = param->_cb;
    event->meta = meta;

    th->events[SENSOR_MAGIC_LIGHT] = event;
    int ret = uv_topic_subscribe(uv_default_loop(), &th->events[SENSOR_MAGIC_LIGHT]->topic,
                                 sensor_orb_table[SENSOR_MAGIC_LIGHT].meta,
                                 sensor_orb_table[SENSOR_MAGIC_LIGHT].topic_cb);
    if (ret < 0) {
        th->events[SENSOR_MAGIC_LIGHT] = NULL;
        free(event);
        FEATURE_LOG_ERROR("%s::%s() subscribe error:%d\n", file_tag, __FUNCTION__, ret);
    }
}

void sensor_wrap_unsubscribeLight(FeatureInstanceHandle feature, AppendData data) {
    unsubscribe(feature, SENSOR_MAGIC_LIGHT, true);
}

void sensor_wrap_subscribeStepCounter(FeatureInstanceHandle feature, AppendData data,
                                      sensor_StepCount *param) {
    FeatureInvokeCallback(feature, param->_fail, "Current device does not support pedometer sensor",
                          1000);
}

void sensor_wrap_unsubscribeStepCounter(FeatureInstanceHandle feature, AppendData data) {
    unsubscribe(feature, SENSOR_MAGIC_STEP, true);
}
