// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "callback_test.h"

static const char* file_tag = "[jidl_feature] callback_test_impl";

// FeatureCallbacks to be implemented
void callback_test_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void callback_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void callback_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void callback_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void callback_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void callback_test_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

// Function wrappers to be implemented
void callback_test_wrap_goo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtInt b, FtCallbackId cb)
{
    // callback cb1(int x, string y, double z)
    FEATURE_LOG_INFO("%s, a: %d, b: %d, will invoke cb1", file_tag, a, b);
    if (!callback_test_cb1_invoke(feature, cb, a, "hello", (double)b)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void callback_test_wrap_goo2(FeatureInstanceHandle feature, AppendData data, FtCallbackId cb, FtCallbackId cb3, FtCallbackId cb4)
{
    // callback cb3()
    FEATURE_LOG_INFO("%s, will invoke cb3", file_tag);
    if (!callback_test_cb3_invoke(feature, cb3)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb3);

    // callback cb4(...)
    FEATURE_LOG_INFO("%s, will invoke cb4", file_tag);
    int32_t* var1 = (int32_t*)FeatureMalloc(sizeof(int32_t), FT_INT32);
    *var1 = 15;
    char* var2 = (char*)FeatureMalloc(sizeof("hello") + 1, FT_STRING);
    sprintf(var2, "%s", "hello");
    char* var3 = (char*)FeatureMalloc(sizeof("world") + 1, FT_STRING);
    sprintf(var3, "%s", "world");
    bool ret = FeatureInvokeCallbackCount(feature, cb4, 3, var1, var2, var3);
    FeatureFreeValue(var1);
    FeatureFreeValue(var2);
    FeatureFreeValue(var3);

    if (!ret) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb4);

    // callback cb2(int a, string b, ...)
    FEATURE_LOG_INFO("%s, will invoke cb2", file_tag);
    var1 = (int32_t*)FeatureMalloc(sizeof(int32_t), FT_INT32);
    *var1 = 50;
    double* var4 = (double*)FeatureMalloc(sizeof(double), FT_DOUBLE);
    *var4 = 4.5;
    ret = FeatureInvokeCallbackCount(feature, cb, 4, 20, "hello world", var1, var4);

    FeatureFreeValue(var1);
    FeatureFreeValue(var4);
    if (!ret) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void callback_test_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FtCallbackId cb, FtCallbackId cb2)
{
    // callback cb1(int x, string y, double z)
    FEATURE_LOG_INFO("%s, x: %d, y: %f, will invoke cb1", file_tag, x, y);
    if (!callback_test_cb1_invoke(feature, cb, x, "greeting", y)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);

    // callback cb2(int a, string b, ...)
    FEATURE_LOG_INFO("%s, will invoke cb2", file_tag);
    char* strValue = (char*)FeatureMalloc(sizeof("you") + 1, FT_STRING);
    sprintf(strValue, "%s", "you");
    bool ret = FeatureInvokeCallbackCount(feature, cb2, 3, x, "love", strValue);

    FeatureFreeValue(strValue);
    if (!ret) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb2);
}

void callback_test_wrap_foo3(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FtCallbackId cb)
{
    // callback cb1(int x, string y, double z)
    FEATURE_LOG_INFO("%s, x: %d, y: %f, will invoke cb1", file_tag, x, y);
    if (!callback_test_cb1_invoke(feature, cb, x, "greeting", y)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}
