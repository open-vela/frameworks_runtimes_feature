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

#include "jidl/package.h"
#include "pm/PackageManager.h"

static const char* file_tag = "[jidl_feature] package_impl";

using namespace os::pm;
using android::binder::Status;

void system_internal_package_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_internal_package_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_internal_package_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PackageManager* pm = new PackageManager();
    FeatureSetObjectData(handle, pm);
}

void system_internal_package_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetObjectData(handle));
    if (pm) {
        free(pm);
        pm = NULL;
    }
}

void system_internal_package_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_internal_package_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

static char* StringToFtString(std::string str)
{
    int len = str.length();
    char* ftStr = (char*)FeatureMalloc(len + 1, FT_STRING);
    strcpy(ftStr, str.c_str());
    return ftStr;
}

static void NativePackageInfoToJSPackageInfo(PackageInfo info,
    system_internal_package_PackageInfo* js_info)
{
    if (js_info == NULL) {
        return;
    }

    js_info->packageName = StringToFtString(info.packageName);
    js_info->name = StringToFtString(info.name);
    js_info->icon = StringToFtString(info.icon);
    js_info->installedPath = StringToFtString(info.installedPath);
    js_info->manifest = StringToFtString(info.manifest);
    js_info->appType = StringToFtString(info.appType);
    js_info->version = StringToFtString(info.version);
    js_info->installTime = StringToFtString(info.installTime);
    js_info->appSize = info.size;
    js_info->extra = StringToFtString("");
}

class FeatureInstallListener : public BnInstallObserver {
public:
    FeatureInstallListener(FeatureInstanceHandle handle, FtCallbackId progress_cb,
        FtCallbackId result_cb)
        : mHandle(handle)
        , mProgressCb(progress_cb)
        , mResultCb(result_cb)
    {
    }
    Status onInstallProcess(const std::string& packageName, int32_t process) override
    {
        FeatureInvokeCallback(mHandle, mProgressCb, packageName, process);
        return Status::ok();
    }

    Status onInstallResult(const std::string& packageName, int32_t code,
        const std::string& msg) override
    {
        FeatureInvokeCallback(mHandle, mResultCb, packageName.c_str(), code, msg.c_str());
        return Status::ok();
    }

private:
    FeatureInstanceHandle mHandle;
    FtCallbackId mProgressCb;
    FtCallbackId mResultCb;
};

class FeatureUninstallListener : public BnUninstallObserver {
public:
    FeatureUninstallListener(FeatureInstanceHandle handle, FtCallbackId result_cb)
        : mHandle(handle)
        , mResultCb(result_cb)
    {
    }
    Status onUninstallResult(const std::string& packageName, int32_t code,
        const std::string& msg) override
    {
        FeatureInvokeCallback(mHandle, mResultCb, packageName.c_str(), code, msg.c_str());
        return Status::ok();
    }

private:
    FeatureInstanceHandle mHandle;
    FtCallbackId mResultCb;
};

FtArray* system_internal_package_wrap_getAllPackageInfo(FeatureInstanceHandle feature,
    AppendData append_data)
{
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetObjectData(feature));
    if (pm == NULL) {
        FEATURE_LOG_ERROR("%s::%s() pm is NULL\n", file_tag, __FUNCTION__);
        return NULL;
    }
    std::vector<PackageInfo> pkgInfos;
    int status = pm->getAllPackageInfo(&pkgInfos);
    if (status) {
        FEATURE_LOG_ERROR("%s::%s() getAllPackageInfo failed, status:%d\n", file_tag, __FUNCTION__,
            status);
        return NULL;
    }
    FtArray* strArray = system_internal_package_malloc_PackageInfo_struct_type_array();
    strArray->_size = pkgInfos.size();
    strArray->_element = malloc(sizeof(char*) * strArray->_size);
    for (size_t i = 0; i < pkgInfos.size(); i++) {
        system_internal_package_PackageInfo* js_pkg = system_internal_packageMallocPackageInfo();
        NativePackageInfoToJSPackageInfo(pkgInfos[i], js_pkg);
        ((system_internal_package_PackageInfo**)strArray->_element)[i] = js_pkg;
    }
    return strArray;
}

system_internal_package_PackageInfo* system_internal_package_wrap_getPackageInfo(
    FeatureInstanceHandle feature, AppendData append_data, FtString packageName)
{
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetObjectData(feature));
    if (pm == NULL) {
        FEATURE_LOG_ERROR("%s::%s() pm is NULL\n", file_tag, __FUNCTION__);
        return NULL;
    }
    os::pm::PackageInfo info;
    int ret = pm->getPackageInfo(packageName, &info);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s() getPackageInfo failed\n", file_tag, __FUNCTION__);
        return NULL;
    }
    system_internal_package_PackageInfo* js_info = system_internal_packageMallocPackageInfo();
    NativePackageInfoToJSPackageInfo(info, js_info);
    return js_info;
}

FtInt system_internal_package_wrap_clearAppCache(FeatureInstanceHandle feature,
    AppendData append_data, FtString packageName)
{
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetObjectData(feature));
    if (pm == NULL) {
        FEATURE_LOG_ERROR("%s::%s() pm is NULL\n", file_tag, __FUNCTION__);
        return NULL;
    }
    return pm->clearAppCache(packageName);
}

void system_internal_package_wrap_installPackage(FeatureInstanceHandle feature,
    AppendData append_data,
    system_internal_package_InstallInfo* info)
{
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetObjectData(feature));
    if (pm == NULL) {
        FEATURE_LOG_ERROR("%s::%s() pm is NULL\n", file_tag, __FUNCTION__);
        return;
    }
    InstallParam installParam;
    installParam.force = info->isForce;
    installParam.path = info->path;
    sp<FeatureInstallListener> listener = new FeatureInstallListener(feature, info->progress, info->result);
    int status = pm->installPackage(installParam, listener);
    if (status) {
        FEATURE_LOG_ERROR("%s::%s() installPackage failed, status = %d\n", file_tag, __FUNCTION__,
            status);
    }
}

void system_internal_package_wrap_uninstallPackage(FeatureInstanceHandle feature,
    AppendData append_data,
    system_internal_package_UninstallInfo* info)
{
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetObjectData(feature));
    if (pm == NULL) {
        FEATURE_LOG_ERROR("%s::%s() pm is NULL\n", file_tag, __FUNCTION__);
        return;
    }
    UninstallParam uninstallparam;
    uninstallparam.packageName = info->packageName;
    uninstallparam.clearCache = info->isClearCache;
    sp<FeatureUninstallListener> listener = new FeatureUninstallListener(feature, info->result);
    int status = pm->uninstallPackage(uninstallparam, listener);
    if (status) {
        FEATURE_LOG_ERROR("%s::%s() uninstallPackage failed, status = %d\n", file_tag, __FUNCTION__,
            status);
    }
}
