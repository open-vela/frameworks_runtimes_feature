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
static const char *file_tag = "[jidl_feature] Device_impl";

#define STRCPY(dst, src)                                                       \
  char *dst = (char *)FeatureMalloc(strlen(src) + 1, FT_CHAR);                 \
  strcpy(dst, src);

void device_onRegister(const char *feature_name) {
  FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void device_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void device_onRequired(FeatureRuntimeContext ctx,
                       FeatureInstanceHandle handle) {
  FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void device_onDetached(FeatureRuntimeContext ctx,
                       FeatureInstanceHandle handle) {
  FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void device_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void device_onUnregister(const char *feature_name) {
  FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

device_Device *device_wrap_getInfo(FeatureInstanceHandle feature, AppendData data) {
  uv_devinfo_t devinfo;
  char *screenShape = NULL;
  int platformVersionCode = 0;
  device_Device *device = deviceMallocDevice();
  const char *devicetypeMap[] = {"unknow", "watch", "band", "smartspeaker"};

  memset(&devinfo, 0, sizeof(devinfo));
  uv_getdeviceinfo(&devinfo);

  STRCPY(brand, devinfo.brand);
  STRCPY(did, devinfo.did);
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
  STRCPY(platformVersionName, "undefined");
#endif
  STRCPY(devicetype, devicetypeMap[devinfo.devicetype]);

  if (devinfo.screenshape == UV_EXT_SCREENSHAPE_ROUND) {
    STRCPY(screenshape, "circle");
    screenShape = screenshape;
  } else if (devinfo.screenshape == UV_EXT_SCREENSHAPE_SQUARE) {
    STRCPY(screenshape, "rect");
    screenShape = screenshape;
  } else {
    STRCPY(screenshape, "undefined");
    screenShape = screenshape;
  }
  device->_brand = brand;
  device->_IMEI = did;
  device->_manufacturer = manufacturer;
  device->_model = model;
  device->_product = product;
  device->_osType = ostype;
  device->_osVersionName = osversionname;
  device->_osVersionCode = devinfo.osversioncode;
  device->_platformVersionName = platformVersionName;
  device->_platformVersionCode = platformVersionCode;
  device->_language = language;
  device->_region = region;
  device->_screenWidth = devinfo.screenwidth;
  device->_screenHeight = devinfo.screenheight;
  device->_deviceType = devicetype;
  device->_screenShape = screenShape;
  return device;
}

FtString device_wrap_getDeviceid(FeatureInstanceHandle feature, AppendData data) {
  char did[32 + 1] = {0};

  int ret = uv_devinfobuff(did, sizeof(did), UV_EXT_DEVINFO_DID);
  if (0 == ret) {
    STRCPY(res, did);
    return res;
  } else {
    FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
    return NULL;
  }
}

FtString device_wrap_getid(FeatureInstanceHandle feature, AppendData data) {
  char id[32 + 1] = {0};
  /*暂时拿不到id数据，因此用did替代*/
  int ret = uv_devinfobuff(id, sizeof(id), UV_EXT_DEVINFO_DID);
  if (0 == ret) {
    STRCPY(res, id);
    return res;
  } else {
    FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
    return NULL;
  }
}

FtString device_wrap_getserial(FeatureInstanceHandle feature, AppendData data) {
  char serial[32 + 1] = {0};
  /*暂时拿不到id数据，因此用did替代*/
  int ret = uv_devinfobuff(serial, sizeof(serial), UV_EXT_DEVINFO_DID);
  if (0 == ret) {
    STRCPY(res, serial);
    return res;
  } else {
    FEATURE_LOG_ERROR("could not get devinfo id with uv_devinfobuff\n");
    return NULL;
  }
}

FtString device_wrap_gettotalstorage(FeatureInstanceHandle feature, AppendData data) {
  char totalstorage[32 + 1] = {0};
  struct statfs fs_buf;

  int ret = statfs(USERSPACE_PATH, &fs_buf);
  if (0 == ret) {
    /*每个block里面包含的字节数*/
    unsigned long long blocksize = fs_buf.f_bsize;
    /*总的字节数*/
    unsigned long long totalsize = blocksize * fs_buf.f_blocks;

    snprintf(totalstorage, sizeof(totalstorage), "%lld", totalsize);
    STRCPY(res, totalstorage);
    return res;
  } else {
    FEATURE_LOG_ERROR("could not get availablestorage with statfs\n");
    return NULL;
  }
}

FtString device_wrap_getavailablestorage(FeatureInstanceHandle feature, AppendData data) {
  char availablestorage[32 + 1] = {0};
  struct statfs fs_buf;
  int ret = statfs(USERSPACE_PATH, &fs_buf);

  if (0 == ret) {
    unsigned long long blocksize = fs_buf.f_bsize;
    unsigned long long availsize = blocksize * fs_buf.f_bavail;

    snprintf(availablestorage, sizeof(availablestorage), "%lld", availsize);
    STRCPY(res, availablestorage);
    return res;
  } else {
    FEATURE_LOG_ERROR("could not get availablestorage with statfs\n");
    return NULL;
  }
}