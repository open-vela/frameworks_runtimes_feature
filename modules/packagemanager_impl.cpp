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

#include "packagemanager.h"

#include <binder/ProcessState.h>
#include <sstream>
#include <utils/Errors.h>

#include <tuple>
#include <unordered_map>

#include "app_path.h"
#include "pm/PackageManager.h"

static const char* kFileTag = "[jidl_feature] packagemanager_impl";

using namespace ::os::pm;
using android::binder::Status;

namespace {

enum class AppType {
    kAppUnkonwn = 0,
    kAppSystem,
    kAppNormal
};

AppType GetAppType(const std::string& path)
{
    if (path.compare(0, strlen(CONFIG_HAP_APP_PATH), CONFIG_HAP_APP_PATH) == 0) {
        return AppType::kAppNormal;
    } else if (path.compare(0, strlen("/resource"), "/resource") == 0) {
        return AppType::kAppSystem;
    }
    return AppType::kAppUnkonwn;
}

void NativePackageInfoToJSPackageInfo(PackageInfo& info,
    system_packagemanager_PackageInfo* jsInfo, ft_context_ref ftCtx)
{
    if (jsInfo == NULL) {
        return;
    }
    jsInfo->package = info.packageName.c_str();
    jsInfo->appName = info.name.c_str();
    jsInfo->appIcon = info.icon.c_str();
    jsInfo->versionCode = info.extra.has_value() ? info.extra->versionCode : 1;
    jsInfo->versionName = info.version.c_str();
    jsInfo->minAPILevel = 1;
    jsInfo->permissions = NULL;
    jsInfo->appSize = info.size;
    jsInfo->isSystem = GetAppType(info.installedPath) == AppType::kAppSystem;
    ft_value_t* temp = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *temp = ft_new_object(ftCtx);
    jsInfo->extra = temp;
}

}

void system_packagemanager_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", kFileTag, __FUNCTION__);
}

void system_packagemanager_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", kFileTag, __FUNCTION__);
    PackageManager* pm = new PackageManager();
    FeatureSetProtoData(handle, pm);
}

void system_packagemanager_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", kFileTag, __FUNCTION__);
}

void system_packagemanager_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", kFileTag, __FUNCTION__);
}

void system_packagemanager_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", kFileTag, __FUNCTION__);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(handle));
    if (pm) {
        delete (pm);
        pm = NULL;
    }
}

void system_packagemanager_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", kFileTag, __FUNCTION__);
}

struct PmResultPostData {
    FeatureInstanceHandle handle;
    FtPromiseId pid;
    std::string packageName;
    std::string operation;
    int32_t code;
    std::string msg;
    int promiseCode;
    int resultCode;
};

static void HandlePmResultOnJsThread(int status, void* user_data)
{
    PmResultPostData* data = static_cast<PmResultPostData*>(user_data);
    if (!data)
        return;

    std::unique_ptr<PmResultPostData> guard(data);
    if (status == FEATURE_TASK_MODE_FREE) {
        FEATURE_LOG_WARN("%s::HandlePmResultOnJsThread: FeaturePost free mode", kFileTag);
        FeatureFreeInstanceHandle(data->handle);
        return;
    }

    if (FeatureInstanceIsDetached(data->handle)) {
        FEATURE_LOG_ERROR("%s::FeatureInstanceHandle is detached !", kFileTag);
        FeatureFreeInstanceHandle(data->handle);
        return;
    }

    if (data->code == 0) {
        system_packagemanager_SucessObj sucessObj;
        sucessObj.result = true;
        FeaturePromiseResolve(data->handle, data->pid, &sucessObj);
    } else {
        FEATURE_LOG_ERROR("%s::reject promise, msg: %s", kFileTag, data->msg.c_str());
        FeaturePromiseReject(data->handle, data->pid, data->promiseCode, data->msg.c_str());
    }

    system_packagemanager_StatusChangeEvent result { 0 };
    result.package = data->packageName.c_str();
    result.operation = data->operation.c_str();
    result.code = data->resultCode;
    if (data->code == 0) {
        result.status = "success";
        result.msg = "";
    } else {
        result.status = "fail";
        result.msg = data->msg.c_str();
    }
    FeatureEmitEventByName(data->handle, "onstatuschange", &result);

    FeatureFreeInstanceHandle(data->handle);
}

class FeatureInstallListener : public BnInstallObserver {
public:
    FeatureInstallListener(FeatureInstanceHandle handle, FtPromiseId pid)
        : handle_(handle)
        , pid_(pid)
    {
        FeatureDupInstanceHandle(handle_);
    }
    ~FeatureInstallListener() { FeatureFreeInstanceHandle(handle_); }
    Status onInstallProcess(const std::string& packageName, int32_t process) override
    {
        FEATURE_LOG_INFO("%s::onInstallProcess: %s(%" PRIi32 ")\n", kFileTag, packageName.c_str(), process);
        return Status::ok();
    }

    Status onInstallResult(const std::string& packageName, int32_t code,
        const std::string& msg) override
    {
        FEATURE_LOG_INFO("%s::onInstallResult: %s(%s %" PRIi32 ")\n", kFileTag, packageName.c_str(), msg.c_str(), code);

        auto* postData = new PmResultPostData();
        postData->handle = handle_;
        FeatureDupInstanceHandle(postData->handle);
        postData->pid = pid_;
        postData->packageName = packageName;
        postData->operation = "install";
        postData->code = code;
        postData->msg = msg;

        if (code != 0) {
            const std::unordered_map<int32_t, std::tuple<int, int>> errorMap = {
                { android::NAME_NOT_FOUND, { 1000, 202 } },
                { android::ALREADY_EXISTS, { 1001, 202 } },
                { android::NOT_ENOUGH_DATA, { 1002, 4102 } },
                { android::NO_MEMORY, { 200, 5101 } }
            };
            auto [promiseCode, resultCode] = errorMap.count(code) ? errorMap.at(code) : std::make_tuple(202, 202);
            postData->promiseCode = promiseCode;
            postData->resultCode = resultCode;
        } else {
            postData->promiseCode = 0;
            postData->resultCode = 0;
        }

        if (!FeaturePost(handle_, HandlePmResultOnJsThread, postData)) {
            FEATURE_LOG_ERROR("%s::FeaturePost failed for install result!", kFileTag);
            FeatureFreeInstanceHandle(postData->handle);
            delete postData;
        }
        return Status::ok();
    }

private:
    FeatureInstanceHandle handle_;
    FtPromiseId pid_;
};

class FeatureUninstallListener : public BnUninstallObserver {
public:
    FeatureUninstallListener(FeatureInstanceHandle handle, FtPromiseId pid)
        : handle_(handle)
        , pid_(pid)
    {
        FeatureDupInstanceHandle(handle_);
    }
    ~FeatureUninstallListener() { FeatureFreeInstanceHandle(handle_); }
    Status onUninstallResult(const std::string& packageName, int32_t code,
        const std::string& msg) override
    {
        FEATURE_LOG_INFO("%s::onUninstallResult: %s(%s %" PRIi32 ")\n", kFileTag, packageName.c_str(), msg.c_str(), code);

        auto* postData = new PmResultPostData();
        postData->handle = handle_;
        FeatureDupInstanceHandle(postData->handle);
        postData->pid = pid_;
        postData->packageName = packageName;
        postData->operation = "uninstall";
        postData->code = code;
        postData->msg = msg;

        if (code != 0) {
            const std::unordered_map<int32_t, std::tuple<int, int>> errorMap = {
                { android::NAME_NOT_FOUND, { 1000, 202 } },
                { android::ALREADY_EXISTS, { 1001, 202 } }
            };
            auto [promiseCode, resultCode] = errorMap.count(code) ? errorMap.at(code) : std::make_tuple(202, 202);
            postData->promiseCode = promiseCode;
            postData->resultCode = resultCode;
        } else {
            postData->promiseCode = 0;
            postData->resultCode = 0;
        }

        if (!FeaturePost(handle_, HandlePmResultOnJsThread, postData)) {
            FEATURE_LOG_ERROR("%s::FeaturePost failed for uninstall result!", kFileTag);
            FeatureFreeInstanceHandle(postData->handle);
            delete postData;
        }
        return Status::ok();
    }

private:
    FeatureInstanceHandle handle_;
    FtPromiseId pid_;
};

void system_packagemanager_wrap_hasInstalled(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_packagemanager_HasInstalledOptions* options)
{
    if (options == NULL || options->package == NULL || options->type == NULL) {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    if (std::string(options->type) != "package" && std::string(options->type) != "widget") {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(protoHandle));
    PackageInfo info;
    int ret = pm->getPackageInfo(options->package, &info);
    system_packagemanager_SucessObj jsObj = system_packagemanager_SucessObj { 0 };
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::getPackageInfo failed\n", kFileTag);
        jsObj.result = false;
        FeaturePromiseResolve(feature, pid, &jsObj);
        return;
    }
    jsObj.result = true;
    FeaturePromiseResolve(feature, pid, &jsObj);
    return;
}

void system_packagemanager_wrap_install(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_packagemanager_InstallOptions* options)
{
    if (options == NULL || options->uri == NULL) {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    size_t pos = std::string(options->uri).find_last_of('.');
    std::string suffix;
    if (pos != std::string::npos) {
        suffix = std::string(options->uri).substr(pos + 1);
        if (suffix != "rpk" && suffix != "apk") {
            FeaturePromiseReject(feature, pid, 202, "parameter error");
            return;
        }
    }

    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    char* str = app_relative_to_absolute_path(FeatureGetPackageName(protoHandle), const_cast<char*>(options->uri));
    if (!str) {
        FEATURE_LOG_ERROR("%s::app_relative_to_absolute_path failed\n", kFileTag);
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }

    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(protoHandle));

    std::string packagePath = str;
    free(str);

    InstallParam installParam;
    installParam.force = options->force;
    installParam.path = packagePath;
    sp<FeatureInstallListener> listener = new FeatureInstallListener(feature, pid);
    pm->installPackage(installParam, listener);
}

void system_packagemanager_wrap_uninstall(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_packagemanager_UnInstalledOptions* options)
{
    if (options == NULL || options->package == NULL) {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(protoHandle));

    UninstallParam uninstallparam;
    uninstallparam.packageName = options->package;
    sp<FeatureUninstallListener> listener = new FeatureUninstallListener(feature, pid);
    pm->uninstallPackage(uninstallparam, listener);
}

void system_packagemanager_wrap_getPackageInfo(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_packagemanager_GetPackageInfoOptions* options)
{
    if (options == NULL || options->package == NULL) {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(protoHandle));
    PackageInfo info;
    int ret = pm->getPackageInfo(options->package, &info);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::getPackageInfo failed\n", kFileTag);
        FeaturePromiseReject(feature, pid, 1000, "application package does not exist");
        return;
    }
    system_packagemanager_PackageInfo jsInfo = system_packagemanager_PackageInfo { 0 };
    ft_context_ref ftCtx = FeatureGetContext(feature);
    NativePackageInfoToJSPackageInfo(info, &jsInfo, ftCtx);
    FeaturePromiseResolve(feature, pid, &jsInfo);
    ft_free_value(ftCtx, *(jsInfo.extra));
    FeatureFreeValue(jsInfo.extra);
}

void system_packagemanager_wrap_getInstalledPackages(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_packagemanager_nullPara* para)
{
    if (para == NULL || para->placeHolder != NULL) {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(protoHandle));

    std::vector<PackageInfo> pkgInfos;
    const int kSliceSize = 20;
    int status = pm->getAllPackageInfoEx(&pkgInfos, kSliceSize);
    if (status) {
        FEATURE_LOG_ERROR("%s::getAllPackageInfoEx failed, status:%d\n", kFileTag, status);
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }

    FtArray packageArray;
    packageArray._size = pkgInfos.size();
    std::vector<system_packagemanager_PackageInfo> jsPkgsStorage(pkgInfos.size());
    std::vector<system_packagemanager_PackageInfo*> elements(pkgInfos.size());
    ft_context_ref ftCtx = FeatureGetContext(feature);
    for (size_t i = 0; i < pkgInfos.size(); i++) {
        NativePackageInfoToJSPackageInfo(pkgInfos[i], &jsPkgsStorage[i], ftCtx);
        elements[i] = &jsPkgsStorage[i];
    }
    packageArray._element = elements.data();
    FeaturePromiseResolve(feature, pid, &packageArray);
    for (size_t i = 0; i < pkgInfos.size(); i++) {
        ft_free_value(ftCtx, *(jsPkgsStorage[i].extra));
        FeatureFreeValue(jsPkgsStorage[i].extra);
    }
}

void system_packagemanager_wrap_getPackagesInOperation(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_packagemanager_nullPara* para)
{
    if (para == NULL || para->placeHolder != NULL) {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(protoHandle));

    std::vector<PackageInOperation> statusInfo;
    int status = pm->getPackagesInOperation(&statusInfo);
    if (status) {
        FEATURE_LOG_ERROR("%s::getAllPackageInfo failed, status:%d\n", kFileTag, status);
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }

    std::vector<system_packagemanager_PackageItem> jsPkgsStorage;
    std::vector<system_packagemanager_PackageItem*> elements;
    jsPkgsStorage.reserve(statusInfo.size());
    elements.reserve(statusInfo.size());

    for (size_t i = 0; i < statusInfo.size(); i++) {
        if (statusInfo[i].operationStatus == static_cast<int>(os::pm::PackageOperationStatus::INSTALLING)) {
            jsPkgsStorage.emplace_back();
            auto& item = jsPkgsStorage.back();
            item.package = statusInfo[i].packageName.c_str();
            item.operation = "install";
            elements.push_back(&item);
        } else if (statusInfo[i].operationStatus == static_cast<int>(os::pm::PackageOperationStatus::UNINSTALLING)) {
            jsPkgsStorage.emplace_back();
            auto& item = jsPkgsStorage.back();
            item.package = statusInfo[i].packageName.c_str();
            item.operation = "uninstall";
            elements.push_back(&item);
        }
    }

    FtArray packageArray;
    packageArray._size = elements.size();
    packageArray._element = elements.data();
    FeaturePromiseResolve(feature, pid, &packageArray);
}

void system_packagemanager_wrap_clearPackageData(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_packagemanager_ClearPackageDataOptions* options)
{
    if (options == NULL || options->package == NULL) {
        FeaturePromiseReject(feature, pid, 202, "parameter error");
        return;
    }
    FeatureProtoHandle protoHandle = FeatureGetProtoHandle(feature);
    PackageManager* pm = static_cast<PackageManager*>(FeatureGetProtoData(protoHandle));

    int result = pm->clearAppCache(options->package);
    if (result != 0) {
        FEATURE_LOG_ERROR("%s::clearAppCache failed\n", kFileTag);
        FeaturePromiseReject(feature, pid, 1000, "application package does not exist");
        return;
    }
    system_packagemanager_SucessObj jsObj = system_packagemanager_SucessObj { 0 };
    jsObj.result = true;
    FeaturePromiseResolve(feature, pid, &jsObj);

    return;
}
