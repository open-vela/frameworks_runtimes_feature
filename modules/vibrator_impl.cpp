/*
 * Copyright (C) 2024 Xiaomi Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "uv_ext.h"
#include "vibrator.h"
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <vibrator_api.h>

#define EFFECT_ID_BOUNDARY 10

static const char* file_tag = "[jidl_feature] vibrator_impl";

struct VibratorContext {
    uv_timer_t timer;
    int owner;
};

static bool is_positive_integer(double value)
{
    return value > 0 && (floor(value) == value);
}

static void vibrator_start_timer_cb(uv_timer_t* timer)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    VibratorContext* th = (VibratorContext*)timer->data;

    uv_timer_stop(timer);
    if (th->owner != -1) {
        th->owner = -1;
    }
}

static void timer_close_cb(uv_handle_t* handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    VibratorContext* th = (VibratorContext*)handle->data;
    if (th) {
        free(th);
    }
}

void system_vibrator_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_vibrator_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    VibratorContext* th = static_cast<VibratorContext*>(malloc(sizeof(VibratorContext)));
    th->owner = -1;
    th->timer.data = th;
    uv_timer_init(FeatureGetUVLoop(FeatureGetManagerHandleFromProto(handle)),
        &(th->timer));
    FeatureSetProtoData(handle, th);
}

void system_vibrator_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_vibrator_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_vibrator_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    VibratorContext* th = static_cast<VibratorContext*>(FeatureGetProtoData(handle));
    if (!th) {
        FEATURE_LOG_ERROR("%s::%s() vibraotr context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    uv_close((uv_handle_t*)&th->timer, timer_close_cb);
}

void system_vibrator_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_vibrator_wrap_vibrate(FeatureInstanceHandle feature, union AppendData append_data,
    system_vibrator_VibrateObject* obj)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    const char* mode_state[] = { "long", "short" };
    const char* mode;

    if (!obj) {
        vibrator_play_predefined(POP + EFFECT_ID_BOUNDARY, VIBRATION_DEFAULTES, nullptr);
        return;
    }

    mode = obj->mode;
    if (strncmp(mode, mode_state[0], strlen(mode_state[0])) == 0) {
        vibrator_play_predefined(POP + EFFECT_ID_BOUNDARY, VIBRATION_DEFAULTES, nullptr);
    } else if (strncmp(mode, mode_state[1], strlen(mode_state[1])) == 0) {
        vibrator_play_predefined(TICK + EFFECT_ID_BOUNDARY, VIBRATION_DEFAULTES, nullptr);
    }
}

void system_vibrator_wrap_start(FeatureInstanceHandle feature, union AppendData append_data,
    system_vibrator_StartObject* obj)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    VibratorContext* th = static_cast<VibratorContext*>(FeatureGetProtoData(proto_handle));
    int ret;
    if (!th) {
        FEATURE_LOG_ERROR("%s::%s() vibraotr context is NULL\n", file_tag, __FUNCTION__);
        return;
    }

    if (!obj || !is_positive_integer(obj->duration) || !is_positive_integer(obj->interval) || !is_positive_integer(obj->count)) {
        FEATURE_LOG_ERROR("unvaild arg!\n");
        if (FeatureCheckCallbackId(feature, obj->fail)) {
            FeatureInvokeCallback(feature, obj->fail, "unvaild arg", 202);
            FeatureRemoveCallback(feature, obj->fail);
        }
        goto out;
    }

    if (th->owner != -1) {
        FEATURE_LOG_ERROR("%s::%s() already has owner %d\n", file_tag, __FUNCTION__, th->owner);
        if (FeatureCheckCallbackId(feature, obj->fail)) {
            FeatureInvokeCallback(feature, obj->fail, "task already exists", 205);
            FeatureRemoveCallback(feature, obj->fail);
        }
        goto out;
    }

    ret = vibrator_play_interval(obj->duration, obj->interval, obj->count);
    if (ret < 0) {
        if (FeatureCheckCallbackId(feature, obj->fail)) {
            FeatureInvokeCallback(feature, obj->fail, "fail", ret);
        }
        goto out;
    } else {
        th->owner = 1;
        if (FeatureCheckCallbackId(feature, obj->success)) {
            system_vibrator_VibrateDataRet* data = system_vibratorMallocVibrateDataRet();
            data->id = th->owner;
            FeatureInvokeCallback(feature, obj->success, data);
            FeatureFreeValue(data);
        }
    }

    ret = uv_timer_start(&th->timer, vibrator_start_timer_cb,
        (obj->duration + obj->interval) * obj->count, 0);

out:
    if (FeatureCheckCallbackId(feature, obj->complete)) {
        FeatureInvokeCallback(feature, obj->complete);
    }

    if (FeatureCheckCallbackId(feature, obj->success)) {
        FeatureRemoveCallback(feature, obj->success);
    }
    if (FeatureCheckCallbackId(feature, obj->fail)) {
        FeatureRemoveCallback(feature, obj->fail);
    }
    if (FeatureCheckCallbackId(feature, obj->complete)) {
        FeatureRemoveCallback(feature, obj->complete);
    }
}

FtBool system_vibrator_wrap_stop(FeatureInstanceHandle feature, union AppendData append_data, FtInt id)
{
    FEATURE_LOG_INFO("%s::%s() id is %d\n", file_tag, __FUNCTION__, id);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    VibratorContext* th = static_cast<VibratorContext*>(FeatureGetProtoData(proto_handle));

    if (id != th->owner) {
        FEATURE_LOG_ERROR("%s::%s() id is not owner %d\n", file_tag, __FUNCTION__, th->owner);
        return false;
    }

    uv_timer_stop(&th->timer);

    int ret = vibrator_cancel();
    if (ret < 0) {
        FEATURE_LOG_ERROR("vibrator_stop failed, ret: %d\n", ret);
        return false;
    }
    th->owner = -1;

    return true;
}

FtInt system_vibrator_wrap_getSystemDefaultMode(FeatureInstanceHandle feature,
    union AppendData append_data)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    uint8_t disable;
    vibrator_intensity_e mode;
    int ret = vibrator_is_disabled(&disable);
    if (ret < 0)
        return -1;

    if (disable) {
        return 0;
    }

    ret = vibrator_get_intensity(&mode);
    if (ret < 0)
        return -1;

    switch (mode) {
    case VIBRATION_INTENSITY_LOW:
    case VIBRATION_INTENSITY_MEDIUM:
        return 1;
    case VIBRATION_INTENSITY_HIGH:
        return 2;
    default:
        return -1;
    }
}