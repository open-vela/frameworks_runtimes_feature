#include "feature_log.h"
#include "jumpapp.h"
#ifdef CONFIG_QUICKAPP_VAPP_XMS
#include "application.h"
#include "feature_context_qjs.h"
#include "jse_api.h"
#endif
#include <cstring>
#include <limits.h>

#define JUMPAPP_LIFECYCLE_DEBUG() \
    FEATURE_LOG_DEBUG("[jidl_feature] jumpApp_impl:: %s()", __FUNCTION__)

void jumpApp_onRegister(const char* feature_name)
{
    JUMPAPP_LIFECYCLE_DEBUG();
}
void jumpApp_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    JUMPAPP_LIFECYCLE_DEBUG();
}
void jumpApp_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    JUMPAPP_LIFECYCLE_DEBUG();
}
void jumpApp_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    JUMPAPP_LIFECYCLE_DEBUG();
}
void jumpApp_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    JUMPAPP_LIFECYCLE_DEBUG();
}
void jumpApp_onUnregister(const char* feature_name)
{
    JUMPAPP_LIFECYCLE_DEBUG();
}

#define NATIVE_APP_PREFIX "native://"
#define QUICK_APP_PREFIX "hap://app/"

void jumpApp_wrap_jumpApp(FeatureInstanceHandle feature, AppendData append_data, FtString uri, FtString arg)
{
    if (strstr(uri, NATIVE_APP_PREFIX) == NULL) {
        FEATURE_LOG_ERROR("[jump native] uri format error!");
        return;
    }

    FEATURE_LOG_DEBUG("[jump native] uri:%s, param:%s", uri, arg);
    // TODO jump to native application
}

void jumpApp_wrap_launchQuickApp(FeatureInstanceHandle feature, AppendData append_data, FtString uri)
{
    FEATURE_LOG_DEBUG("[jump native] jumpApp_wrap_launchQuickApp uri: %s", uri);
    if (strncmp(uri, QUICK_APP_PREFIX, strlen(QUICK_APP_PREFIX)) != 0) {
        FEATURE_LOG_ERROR("[jump native] uri should start with %s!", QUICK_APP_PREFIX);
        return;
    }

    const char* pos_pkg = uri + strlen(QUICK_APP_PREFIX);
    const char* pos_path = strchr(pos_pkg, '/');

    char pkg[PATH_MAX] = "";
    if (pos_path == NULL) {
        sprintf(pkg, "%s", pos_pkg);
    } else {
        strncpy(pkg, pos_pkg, pos_path - pos_pkg);
    }

    if (pkg[0] == '\0') {
        FEATURE_LOG_ERROR("[jump native] package name is null!");
    }

#ifdef CONFIG_QUICKAPP_VAPP_XMS
    os::app::Intent intent;
    intent.setTarget(pkg);
    if (pos_path != NULL) {
        intent.setData(pos_path);
    }

    ft_context_ref ctx = FeatureGetContext(feature);
    Application* app = static_cast<Application*>(jse_get_context_opaque(GET_QJS_CTX(ctx)));
    assert(app != NULL);

    os::app::Context* xms_context = static_cast<os::app::Context*>(app->getXmsContext());
    if (xms_context == NULL) {
        FEATURE_LOG_ERROR("[jump native] no xms_context");
        return;
    }
    xms_context->startActivity(intent);
#else
    FEATURE_LOG_ERROR("[jump native] Failed to jump for no CONFIG_QUICKAPP_VAPP_XMS.");
#endif
}
