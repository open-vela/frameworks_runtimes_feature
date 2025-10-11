#include <math.h>
#include <nuttx/nuttx.h>
#include <system/state.h>

#include "battery.h"
#include "uv.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] battery_impl";
#define INVOKE_SUCCESS_CB(feature, cb, ...)                             \
    do {                                                                \
        if (cb && !FeatureInvokeCallback(feature, cb, ##__VA_ARGS__)) { \
            FEATURE_LOG_ERROR("invoke success callback failed !");      \
        }                                                               \
    } while (0)

#define INVOKE_FAIL_CB(feature, cb, msg, code)                      \
    do {                                                            \
        if (cb && !FeatureInvokeCallback(feature, cb, msg, code)) { \
            FEATURE_LOG_ERROR("invoke fail callback failed !");     \
        }                                                           \
    } while (0)

#define REMOVE_ALL_CALLBACK(__succ__, __fail__)   \
    do {                                          \
        FeatureRemoveCallback(feature, __succ__); \
        FeatureRemoveCallback(feature, __fail__); \
    } while (0)

void system_battery_wrap_getStatus(FeatureInstanceHandle feature, AppendData append_data, system_battery_getstatusParm* param)
{
    int fd;
    struct battery_state battery;
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    system_battery_getStatusRet* status = static_cast<system_battery_getStatusRet*>(FeatureGetProtoData(proto_handle));

    fd = orb_subscribe(ORB_ID(battery_state));
    if (fd < 0) {
        FEATURE_LOG_ERROR("orb_subscribe battery_state failed");
        INVOKE_FAIL_CB(feature, param->fail, "orb_subscribe battery_state failed", 200);
        goto out;
    }

    if (orb_copy(ORB_ID(battery_state), fd, &battery) != OK) {
        FEATURE_LOG_ERROR("orb_copy battery_state failed");
        INVOKE_FAIL_CB(feature, param->fail, "orb_copy battery_state failed", 200);
        goto out;
    }

    status->charging = (battery.state == 1);
    status->level = round(battery.level / 10) / 10;
    INVOKE_SUCCESS_CB(feature, param->success, status);

out:
    orb_unsubscribe(fd);
    REMOVE_ALL_CALLBACK(param->success, param->fail);
    if (param->complete) {
        FeatureInvokeCallback(feature, param->complete, "getLocation complete");
        FeatureRemoveCallback(feature, param->complete);
    }
}

void system_battery_onRegister(const char* feature_name)
{
}
void system_battery_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    system_battery_getStatusRet* ret = system_batteryMallocgetStatusRet();
    if (!ret) {
        FEATURE_LOG_ERROR("%s::%s() malloc error", file_tag, __FUNCTION__);
        return;
    }

    FeatureSetProtoData(handle, ret);
}
void system_battery_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
}
void system_battery_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
}
void system_battery_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    system_battery_getStatusRet* ret = static_cast<system_battery_getStatusRet*>(FeatureGetProtoData(handle));
    FeatureFreeValue(ret);
}
void system_battery_onUnregister(const char* feature_name)
{
}