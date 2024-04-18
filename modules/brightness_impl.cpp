
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include "src/brightness.h"
#include <BrightnessService.h>
#include <cstddef>
#include <os/brightness/BnBrightnessObserver.h>
#include <os/brightness/IBrightnessService.h>

class MonitorBrightnessCallback : public os::brightness::BnBrightnessObserver {
public:
    android::binder::Status onBrightnessChanged(int32_t level) override
    {
        if (feature == nullptr) {
            return android::binder::Status::fromExceptionCode(android::binder::Status::Exception::EX_NULL_POINTER);
        }
        ft_context_ref ft_ctx = FeatureGetContext(feature);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_level = ft_from_int(ft_ctx, level);
        ft_obj_set_property(ft_ctx, ret_obj, "value", ret_level);

        FeatureInvokeCallback(feature, cid, &ret_obj);
        return android::binder::Status::ok();
    }
    FeatureInstanceHandle feature {};
    FtCallbackId cid {};
};

android::sp<os::brightness::IBrightnessService> service;
android::sp<MonitorBrightnessCallback> callback;

void system_brightness_onRegister(const char* feature_name) { }
void system_brightness_onCreate(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle) { }
void system_brightness_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    FEATURE_LOG_INFO("defaultServiceManager(): %p", sm.get());

    // obtain brightness.service
    android::sp<android::IBinder> binder = sm->getService(os::brightness::BrightnessService::name());
    if (binder == NULL) {
        FEATURE_LOG_INFO("brightness service binder is null, abort...");
        return;
    }
    FEATURE_LOG_INFO("brightness service binder is %p", binder.get());
    service = android::interface_cast<os::brightness::IBrightnessService>(binder);
    FEATURE_LOG_INFO("brightness service is %p", service.get());
}

void detach()
{
    if (service && callback) {
        service->unmonitorBrightness(callback);
        callback->cid = 0;
        callback->feature = nullptr;
        callback = nullptr;
        service = nullptr;
    }
}

void system_brightness_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    detach();
}

void system_brightness_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    detach();
}

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
        FeatureInvokeCallback(feature, fail, "execute error", code);
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
    if (!service) {
        FEATURE_LOG_ERROR("FATAL: service == NULL");
        return;
    }
    int32_t level;
    auto status = service->getTargetBrightness(&level);
    FEATURE_LOG_INFO("brightness target level is %d", level);

    do_callback(feature, level, status.isOk() ? 0 : -1, param->success, param->fail, param->complete);
}

void system_brightness_wrap_setValue(FeatureInstanceHandle feature,
    union AppendData append_data,
    system_brightness_SetValueParam* param)
{
    if (!service) {
        FEATURE_LOG_ERROR("FATAL: service == NULL");
        return;
    }
    int ret = -1;
    if (param->value >= 0 && param->value <= 255) {
        auto status = service->setTargetBrightness(param->value, 0);
        ret = status.isOk() ? 0 : -1;
        FEATURE_LOG_INFO("brightness target level is %d", param->value);
    }
    do_callback(feature, ret, ret, param->success, param->fail, param->complete);
}

void system_brightness_wrap_getMode(FeatureInstanceHandle feature,
    union AppendData append_data,
    system_brightness_GetModeParam* param)
{
    if (!service) {
        FEATURE_LOG_ERROR("FATAL: service == NULL");
        return;
    }
    os::brightness::Mode mode;
    auto status = service->getBrightnessMode(&mode);
    FEATURE_LOG_INFO("brightness target mode is %d", (int)mode);
    do_callback(feature, !(int)mode, status.isOk() ? 0 : -1,
        param->success, param->fail, param->complete);
}

void system_brightness_wrap_setMode(FeatureInstanceHandle feature,
    union AppendData append_data,
    system_brightness_SetModeParam* param)
{
    if (!service) {
        FEATURE_LOG_ERROR("FATAL: service == NULL");
        return;
    }
    int ret = -1;
    if (param->mode == 0 || param->mode == 1) {
        auto status = service->setBrightnessMode(param->mode == 1 ? os::brightness::Mode::AUTO : os::brightness::Mode::MANUAL);
        ret = status.isOk() ? 0 : -1;
    }
    do_callback(feature, ret, ret, param->success, param->fail, param->complete);
    FEATURE_LOG_INFO("brightness target mode is %d", param->mode);
}

void system_brightness_wrap_setKeepScreenOn(
    FeatureInstanceHandle feature, union AppendData append_data,
    system_brightness_SetKeepScreenOnParam* param)
{
    // TODO
}

void system_brightness_set_onbrightnesschanged(FeatureInstanceHandle feature, union AppendData append_data, FtCallbackId cb)
{
    if (!service) {
        FEATURE_LOG_ERROR("FATAL: service == NULL");
        return;
    }
    callback = android::sp<MonitorBrightnessCallback>::make();
    callback->feature = feature;
    callback->cid = cb;
    service->monitorBrightness(callback);
}
