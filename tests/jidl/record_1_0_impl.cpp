// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "record_1_0.h"

const char* file_tag = "[jidl_feature] record_1_0_impl";

// FeatureCallbacks to be implemented
void Record_1_0_onRegister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Record_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Record_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Record_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Record_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Record_1_0_onUnregister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Record_1_0_wrap_start(FeatureInstanceHandle feature,
                   AppendData data,
                   long duration,
                   FtInt sampleRate,
                   FtInt numberOfChannels,
                   FtInt encodeBitRate,
                   FtString format,
                   FeatureCallbackId s_cb,
                   FeatureCallbackId f_cb,
                   FeatureCallbackId c_cb) {
    printf("%s::%s(), duration: %ld, sampleRate: %d, numberOfChannels: %d, encodeBitRate: %d, format: %s\n",
          file_tag,  __FUNCTION__, duration, sampleRate, numberOfChannels, encodeBitRate, format);
}

void Record_1_0_wrap_stop(FeatureInstanceHandle feature, AppendData data) {
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}
