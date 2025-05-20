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
#include <kvdb.h>

#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"

#define SYSTEMDEBUG_LIFECYCLE_DEBUG() \
    FEATURE_LOG_DEBUG("[jidl_feature] debug_impl:: %s()", __FUNCTION__)

#define APPDEBUGKEY "persist.quickapp.debug"

const char* const DEFAULT_CODE = "0";

void system_debug_onRegister(const char* feature_name)
{
    SYSTEMDEBUG_LIFECYCLE_DEBUG();
}
void system_debug_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    SYSTEMDEBUG_LIFECYCLE_DEBUG();
    FEATURE_LOG_INFO("[jump native] system_debug_onCreate");
}
void system_debug_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    SYSTEMDEBUG_LIFECYCLE_DEBUG();
}
void system_debug_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    SYSTEMDEBUG_LIFECYCLE_DEBUG();
}
void system_debug_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    SYSTEMDEBUG_LIFECYCLE_DEBUG();
}
void system_debug_onUnregister(const char* feature_name)
{
    SYSTEMDEBUG_LIFECYCLE_DEBUG();
}

void system_debug_wrap_enable(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid)
{
    FEATURE_LOG_INFO("[jump native] system_debug_wrap_enable");
    int enable = 1;
    int ret = property_set_int32(APPDEBUGKEY, enable);
    if (ret != 0) {
        FEATURE_LOG_ERROR("[jump native] system_debug_wrap_enable property_set_int error");
        FeaturePromiseReject(feature, pid, ret, DEFAULT_CODE);
        return;
    }
    FeaturePromiseResolve(feature, pid, enable);
}

void system_debug_wrap_disable(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid)
{
    FEATURE_LOG_INFO("[jump native] system_debug_wrap_disable");
    int disable = 0;
    int ret = property_set_int32(APPDEBUGKEY, disable);
    if (ret != 0) {
        FEATURE_LOG_ERROR("[jump native] system_debug_wrap_disable property_set_int error");
        FeaturePromiseReject(feature, pid, ret, DEFAULT_CODE);
        return;
    }
    FeaturePromiseResolve(feature, pid, disable);
}

void system_debug_wrap_getStatus(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid)
{
    FEATURE_LOG_INFO("[jump native] system_debug_wrap_getStatus");
    int ret = property_get_int32(APPDEBUGKEY, -1);
    if (ret == -1) {
        FEATURE_LOG_ERROR("[jump native] system_debug_wrap_getStatus error");
        FeaturePromiseReject(feature, pid, ret, DEFAULT_CODE);
        return;
    }
    FeaturePromiseResolve(feature, pid, ret);
}
