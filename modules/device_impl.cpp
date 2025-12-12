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
#include "feature_utils.h"
#include "uv_ext.h"
#include <sys/statfs.h>
#ifdef CONFIG_QUICKAPP
#include "quickapp.h"
#endif
#define USERSPACE_PATH "/data"
static const char* file_tag = "[jidl_feature] Device_impl";

#define STRCPY(dst, src)                                              \
    do {                                                              \
        char* tmp = (char*)FeatureMalloc(strlen(src) + 1, FT_STRING); \
        strcpy(tmp, src);                                             \
        dst = tmp;                                                    \
    } while (0)

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

static void finish_callback_getinfo(int status, FeatureInstanceHandle feature, system_device_getinfo_params* params, const char* msg,
    system_device_Device* device)
{
    FtCallbackId success_id, fail_id, complete_id;
    system_device_common_params* common_params = (system_device_common_params*)params;
    if (common_params == NULL) {
        return;
    }
    success_id = common_params->success;
    fail_id = common_params->fail;
    complete_id = common_params->complete;

    if (status == 0 && success_id != 0) {
        FeatureInvokeCallback(feature, success_id, device);
    } else if (fail_id != 0) {
        FeatureInvokeCallback(feature, fail_id, msg, status);
    }
    FeatureInvokeCallback(feature, complete_id);
    FeatureRemoveCallback(feature, success_id);
    FeatureRemoveCallback(feature, fail_id);
    FeatureRemoveCallback(feature, complete_id);
}

static void finish_callback_common(int status, FeatureInstanceHandle feature, system_device_common_params* params, const char* msg,
    ft_value_t* obj)
{
    FtCallbackId success_id, fail_id, complete_id;

    if (params == NULL) {
        return;
    }
    success_id = params->success;
    fail_id = params->fail;
    complete_id = params->complete;

    if (status == 0 && success_id != 0) {
        FeatureInvokeCallback(feature, success_id, obj);
    } else if (fail_id != 0) {
        FeatureInvokeCallback(feature, fail_id, msg, status);
    }
    FeatureInvokeCallback(feature, complete_id);
    FeatureRemoveCallback(feature, success_id);
    FeatureRemoveCallback(feature, fail_id);
    FeatureRemoveCallback(feature, complete_id);
}

void system_device_wrap_getInfo(FeatureInstanceHandle feature, AppendData append_data, system_device_getinfo_params* params)
{
    uv_devinfo_t devinfo;
    int status;
    system_device_Device* device;

    device = system_deviceMallocDevice();
    memset(&devinfo, 0, sizeof(devinfo));
    status = uv_getdeviceinfo(&devinfo);
    if (status != 0) {
        finish_callback_getinfo(status, feature, params, "get device info failed", NULL);
        FeatureFreeValue(device);
        return;
    }

    STRCPY(device->brand, devinfo.brand);
    STRCPY(device->IMEI, devinfo.did);
    STRCPY(device->manufacturer, devinfo.manufacturer);
    STRCPY(device->model, devinfo.model);
    STRCPY(device->product, devinfo.product);
    STRCPY(device->osType, devinfo.ostype);
    STRCPY(device->osVersionName, devinfo.osversionname);
    STRCPY(device->language, devinfo.language);
    STRCPY(device->region, devinfo.region);
#ifdef CONFIG_QUICKAPP
    STRCPY(device->platformVersionName, QAppVersion());
    device->platformVersionCode = QAppVersionCode();
    device->APILevel = QAppAPILevel();
#else
    STRCPY(device->platformVersionName, "unknown");
#endif
    STRCPY(device->deviceType, devinfo.devicetype);
    STRCPY(device->screenShape, devinfo.screenshape);

#ifdef CONFIG_MIWEAR_DEVICE_PID
    STRCPY(device->miProductId, CONFIG_MIWEAR_DEVICE_PID);
#endif

#ifdef CONFIG_DEVICE_MODEL
    STRCPY(device->deviceModel, CONFIG_DEVICE_MODEL);
#endif

#ifdef CONFIG_MIWEAR_DEVICE_ALIAS
    STRCPY(device->miDeviceAlias, CONFIG_MIWEAR_DEVICE_ALIAS);
#endif

    device->osVersionCode = devinfo.osversioncode;
    device->screenWidth = devinfo.screenwidth;
    device->screenHeight = devinfo.screenheight;
    device->screenDensity = (int)((devinfo.screendensity + 0.05) * 10) / 10.0;
    finish_callback_getinfo(status, feature, params, "get device info successfully", device);
    FeatureFreeValue(device);
}

FtString system_device_wrap_getDeviceId(FeatureInstanceHandle feature, AppendData append_data, system_device_common_params* params)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FtString ret = NULL;
    char did[32 + 1] = { 0 };

    int status = uv_devinfobuff(did, sizeof(did), UV_EXT_DEVINFO_DID);
    if (0 == status) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = ft_from_string(ft_ctx, did);
        ft_obj_set_property(ft_ctx, ret_obj, "deviceId", ret_data);
        finish_callback_common(status, feature, params, "getDeviceid successfully", &ret_obj);
        ft_free_value(ft_ctx, ret_obj);
    } else {
        FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
        finish_callback_common(status, feature, params, "getDeviceid failed", NULL);
    }
    ret = (char*)FeatureMalloc(strlen(did) + 1, FT_STRING);
    if (ret) {
        sprintf((char*)ret, "%s", did);
    }
    return ret;
}

void system_device_wrap_getId(FeatureInstanceHandle feature, AppendData append_data, system_device_common_params* params)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    char did[32 + 1] = { 0 };
    int status = uv_devinfobuff(did, sizeof(did), UV_EXT_DEVINFO_DID);

    if (0 == status) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = ft_from_string(ft_ctx, did);
        ft_obj_set_property(ft_ctx, ret_obj, "deviceId", ret_data);
        finish_callback_common(status, feature, params, "getId successfully", &ret_obj);
        ft_free_value(ft_ctx, ret_obj);
    } else {
        FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
        finish_callback_common(status, feature, params, "getId failed", NULL);
    }
}

void system_device_wrap_getSerial(FeatureInstanceHandle feature, AppendData append_data, system_device_common_params* params)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    char serial[32 + 1] = { 0 };
    int status = uv_devinfobuff(serial, sizeof(serial), UV_EXT_DEVINFO_SERIAL);

    if (0 == status) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = ft_from_string(ft_ctx, serial);
        ft_obj_set_property(ft_ctx, ret_obj, "serial", ret_data);
        finish_callback_common(status, feature, params, "getSerial successfully", &ret_obj);
        ft_free_value(ft_ctx, ret_obj);
    } else {
        FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
        finish_callback_common(status, feature, params, "getSerial failed", NULL);
    }
}

void system_device_wrap_getTotalStorage(FeatureInstanceHandle feature, AppendData append_data, system_device_common_params* params)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    struct statfs fs_buf;
    int status = statfs(USERSPACE_PATH, &fs_buf);

    if (0 == status) {
        unsigned long long blocksize = fs_buf.f_bsize;
        unsigned long long totalsize = blocksize * fs_buf.f_blocks;

        ft_value_t ret_obj = ft_new_object(ft_ctx);
        // The range of int64 is enough for the storage size
        ft_value_t ret_data = ft_from_int64(ft_ctx, totalsize);
        ft_obj_set_property(ft_ctx, ret_obj, "totalStorage", ret_data);

        finish_callback_common(status, feature, params, "getTotalStorage successfully", &ret_obj);
        ft_free_value(ft_ctx, ret_obj);
    } else {
        FEATURE_LOG_ERROR("could not get availablestorage with statfs\n");
        finish_callback_common(status, feature, params, "getTotalStorage failed", NULL);
    }
}

void system_device_wrap_getAvailableStorage(FeatureInstanceHandle feature, AppendData append_data, system_device_common_params* params)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    struct statfs fs_buf;
    int status = statfs(USERSPACE_PATH, &fs_buf);

    if (0 == status) {
        unsigned long long blocksize = fs_buf.f_bsize;
        unsigned long long availsize = blocksize * fs_buf.f_bavail;

        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = ft_from_int64(ft_ctx, availsize);
        ft_obj_set_property(ft_ctx, ret_obj, "availableStorage", ret_data);

        finish_callback_common(status, feature, params, "getAvailableStorage successfully", &ret_obj);
        ft_free_value(ft_ctx, ret_obj);
    } else {
        FEATURE_LOG_ERROR("could not get availablestorage with statfs\n");
        finish_callback_common(status, feature, params, "getAvailableStorage failed", NULL);
    }
}