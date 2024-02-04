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

#include "device.h"
#include "feature_log.h"
#include "uv_ext.h"
#include <sys/statfs.h>
#ifdef CONFIG_QUICKAPP
#include "ajs_utils.h"
#include "ajs_version.h"
#endif
#define USERSPACE_PATH "/data"
static const char* file_tag = "[jidl_feature] Device_impl";

#define STRCPY(dst, src)                                        \
    char* dst = (char*)FeatureMalloc(strlen(src) + 1, FT_CHAR); \
    strcpy(dst, src);

void system_device_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_device_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_device_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_device_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_device_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_device_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

static void finish_callback(int status, FeatureInstanceHandle feature, system_device_CallBack* cb, const char* msg,
    system_device_Device* device)
{
    FtCallbackId success_id, fail_id, complete_id;

    if (cb == NULL) {
        return;
    }
    success_id = cb->success;
    fail_id = cb->fail;
    complete_id = cb->complete;

    if (status == 0) {
        FeatureInvokeCallback(feature, success_id, device);
    } else {
        FeatureInvokeCallback(feature, fail_id, msg, status);
    }
    FeatureInvokeCallback(feature, complete_id);
    FeatureRemoveCallback(feature, success_id);
    FeatureRemoveCallback(feature, fail_id);
    FeatureRemoveCallback(feature, complete_id);
}

system_device_Device* system_device_wrap_getInfo(FeatureInstanceHandle feature, AppendData append_data, system_device_CallBack* cb)
{
    uv_devinfo_t devinfo;
    char* screenShape = NULL;
    int platformVersionCode = 0;
    char serial[32 + 1] = { 0 };
    char totalstorage[32 + 1] = { 0 };
    char availablestorage[32 + 1] = { 0 };
    struct statfs fs_buf;
    int ret;
    system_device_Device* device;
    const char* devicetypeMap[] = { "unknown", "watch", "band", "smartspeaker" };

    device = system_deviceMallocDevice();
    ret = uv_devinfobuff(serial, sizeof(serial), UV_EXT_DEVINFO_DID);
    if (ret != 0) {
        FEATURE_LOG_ERROR("could not get serial\n");
    }

    ret = statfs(USERSPACE_PATH, &fs_buf);
    if (0 != ret) {
        FEATURE_LOG_ERROR("could not get availablestorage\n");
    } else {
        unsigned long long blocksize = fs_buf.f_bsize;
        unsigned long long totalsize = blocksize * fs_buf.f_blocks;
        unsigned long long availsize = blocksize * fs_buf.f_bavail;

        snprintf(totalstorage, sizeof(totalstorage), "%lld", totalsize);
        snprintf(availablestorage, sizeof(availablestorage), "%lld", availsize);
    }

    memset(&devinfo, 0, sizeof(devinfo));
    ret = uv_getdeviceinfo(&devinfo);
    if (ret != 0) {
        finish_callback(ret, feature, cb, "get device info failed", device);
        return NULL;
    }

    STRCPY(deviceserial, serial);
    STRCPY(totalspace, totalstorage);
    STRCPY(availablespace, availablestorage);
    STRCPY(brand, devinfo.brand);
    STRCPY(did, devinfo.did);
    STRCPY(deviceid, devinfo.did);
    STRCPY(manufacturer, devinfo.manufacturer);
    STRCPY(model, devinfo.model);
    STRCPY(product, devinfo.product);
    STRCPY(ostype, devinfo.ostype);
    STRCPY(osversionname, devinfo.osversionname);
    STRCPY(language, devinfo.language);
    STRCPY(region, devinfo.region);
#ifdef CONFIG_QUICKAPP
    STRCPY(platformVersionName, AIOTJS::version());
    platformVersionCode = AIOTJS::versionCode();
#else
    STRCPY(platformVersionName, "unknown");
#endif
    STRCPY(devicetype, devicetypeMap[devinfo.devicetype]);

    if (devinfo.screenshape == UV_EXT_SCREENSHAPE_ROUND) {
        STRCPY(screenshape, "circle");
        screenShape = screenshape;
    } else if (devinfo.screenshape == UV_EXT_SCREENSHAPE_SQUARE) {
        STRCPY(screenshape, "rect");
        screenShape = screenshape;
    } else {
        STRCPY(screenshape, "unknown");
        screenShape = screenshape;
    }
    device->serial = deviceserial;
    device->totalStorage = totalspace;
    device->availableStorage = availablespace;
    device->brand = brand;
    device->IMEI = did;
    device->deviceId = deviceid;
    device->manufacturer = manufacturer;
    device->model = model;
    device->product = product;
    device->osType = ostype;
    device->osVersionName = osversionname;
    device->osVersionCode = devinfo.osversioncode;
    device->platformVersionName = platformVersionName;
    device->platformVersionCode = platformVersionCode;
    device->language = language;
    device->region = region;
    device->screenWidth = devinfo.screenwidth;
    device->screenHeight = devinfo.screenheight;
    device->deviceType = devicetype;
    device->screenShape = screenShape;
    finish_callback(ret, feature, cb, "get device info successfully", device);
    return device;
}

FtString system_device_wrap_getDeviceId(FeatureInstanceHandle feature, AppendData append_data, system_device_CallBack* cb)
{
    char did[32 + 1] = { 0 };
    system_device_Device* device = system_device_wrap_getInfo(feature, append_data, NULL);
    int ret = uv_devinfobuff(did, sizeof(did), UV_EXT_DEVINFO_DID);
    if (0 == ret && device) {
        finish_callback(ret, feature, cb, "getDeviceid successfully", device);
        return device->deviceId;
    } else {
        FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
        finish_callback(ret, feature, cb, "getDeviceid failed", NULL);
        return NULL;
    }
}

FtString system_device_wrap_getId(FeatureInstanceHandle feature, AppendData append_data, system_device_CallBack* cb)
{
    char did[32 + 1] = { 0 };
    system_device_Device* device = system_device_wrap_getInfo(feature, append_data, NULL);
    int ret = uv_devinfobuff(did, sizeof(did), UV_EXT_DEVINFO_DID);
    if (0 == ret && device) {
        finish_callback(ret, feature, cb, "getId successfully", device);
        return device->deviceId;
    } else {
        FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
        finish_callback(ret, feature, cb, "getId failed", NULL);
        return NULL;
    }
}

FtString system_device_wrap_getSerial(FeatureInstanceHandle feature, AppendData append_data, system_device_CallBack* cb)
{
    char serial[32 + 1] = { 0 };
    /*暂时拿不到id数据，因此用did替代*/
    int ret = uv_devinfobuff(serial, sizeof(serial), UV_EXT_DEVINFO_DID);
    system_device_Device* device = system_device_wrap_getInfo(feature, append_data, NULL);

    if (0 == ret && device) {
        finish_callback(ret, feature, cb, "getSerial successfully", device);
        return device->serial;
    } else {
        FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
        finish_callback(ret, feature, cb, "getSerial failed", NULL);
        return NULL;
    }
}

FtString system_device_wrap_getTotalStorage(FeatureInstanceHandle feature, AppendData append_data, system_device_CallBack* cb)
{
    struct statfs fs_buf;
    int ret = statfs(USERSPACE_PATH, &fs_buf);
    system_device_Device* device = system_device_wrap_getInfo(feature, append_data, NULL);

    if (0 == ret && device) {
        finish_callback(ret, feature, cb, "getTotalStorage successfully", device);
        return device->totalStorage;
    } else {
        FEATURE_LOG_ERROR("could not get availablestorage with statfs\n");
        finish_callback(ret, feature, cb, "getTotalStorage failed", NULL);
        return NULL;
    }
}

FtString system_device_wrap_getAvailableStorage(FeatureInstanceHandle feature, AppendData append_data, system_device_CallBack* cb)
{
    struct statfs fs_buf;
    int ret = statfs(USERSPACE_PATH, &fs_buf);
    system_device_Device* device = system_device_wrap_getInfo(feature, append_data, NULL);

    if (0 == ret && device) {
        finish_callback(ret, feature, cb, "getAvailableStorage successfully", device);
        return device->availableStorage;
    } else {
        FEATURE_LOG_ERROR("could not get availablestorage with statfs\n");
        finish_callback(ret, feature, cb, "getAvailableStorage failed", NULL);
        return NULL;
    }
}