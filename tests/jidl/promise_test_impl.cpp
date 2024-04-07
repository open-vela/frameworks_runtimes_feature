// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "promise_test.h"

static const char* file_tag = "[jidl_feature] promise_1_0_impl";

template <typename T>
class FTArrayHelper {
private:
    FtArray* _data;

public:
    FTArrayHelper(FtArray* data)
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
void promise_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented
void promise_test_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a, FtString b)
{
    printf("%s::%s(), a: %d, b: %s\n", file_tag, __FUNCTION__, a, b);
    int rs = a;
    int rj = 10;
    if (rs != 0) {
        FeaturePromiseResolve(feature, pid, rs);
    } else {
        FeaturePromiseReject(feature, pid, rj);
    }
}

void promise_test_wrap_foo1(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a)
{
    printf("%s::%s(), a: %d\n", file_tag, __FUNCTION__, a);
    int rs = a;
    const char* rj = "hello";
    if (rs != 0) {
        FeaturePromiseResolve(feature, pid, rs);
    } else {
        FeaturePromiseReject(feature, pid, rj);
    }
}

void promise_test_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    const char* rj = "world";
    FeaturePromiseReject(feature, pid, rj);
}

void promise_test_wrap_bar(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_wrap_bar1(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_test_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params)
{
    printf("[jidl_feature] ");
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    for (int i = 0; i < vari_params.vari_count; i++) {
        ft_value_t param = vari_params.vari_args[i];
        ft_type param_type = ft_get_type(ft_ctx, param);
        if (param_type == FT_TYPE_OBJECT) {
            const char* param_obj = ft_to_string(ft_ctx, param);
            printf("%s ", param_obj);
            ft_free_string(ft_ctx, param_obj);
        } else if (param_type == FT_TYPE_ARRAY) {
            uint32_t array_size = ft_array_size(ft_ctx, param);
            printf("[");
            for (uint32_t j = 0; j < array_size; ++j) {
                ft_value_t elem = ft_array_at(ft_ctx, param, j);
                ft_type elem_type = ft_get_type(ft_ctx, elem);
                if (elem_type == FT_TYPE_NUMBER) {
                    double param_num;
                    if (ft_to_double(ft_ctx, elem, &param_num))
                        printf("%lf ", param_num);
                } else if (elem_type == FT_TYPE_STRING) {
                    const char* param_str = ft_to_string(ft_ctx, elem);
                    printf("%s ", param_str);
                    ft_free_string(ft_ctx, param_str);
                } else if (elem_type == FT_TYPE_BOOL) {
                    bool param_bool;
                    ft_to_bool(ft_ctx, param, &param_bool);
                    printf("%d ", param_bool);
                } else {
                    printf("invalid array element type!");
                    return;
                }
            }
            printf("] ");
        } else if (param_type == FT_TYPE_STRING) {
            const char* param_str = ft_to_string(ft_ctx, param);
            printf("%s ", param_str);
            ft_free_string(ft_ctx, param_str);
        } else if (param_type == FT_TYPE_NUMBER) {
            double param_num;
            ft_to_double(ft_ctx, param, &param_num);
            printf("%lf ", param_num);
        } else if (param_type == FT_TYPE_BOOL) {
            bool param_bool;
            ft_to_bool(ft_ctx, param, &param_bool);
            printf("%d ", param_bool);
        } else {
            printf("invalid param type!");
            return;
        }
    }
    printf("\n");
}
