// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "promise_1_0.h"


static const char* file_tag = "[jidl_feature] promise_1_0_impl";

template <typename T>
class FTArrayHelper {
private:
    FTArray* _data;

public:
    FTArrayHelper(FTArray* data)
    {
        _data = data;
    }

    ~FTArrayHelper()
    {
    }

    T& operator[](int32_t index)
    {
        return ((T*)_data->_element)[index];
    }

    int32_t size() const { return _data->_size; }
};

// FeatureCallbacks to be implemented
void Promise_1_0_onRegister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_onUnregister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

// Function wrappers to be implemented
void Promise_1_0_wrap_foo(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle, FtInt a, FtString b)
{
    printf("%s::%s(), a: %d, b: %s\n", file_tag,  __FUNCTION__, a, b);
    int rs = a;
    int rj = 10;
    if (rs != 0) {
        FeaturePromiseResolve(feature, promiseHandle, rs);
    } else {
        FeaturePromiseReject(feature, promiseHandle, rj);
    }
}

void Promise_1_0_wrap_foo1(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle, FtInt a)
{
    printf("%s::%s(), a: %d\n", file_tag,  __FUNCTION__, a);
    int rs = a;
    const char* rj = "hello";
    if (rs != 0) {
        FeaturePromiseResolve(feature, promiseHandle, rs);
    } else {
        FeaturePromiseReject(feature, promiseHandle, rj);
    }
}

void Promise_1_0_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
    int rs = 0;
    const char* rj = "world";
    if (rs != 0) {
        FeaturePromiseResolve(feature, promiseHandle, rs);
    } else {
        FeaturePromiseReject(feature, promiseHandle, rj);
    }
}

void Promise_1_0_wrap_bar(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_wrap_bar1(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Promise_1_0_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters variadicParameters)
{
    printf("[jidl_feature] ");
    auto ctx = static_cast<feature_context_ref>(GetFeatureContext(feature));
    for (int i = 0; i < variadicParameters.variadic_count; i++) {
        feature_value_t& param = *variadicParameters.variadic_args[i];
        if (feature_is_object(param)) {
            feature_value_t json_obj = feature_stringify(ctx, param);
            const char* str_json = feature_to_cstring(ctx, json_obj);
            printf("%s", str_json);
            feature_free_cstring(ctx, str_json);
            feature_free_value(ctx, json_obj);
        } else {
            feature_value_t str_obj = feature_to_string(ctx, param);
            const char* str_json = feature_to_cstring(ctx, str_obj);
            printf("%s", str_json);
            feature_free_cstring(ctx, str_json);
            feature_free_value(ctx, str_obj);
        }
    }
    printf("\n");
}
