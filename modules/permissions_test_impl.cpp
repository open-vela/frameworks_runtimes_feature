// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "permissions_test.h"

static const char* file_tag = "[jidl_feature] permissions_test_impl";

// FeatureCallbacks to be implemented
void permissions_test_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void permissions_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void permissions_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void permissions_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void permissions_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void permissions_test_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

// Function wrappers to be implemented
FtInt permissions_test_wrap_foo(FeatureInstanceHandle feature, AppendData append_data, FtInt a, FtString b, FtDouble c)
{
    FEATURE_LOG_INFO("%s, a: %d, b: %s, c: %f", file_tag, a, b, c);
    return 2;
}

void permissions_test_wrap_foo2(FeatureInstanceHandle feature, AppendData append_data, FtInt a, FtVariParams vari_params)
{
    FEATURE_LOG_INFO("%s, a: %d", file_tag, a);
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
                    printf("invalid array element type!\n");
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
            printf("invalid param type!\n");
            return;
        }
    }
    printf("\n");
}

void permissions_test_wrap_bar(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtInt type)
{
    FEATURE_LOG_INFO("%s, pid: %d, type: %" PRIi32, file_tag, pid, type);
    if (type != 0) {
        FeaturePromiseResolve(feature, pid, type);
    } else {
        FeaturePromiseReject(feature, pid, 202, "bar promise rejected");
    }
}

void permissions_test_wrap_bar2(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtFloat a, FtString b)
{
    FEATURE_LOG_INFO("%s, a: %f, b: %s", file_tag, a, b);
    if (a > 1.0) {
        FeaturePromiseResolve(feature, pid, b);
    } else {
        FeaturePromiseReject(feature, pid, 202, "bar2 promise rejected");
    }
}

void permissions_test_wrap_goo(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId cb1, FtCallbackId cb2)
{
    FEATURE_LOG_INFO("%s, will invoke cb1: %d", file_tag, cb1);
    if (!FeatureInvokeCallback(feature, cb1, 5, "hello")) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb1);

    FEATURE_LOG_INFO("%s, will invoke cb2: %d", file_tag, cb2);
    int32_t* var3 = (int32_t*)FeatureMalloc(sizeof(int32_t), FT_INT32);
    *var3 = 50;
    double* var4 = (double*)FeatureMalloc(sizeof(double), FT_DOUBLE);
    *var4 = 4.5;
    bool ret = FeatureInvokeCallbackCount(feature, cb2, 4, 20, "hello world", var3, var4);
    FeatureFreeValue(var3);
    FeatureFreeValue(var4);
    if (!ret) {
        FEATURE_LOG_ERROR("invoke failed!");
    }
    FeatureRemoveCallback(feature, cb2);
}

void permissions_test_wrap_goo2(FeatureInstanceHandle feature, AppendData append_data, permissions_test_Book* book)
{
    FEATURE_LOG_INFO("%s, book: %p", file_tag, book);
    if (book) {
        FEATURE_LOG_INFO("%s, fail: %d, complete: %d", file_tag, book->fail, book->complete);
        if (book->fail) {
            if (!FeatureInvokeCallback(feature, book->fail, "goo2 failed!", 200)) {
                FEATURE_LOG_ERROR("invoke fail failed !");
                return;
            }
            FeatureRemoveCallback(feature, book->fail);
        }
        if (book->complete) {
            if (!FeatureInvokeCallback(feature, book->complete)) {
                FEATURE_LOG_ERROR("invoke complete failed !");
                return;
            }
            FeatureRemoveCallback(feature, book->complete);
        }
    }
}
