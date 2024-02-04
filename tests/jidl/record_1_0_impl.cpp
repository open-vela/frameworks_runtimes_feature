// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "record_1_0.h"

const char* file_tag = "[jidl_feature] record_1_0_impl";

// FeatureCallbacks to be implemented
void Record_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Record_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Record_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Record_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Record_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Record_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Record_wrap_start(FeatureInstanceHandle feature,
    AppendData data,
    long duration,
    FtInt sampleRate,
    FtInt numberOfChannels,
    FtInt encodeBitRate,
    FtString format,
    FtCallbackId s_cb,
    FtCallbackId f_cb,
    FtCallbackId c_cb)
{
    printf("%s::%s(), duration: %ld, sampleRate: %d, numberOfChannels: %d, encodeBitRate: %d, format: %s\n",
        file_tag, __FUNCTION__, duration, sampleRate, numberOfChannels, encodeBitRate, format);
}

void Record_wrap_stop(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}
