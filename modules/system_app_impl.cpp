/*
 * Copyright (C) 2024 Xiaomi Corporation
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
 */

#include "feature_exports.h"
#include "feature_main_exports.h"
#include "framework/app_interface.h"
#include "system_app.h"
#include <string>

using ferry::IApplication;

void system_app_onRegister(const char* feature_name) { }
void system_app_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) { }
void system_app_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    void* app = (FeatureInstanceGetManagerUserData(handle, "app"));
    FeatureSetObjectData(handle, app);
}
void system_app_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FeatureSetObjectData(handle, nullptr);
}
void system_app_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) { }
void system_app_onUnregister(const char* feature_name) { }

FtString createString(const std::string& str)
{
    int sz = str.size();
    char* ret = (char*)FeatureMalloc(sz + 1, FT_STRING);
    if (ret) {
        sprintf(ret, "%s", str.c_str());
    }

    return ret;
}

system_app_AppInfo* system_app_wrap_getInfo(FeatureInstanceHandle feature, AppendData append_data)
{
    IApplication* app = static_cast<IApplication*>(FeatureGetObjectData(feature));

    system_app_AppInfo* appInfo = system_appMallocAppInfo();
    if (!appInfo) {
        return appInfo;
    }

    appInfo->packageName = createString(app->packageName());
    appInfo->name = createString(app->getAppManifest()->app_name());
    appInfo->versionName = createString(app->getAppManifest()->version());
    appInfo->versionCode = app->getAppManifest()->versionCode();
    appInfo->icon = createString(app->getAppManifest()->icon_path());
    appInfo->logLevel = createString(app->getAppManifest()->log_level());
    appInfo->source = system_appMallocAppSource();
    if (!appInfo->source) {
        return appInfo;
    }

    appInfo->source->packageName = createString(app->packageName());
    appInfo->source->type = createString("other");
    return appInfo;
}

void system_app_wrap_terminate(FeatureInstanceHandle feature, AppendData append_data)
{
    IApplication* app = static_cast<IApplication*>(FeatureGetObjectData(feature));
    if (!app->isExitRequest()) {
        app->exitAsync();
    } else {
        FEATURE_LOG_INFO("%s: app: %s already request exit!", __func__, app->packageName());
    }
}

FtBool system_app_wrap_canIUse(FeatureInstanceHandle feature, AppendData append_data, FtAny feature_method)
{
    if (feature_method == nullptr) {
        return false;
    }
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_type method_type = ft_get_type(ft_ctx, *feature_method);
    if (method_type != FT_TYPE_STRING) {
        return false;
    } else {
        const char* method_str = ft_to_string(ft_ctx, *feature_method);
        int len = strlen(method_str);
        if (!len)
            return false;
        IApplication* app = static_cast<IApplication*>(FeatureGetObjectData(feature));
        // 1. 从feature 中找
        if (method_str[0] == '@') {
            return app->hasFeature(method_str + 1);
        }

        // 2. 从gui组件中找
        return app->canIUseGui((const char*)method_str);
    }
}

FtAny system_app_wrap_loadLibrary(FeatureInstanceHandle feature, AppendData append_data, FtString name)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t ft_undef = ft_undefined(ft_ctx);
    FtAny ft_lib = (FtAny)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *ft_lib = ft_undef;
    FeatureManagerHandle hmanager = FeatureGetManagerHandleFromInstance(feature);
    if (!hmanager) {
        return ft_lib;
    }

    if (strcmp(name, "WECHAT_APP") == 0) {
        *ft_lib = FeatureRequire(hmanager, ft_undef, "service.wechat");
    } else {
        *ft_lib = FeatureRequire(hmanager, ft_undef, name);
    }

    return ft_lib;
}