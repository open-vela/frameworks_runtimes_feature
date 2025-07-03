#include "brightness.h"
#include <cstddef>
#include <unordered_set>
#include <utils/Log.h>
#include <utils/String8.h>

#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
#include <BrightnessService.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <os/brightness/BnBrightnessObserver.h>
#include <os/brightness/IBrightnessService.h>
#else
// miwear headers
#include <nuttx/list.h>
#include <uv_ext.h>
#endif

/**
 * @brief invoke result callback
 *
 * @param feature feature handle
 * @param value current brightness value returned by brightness API
 * @param code error code
 * @param succ success callback, invoke if we success
 * @param fail fail callback, invoke if we are failed
 * @param comp complete callback, invoke when we finished
 */
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

#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
#define CHECK_SERVICE_VALID()                                              \
    BrightnessData* data = (BrightnessData*)FeatureGetObjectData(feature); \
    if (!data) {                                                           \
        FEATURE_LOG_ERROR("FATAL: DATA == NULL");                          \
    }                                                                      \
    auto& service = data->service;                                         \
    if (!service) {                                                        \
        FEATURE_LOG_ERROR("FATAL: service == NULL");                       \
        return;                                                            \
    }

class MonitorBrightnessCallback : public os::brightness::BnBrightnessObserver {
public:
    android::binder::Status onBrightnessChanged(os::brightness::MessageType type, int32_t arg) override
    {
        if (feature == nullptr) {
            return android::binder::Status::fromExceptionCode(android::binder::Status::Exception::EX_NULL_POINTER);
        }
        ft_context_ref ft_ctx = FeatureGetContext(feature);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        if (type == os::brightness::MessageType::BRIGHTNESS_MODE) {
            ft_value_t ret_mode = ft_from_int(ft_ctx, !arg);
            ft_obj_set_property(ft_ctx, ret_obj, "mode", ret_mode);
            for (auto i : mode_cid) {
                FeatureInvokeCallback(feature, i, &ret_obj);
            }
        } else {
            ft_value_t ret_level = ft_from_int(ft_ctx, arg);
            ft_obj_set_property(ft_ctx, ret_obj, "value", ret_level);
            for (auto i : value_cid) {
                FeatureInvokeCallback(feature, i, &ret_obj);
            }
        }
        ft_free_value(ft_ctx, ret_obj);
        return android::binder::Status::ok();
    }
    FeatureInstanceHandle feature {};
    std::unordered_set<FtCallbackId> value_cid;
    std::unordered_set<FtCallbackId> mode_cid;
};

struct BrightnessData {
    android::sp<os::brightness::IBrightnessService> service;
    android::sp<MonitorBrightnessCallback> callback;
};
#else
typedef struct {
    list_node node;
    FeatureInstanceHandle handle;
    void* param;
    int type;
    bool is_close;
    uv_brightness_handle_t brightness;
} Brightness;

enum {
    BRIGHTNESS_GETMODE,
    BRIGHTNESS_SETMODE,
    BRIGHTNESS_GETVALUE,
    BRIGHTNESS_SETVALUE,
    BRIGHTNESS_KEEPON,
    BRIGHTNESS_RECOVER,

};

static Brightness* __brightness_get(FeatureInstanceHandle obj)
{
    Brightness* th = static_cast<Brightness*>(FeatureGetObjectData(obj));
    if (!th) {
        return NULL;
    }

    Brightness* handle = static_cast<Brightness*>(malloc(sizeof(Brightness)));
    if (!handle) {
        return NULL;
    }

    list_add_tail(&th->node, &handle->node);
    handle->brightness = th->brightness;
    handle->is_close = th->is_close;
    handle->handle = th->handle;

    return handle;
}

#define __GET_BRIGHTNESS__(val) \
    Brightness* th = __brightness_get(val);

static void brightness_cb(int status, int val, void* data)
{
    Brightness* th = static_cast<Brightness*>(data);

    if (th->is_close == true) {
        FEATURE_LOG_INFO("[brightness] is_close");
        if (th->param) {
            FeatureFreeValue(th->param);
        }
        list_delete(&th->node);
        free(th);
        return;
    }
    if (status != 0) {
        FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
    }

    FEATURE_LOG_INFO("[brightness] brightness_cb status: %d, val:%d", status, val);
    switch (th->type) {
    case BRIGHTNESS_GETVALUE: {
        system_brightness_GetValueParam* pGetValueParam = (system_brightness_GetValueParam*)th->param;
        do_callback(th->handle, val, status == 0, pGetValueParam->success, pGetValueParam->fail, pGetValueParam->complete);
    } break;
    case BRIGHTNESS_SETVALUE: {
        system_brightness_SetValueParam* pSetValueParam = (system_brightness_SetValueParam*)th->param;
        do_callback(th->handle, status == 0 ? 0 : -1, status == 0, pSetValueParam->success, pSetValueParam->fail, pSetValueParam->complete);
    } break;
    case BRIGHTNESS_GETMODE: {
        system_brightness_GetModeParam* pGetModeParam = (system_brightness_GetModeParam*)th->param;
        do_callback(th->handle, val, status == 0, pGetModeParam->success, pGetModeParam->fail, pGetModeParam->complete);
    } break;
    case BRIGHTNESS_SETMODE: {
        system_brightness_SetModeParam* pSetModeParam = (system_brightness_SetModeParam*)th->param;
        do_callback(th->handle, status == 0 ? 0 : -1, status == 0, pSetModeParam->success, pSetModeParam->fail, pSetModeParam->complete);
    } break;
    case BRIGHTNESS_KEEPON: {
        system_brightness_SetKeepScreenOnParam* pKeepOnParam = (system_brightness_SetKeepScreenOnParam*)th->param;
        do_callback(th->handle, status == 0 ? 0 : -1, status == 0, pKeepOnParam->success, pKeepOnParam->fail, pKeepOnParam->complete);
    } break;
    }

    if (th->param) {
        FeatureFreeValue(th->param);
    }
    list_delete(&th->node);
    free(th);
}

#endif

void system_brightness_onRegister(const char* feature_name)
{
}
void system_brightness_onCreate(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle) { }
void system_brightness_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    if (FeatureGetObjectData(handle) != nullptr) {
        return;
    }
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    BrightnessData* data = new BrightnessData;
    FeatureSetObjectData(handle, data);
    android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    FEATURE_LOG_INFO("defaultServiceManager(): %p", sm.get());

    // obtain brightness.service
    android::sp<android::IBinder> binder = sm->getService(os::brightness::BrightnessService::name());
    if (binder == NULL) {
        FEATURE_LOG_INFO("brightness service binder is null, abort...");
        return;
    }
    FEATURE_LOG_INFO("brightness service binder is %p", binder.get());
    android::sp<os::brightness::IBrightnessService> service = android::interface_cast<os::brightness::IBrightnessService>(binder);
    data->service = service;
    FEATURE_LOG_INFO("brightness service is %p", service.get());
#else
    Brightness* th = static_cast<Brightness*>(calloc(1, sizeof(*th)));
    if (!th) {
        FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
        return;
    }

    list_initialize(&th->node);
    th->is_close = false;
    th->handle = handle;

    auto hMgr = FeatureGetManagerHandleFromInstance(handle);
    if (uv_brightness_init(FeatureGetUVLoop(hMgr), &th->brightness) != 0) {
        free(th);
        FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
        return;
    }
    //绑定
    FeatureSetObjectData(handle, th);
#endif
}

void detach(FeatureInstanceHandle handle)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    //
    BrightnessData* data = (BrightnessData*)FeatureGetObjectData(handle);
    FEATURE_LOG_INFO("%s data=%p", __func__, data);
    if (data && data->service) {
        if (data->callback) {
            FEATURE_LOG_INFO("unmonitorBrightness %p", data->callback.get());
            data->service->unmonitorBrightness(data->callback);
            data->callback->feature = nullptr;
            data->callback->value_cid.clear();
            data->callback->mode_cid.clear();
        }
        data->callback = nullptr;
        data->service = nullptr;
        delete data;
        FeatureSetObjectData(handle, nullptr);
    }
#else
    Brightness* th = static_cast<Brightness*>(FeatureGetObjectData(handle));
    if (th) {
        list_node *node, *tmp;
        list_for_every_safe(&th->node, node, tmp)
        {
            if (node == &th->node) {
                // note: this branch should be optimized.
                continue;
            }
            Brightness* th_ = (Brightness*)(node);
            if (th_->is_close == true) {
                FEATURE_LOG_INFO("[brightness] is_close");
                continue;
            }
            if (th_->param) {
                FeatureFreeValue(th_->param);
            }
            free(node);
        }
        if (th->brightness) {
            if (uv_brightness_recovery(th->brightness, NULL, th) != 0) {
                FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
                return;
            }
            uv_brightness_close(th->brightness);
            th->brightness = NULL;
        }
        th->is_close = true;
        free(th);
        th = NULL;
    }
#endif
}

void system_brightness_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    detach(handle);
}

void system_brightness_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
}

void system_brightness_onUnregister(const char* feature_name) { }

void system_brightness_wrap_getValue(FeatureInstanceHandle feature,
    AppendData append_data,
    system_brightness_GetValueParam* param)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    CHECK_SERVICE_VALID() // Check whether data and service are empty
    int32_t level; // Call the function to get the brightness and store the brightness value in level
    auto status = service->getCurrentBrightness(&level); // Call the function to get the brightness，return success or fail
    FEATURE_LOG_INFO("brightness target level is %d", level);
    do_callback(feature, level, status.isOk() ? 0 : -1, param->success, param->fail, param->complete);
#else
    __GET_BRIGHTNESS__(feature);
    if (!th) {
        FEATURE_LOG_ERROR("get th failed !");
        return;
    }

    th->type = BRIGHTNESS_GETVALUE;
    th->param = FeatureDupValue(param);
    FEATURE_LOG_INFO("[brightness] __brightness_getval");
    int status = uv_brightness_getval(th->brightness, brightness_cb, th);
    if (status != 0) {
        do_callback(feature, status, FT_ERR_GENERAL, param->success, param->fail, param->complete);
        FeatureFreeValue(th->param);
        list_delete(&th->node);
        free(th);
        FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
        return;
    }

    return;
#endif
}

void system_brightness_wrap_setValue(FeatureInstanceHandle feature,
    AppendData append_data,
    system_brightness_SetValueParam* param)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    CHECK_SERVICE_VALID()
    int ret = -1;
    if (param->value >= 0 && param->value <= 255) {
        auto status = service->setTargetBrightness(param->value, 0);
        ret = status.isOk() ? 0 : -1;
        FEATURE_LOG_INFO("brightness target level is %d", param->value);
    }
    do_callback(feature, ret, ret, param->success, param->fail, param->complete);
#else
    __GET_BRIGHTNESS__(feature);
    if (!th) {
        FEATURE_LOG_ERROR("get th failed !");
        return;
    }

    th->type = BRIGHTNESS_SETVALUE;
    th->param = FeatureDupValue(param);
    FEATURE_LOG_INFO("[brightness] __brightness_setval :%d", param->value);
    int status = uv_brightness_setval(th->brightness, param->value, brightness_cb, th);
    if (status != 0) {
        do_callback(feature, status, FT_ERR_GENERAL, param->success, param->fail, param->complete);
        FeatureFreeValue(th->param);
        list_delete(&th->node);
        free(th);
        FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
        return;
    }

    return;
#endif
}

void system_brightness_wrap_getMode(FeatureInstanceHandle feature,
    AppendData append_data,
    system_brightness_GetModeParam* param)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    CHECK_SERVICE_VALID()
    os::brightness::Mode mode;
    auto status = service->getBrightnessMode(&mode);
    FEATURE_LOG_INFO("brightness target mode is %d", (int)mode);
    do_callback(feature, !(int)mode, status.isOk() ? 0 : -1,
        param->success, param->fail, param->complete);
#else
    __GET_BRIGHTNESS__(feature);

    th->type = BRIGHTNESS_GETMODE;
    th->param = FeatureDupValue(param);
    FEATURE_LOG_INFO("[brightness] __brightness_getmode");
    int status = uv_brightness_getmode(th->brightness, brightness_cb, th);
    if (status != 0) {
        do_callback(feature, status, FT_ERR_GENERAL, param->success, param->fail, param->complete);
        FeatureFreeValue(th->param);
        list_delete(&th->node);
        free(th);
        FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
        return;
    }

    return;
#endif
}

void system_brightness_wrap_setMode(FeatureInstanceHandle feature,
    AppendData append_data,
    system_brightness_SetModeParam* param)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    CHECK_SERVICE_VALID()
    int ret = -1;
    if (param->mode == 0 || param->mode == 1) {
        auto status = service->setBrightnessMode(param->mode == 1 ? os::brightness::Mode::AUTO : os::brightness::Mode::MANUAL);
        ret = status.isOk() ? 0 : -1;
    }
    do_callback(feature, ret, ret, param->success, param->fail, param->complete);
    FEATURE_LOG_INFO("brightness target mode is %d", param->mode);
#else
    __GET_BRIGHTNESS__(feature);
    th->type = BRIGHTNESS_SETMODE;
    th->param = FeatureDupValue(param);
    FEATURE_LOG_INFO("[brightness] __brightness_setmode: %d", param->mode);
    int status = uv_brightness_setmode(th->brightness, param->mode, brightness_cb, th);
    if (status != 0) {
        do_callback(feature, status, FT_ERR_GENERAL, param->success, param->fail, param->complete);
        FeatureFreeValue(th->param);
        list_delete(&th->node);
        free(th);
        FEATURE_LOG_ERROR("[brightness] exception %d", __LINE__);
        return;
    }

    return;
#endif
}

void system_brightness_wrap_setKeepScreenOn(
    FeatureInstanceHandle feature, AppendData append_data,
    system_brightness_SetKeepScreenOnParam* param)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    // TODO
#else
    __GET_BRIGHTNESS__(feature);

    th->type = BRIGHTNESS_KEEPON;
    th->param = FeatureDupValue(param);
    if (param->keepScreenOn == true) {
        if (uv_brightness_turnon(th->brightness, NULL, NULL) != 0) {
            FEATURE_LOG_ERROR("brightness uv_brightness_turnon failed");
            FeatureFreeValue(th->param);
            return;
        }
    }

    int status = uv_brightness_setkeepon(th->brightness, param->keepScreenOn, brightness_cb, th);
    if (status != 0) {
        FEATURE_LOG_ERROR("brightness uv_brightness_setkeepon failed");
        do_callback(feature, status, FT_ERR_GENERAL, param->success, param->fail, param->complete);
        FeatureFreeValue(th->param);
        return;
    }

    return;
#endif
}

void system_brightness_set_onbrightnesschanged(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId cb)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    CHECK_SERVICE_VALID()
    if (!data->callback) {
        data->callback = android::sp<MonitorBrightnessCallback>::make();
        FEATURE_LOG_INFO("monitorBrightness %p", data->callback.get());
        service->monitorBrightness(data->callback);
    }
    data->callback->feature = feature;
    data->callback->value_cid.insert(cb);
#endif
}

void system_brightness_set_onmodechanged(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId cb)
{
#ifdef CONFIG_SYSTEM_BRIGHTNESS_SERVICE
    CHECK_SERVICE_VALID()
    if (!data->callback) {
        data->callback = android::sp<MonitorBrightnessCallback>::make();
        FEATURE_LOG_INFO("monitorBrightness %p", data->callback.get());
        service->monitorBrightness(data->callback);
    }
    data->callback->feature = feature;
    data->callback->mode_cid.insert(cb);
#endif
}
