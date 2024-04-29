// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "any_test.h"
#include "feature_utils.h"

static const char* file_tag = "[jidl_feature] any_test_impl";

// FeatureCallbacks to be implemented
void any_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void any_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void any_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void any_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void any_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void any_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented
void any_test_wrap_setAny(FeatureInstanceHandle feature, AppendData append_data, FtInt a, FtAny any)
{
    printf("%s::%s(), a: %d\n", file_tag, __FUNCTION__, a);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    size_t buff_size;
    FEATURE_LOG_ERROR("any ptr: %p", any);
    if (any) {
        const char* str_json = ft_to_string(ft_ctx, *any);
        printf("any json string: %s", str_json);
        ft_free_string(ft_ctx, str_json);
        printf("\n");
    }
}

FtAny any_test_wrap_getAny(FeatureInstanceHandle feature, AppendData append_data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    ft_value_t* any_ptr = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    FEATURE_LOG_ERROR("any ptr: %p", any_ptr);
    *any_ptr = ft_from_string(ft_ctx, "this is a string any value");
    return any_ptr;
}

void any_test_wrap_print(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params)
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
