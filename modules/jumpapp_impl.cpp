#include "feature_log.h"
#include "jumpapp.h"

void jumpApp_onRegister(const char* feature_name) { }
void jumpApp_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) { }
void jumpApp_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle) { }
void jumpApp_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle) { }
void jumpApp_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) { }
void jumpApp_onUnregister(const char* feature_name) { }

#define NATIVE_APP_PREFIX "native://"
extern "C" int quickapp_navigate_async(const char* uri, const char* arg);

void jumpApp_wrap_jumpApp(FeatureInstanceHandle feature, AppendData data,
    FtString uri, FtString arg)
{
    if (strstr(uri, NATIVE_APP_PREFIX) == NULL) {
        FEATURE_LOG_ERROR("[jump native] uri format error!");
        return;
    }

    FEATURE_LOG_INFO("[jump native] uri:%s, param:%s", uri, arg);
    quickapp_navigate_async(uri + strlen(NATIVE_APP_PREFIX), arg);
}
