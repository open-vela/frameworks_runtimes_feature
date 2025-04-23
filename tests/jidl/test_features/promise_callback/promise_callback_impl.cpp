// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "feature_main_exports.h"
#include "promise_callback.h"

static const char* file_tag = "[jidl_feature] promise_callback_impl";

// FeatureCallbacks to be implemented
void promise_callback_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented
void promise_callback_wrap_foo_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtInt a, FtString b)
{
    bool resolve = a > 0;
    printf("%s::%s(), a: %d, b: %s, resolve: %d\n", file_tag, __FUNCTION__, a, b, resolve);
    if (resolve) {
        FeaturePromiseResolve(feature, pid, a);
    } else {
        FeaturePromiseReject(feature, pid, 202, b);
    }
}

void promise_callback_wrap_bar_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtInt a)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    bool resolve = a > 0;
    printf("%s::%s(), a: %d, resolve: %d\n", file_tag, __FUNCTION__, a, resolve);
    if (resolve) {
        FeaturePromiseResolve(feature, pid, "world resolve");
    } else {
        FeaturePromiseReject(feature, pid, 202, "world reject");
    }
}

void promise_callback_wrap_obj_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, promise_callback_Chapter* chap)
{
    int resolve = pid % 2;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (chap) {
        printf("%s::%s(), page_count: %d, title: %s, is_end: %d\n", file_tag, __FUNCTION__, chap->page_count, chap->title, chap->is_end);
    }
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    if (resolve) {
        ft_value_t page_count = ft_from_int(ft_ctx, 50);
        ft_value_t title = ft_from_string(ft_ctx, "hello");
        ft_value_t chap_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, chap_obj, "page_count", page_count);
        ft_obj_set_property(ft_ctx, chap_obj, "title", title);
        FeaturePromiseResolve(feature, pid, &chap_obj);
    } else {
        FeaturePromiseReject(feature, pid, 202, "world");
    }
}

FtAny promise_callback_wrap_loadLibrary(FeatureInstanceHandle feature, AppendData append_data, FtString name)
{
    printf("%s::%s(), name: %s\n", file_tag, __FUNCTION__, name);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t ft_undef = ft_undefined(ft_ctx);
    FtAny ft_lib = (FtAny)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *ft_lib = ft_undef;
    FeatureManagerHandle hmanager = FeatureGetManagerHandleFromInstance(feature);
    if (!hmanager) {
        return ft_lib;
    }
    *ft_lib = FeatureRequire(hmanager, ft_undef, name);
    return ft_lib;
}

void promise_callback_wrap_print(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params)
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
