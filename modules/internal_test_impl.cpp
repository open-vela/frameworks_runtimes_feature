/*
 * Copyright (C) 2023 Xiaomi Corporation. All rights reserved.
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
 *
 */

#include "feature_log.h"
#include "feature_utils.h"
#include "internal_test.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] Internal_test_impl";

void system_internal_test_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_internal_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_internal_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_internal_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_internal_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_internal_test_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

static void finish_callback(int status, FeatureInstanceHandle feature, system_internal_test_get_extra_info_params* params, const char* msg,
    system_internal_test_ExtraDeviceInfo* extra_info)
{
    FtCallbackId success_id, fail_id, complete_id;

    if (params == NULL) {
        return;
    }
    success_id = params->success;
    fail_id = params->fail;
    complete_id = params->complete;

    if (status == 0 && success_id != 0) {
        FeatureInvokeCallback(feature, success_id, extra_info);
    } else if (fail_id != 0) {
        FeatureInvokeCallback(feature, fail_id, msg, status);
    }

    if (complete_id != 0) {
        FeatureInvokeCallback(feature, complete_id);
    }
    FeatureRemoveCallback(feature, success_id);
    FeatureRemoveCallback(feature, fail_id);
    FeatureRemoveCallback(feature, complete_id);
}

void system_internal_test_wrap_getExtraDeviceInfo(FeatureInstanceHandle feature, AppendData append_data,
    system_internal_test_get_extra_info_params* parm)
{
    uv_devinfo_t devinfo;
    int status;
    system_internal_test_ExtraDeviceInfo* extra_info;

    extra_info = system_internal_testMallocExtraDeviceInfo();
    memset(&devinfo, 0, sizeof(devinfo));
    status = uv_getdeviceinfo(&devinfo);
    if (status != 0) {
        finish_callback(status, feature, parm, "get device info failed", NULL);
        FeatureFreeValue(extra_info);
        return;
    }

    extra_info->bpp = devinfo.bpp;
    finish_callback(status, feature, parm, "get device info successfully", extra_info);
    FeatureFreeValue(extra_info);
}