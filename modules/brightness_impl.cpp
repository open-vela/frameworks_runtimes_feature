extern "C" {
#include "brightness_service.h"
}
#include "src/brightness.h"
#include <cstddef>

void system_brightness_onRegister(const char* feature_name)
{
}
void system_brightness_onCreate(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle) { }
void system_brightness_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle) { }
void system_brightness_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle) { }
void system_brightness_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle) { }
void system_brightness_onUnregister(const char* feature_name) { }

static void do_callback(FeatureInstanceHandle feature, int value, int code,
    FtCallbackId succ, FtCallbackId fail,
    FtCallbackId comp)
{
    if (code >= 0) {
        system_brightness_GetValueModeSuccCBParam param;
        param.value = value;
        param.mode = value;
        FeatureInvokeCallback(feature, succ, &param);
    } else {
        FeatureInvokeCallback(feature, fail, code);
    }
    FeatureInvokeCallback(feature, comp);

    FeatureRemoveCallback(feature, succ);
    FeatureRemoveCallback(feature, fail);
    FeatureRemoveCallback(feature, comp);
}

void system_brightness_wrap_getValue(FeatureInstanceHandle feature,
    union AppendData append_data,
    system_brightness_GetValueParam* param)
{

    int ret = brightness_get_target(brightness_get_system_session());
    do_callback(feature, ret, ret, param->success, param->fail, param->complete);
}

void system_brightness_wrap_setValue(FeatureInstanceHandle feature,
    union AppendData append_data,
    system_brightness_SetValueParam* param)
{
    int ret = brightness_set_target(brightness_get_system_session(), param->value, 0);
    do_callback(feature, ret, ret, param->success, param->fail, param->complete);
}

void system_brightness_wrap_getMode(FeatureInstanceHandle feature,
    union AppendData append_data,
    system_brightness_GetModeParam* param)
{
    int ret = brightness_get_mode(brightness_get_system_session());
    do_callback(feature, ret == BRIGHTNESS_MODE_MANUAL ? 0 : 1, ret,
        param->success, param->fail, param->complete);
}

void system_brightness_wrap_setMode(FeatureInstanceHandle feature,
    union AppendData append_data,
    system_brightness_SetModeParam* param)
{
    int ret = brightness_set_mode(brightness_get_system_session(), param->mode == 0 ? BRIGHTNESS_MODE_MANUAL : BRIGHTNESS_MODE_AUTO);
    do_callback(feature, ret, ret, param->success, param->fail, param->complete);
}

void system_brightness_wrap_setKeepScreenOn(
    FeatureInstanceHandle feature, union AppendData append_data,
    system_brightness_SetKeepScreenOnParam* param)
{
    // TODO
}
