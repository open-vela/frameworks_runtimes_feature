// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "arraybuffer_test.h"
#include "feature_utils.h"

static const char* file_tag = "[jidl_feature] arraybuffer_test_impl";

// FeatureCallbacks to be implemented
void arraybuffer_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void arraybuffer_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void arraybuffer_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void arraybuffer_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void arraybuffer_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void arraybuffer_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented

void arraybuffer_test_wrap_setArraybuffer(FeatureInstanceHandle feature, AppendData append_data, FtInt a, FtAny buffer)
{
    printf("%s::%s(), a: %d, arraybuffer: %p\n", file_tag, __FUNCTION__, a, buffer);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    ft_type val_type = ft_get_type(ft_ctx, *buffer);
    if (val_type == FT_TYPE_BUFFER || val_type == FT_TYPE_TYPED_BUFFER) {
        size_t buff_size;
        uint8_t* buff = ft_to_buffer(ft_ctx, &buff_size, *buffer);
        printf("%s::%s(), buffer type: %d, buffer_size: %ld\n", file_tag, __FUNCTION__, val_type, buff_size);
        for (int i = 0; i < buff_size; ++i) {
            printf(" buffer[%d]: %d\n", i, buff[i]);
        }
        return;
    }

    printf("%s::%s(), %s\n", file_tag, __FUNCTION__, "not a arraybuffer or typed Arraybuffer type");
}

FtAny arraybuffer_test_wrap_getArraybuffer(FeatureInstanceHandle feature, AppendData append_data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    size_t buff_size = 16;
    unsigned char* out_buff = (unsigned char*)alloca(buff_size);
    memset(out_buff, 0, buff_size);
    for (int i = 0; i < buff_size; ++i) {
        out_buff[i] = i;
    }

    ft_value_t* ret_ptr = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    FEATURE_LOG_ERROR("arraybuffer ptr: %p\n", ret_ptr);
    *ret_ptr = ft_from_buffer(ft_ctx, out_buff, buff_size);
    return ret_ptr;
}

FtAny arraybuffer_test_wrap_getTypedArraybuffer(FeatureInstanceHandle feature, AppendData append_data, FtInt type)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    size_t buff_size = 16;
    unsigned char* out_buff = (unsigned char*)alloca(buff_size);
    memset(out_buff, 0, buff_size);
    for (int i = 0; i < buff_size; ++i) {
        out_buff[i] = i;
    }

    ft_value_t* ret_ptr = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    FEATURE_LOG_ERROR("any arraybuffer ptr: %p", ret_ptr);
    *ret_ptr = ft_from_typed_buffer(ft_ctx, out_buff, buff_size, type);
    return ret_ptr;
}

void arraybuffer_test_wrap_print(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params)
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
