#include "ATest_1_0.h"

const char* file_tag1 = "[jidl_feature] ATest_1_0_impl";

// FeatureCallbacks to be implemented
void ATest_1_0_onRegister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag1,  __FUNCTION__);
}

void ATest_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag1,  __FUNCTION__);
}

void ATest_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag1,  __FUNCTION__);
}

void ATest_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag1,  __FUNCTION__);
}

void ATest_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag1,  __FUNCTION__);
}

void ATest_1_0_onUnregister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag1,  __FUNCTION__);
}

FtString ATest_1_0_wrap_test1(FeatureInstanceHandle feature, AppendData data, FtString a, FtInt b)
{
    printf("ATest_1_0_wrap_test1: %s, %d\n", a, b);
    const char *res = "hello, world!";
    return res;
}