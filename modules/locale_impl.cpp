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

#include "feature_locale.h"
#include "feature_log.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] locale_impl";

// FeatureCallbacks to be implemented
void locale_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void locale_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void locale_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void locale_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void locale_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void locale_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

locale_Locale* locale_wrap_get(FeatureInstanceHandle feature, AppendData data)
{
    uv_locale_t uvlocale = {};
    locale_Locale* locale = localeMallocLocale();
    int ret = uv_getlocale(&uvlocale);
    if (ret <= 0) {
        FEATURE_LOG_ERROR("%s::%s getlocale failed\n", file_tag, __FUNCTION__);
    }
    char* language = (char*)FeatureMalloc(strlen(uvlocale.language) + 1, FT_STRING);
    sprintf(language, "%s", uvlocale.language);
    char* region = (char*)FeatureMalloc(strlen(uvlocale.country_region) + 1, FT_STRING);
    sprintf(region, "%s", uvlocale.country_region);
    locale->language = language;
    locale->countryOrRegion = region;
    return locale;
}
