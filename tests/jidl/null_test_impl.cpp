// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "null_test.h"
#include <inttypes.h>

static const char* file_tag = "[jidl_feature] null_test_impl";

// FeatureCallbacks to be implemented
void null_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void null_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void null_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void null_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void null_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void null_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

static void invoke_chapter_changed_cb(FeatureInstanceHandle feature, FtCallbackId chap_changed, int index, const char* title)
{
    FEATURE_LOG_ERROR("chap_changed cb: %d, index: %d, title: %s", chap_changed, index, title);
    if (chap_changed == 0) {
        return;
    }
    if (!FeatureInvokeCallback(feature, chap_changed, index, title)) {
        FEATURE_LOG_ERROR("invoke failed!");
        return;
    }
    FeatureRemoveCallback(feature, chap_changed);
}

// Function wrappers to be implemented
void null_test_wrap_setChapter(FeatureInstanceHandle feature, AppendData append_data, FtInt index, null_test_Chapter* chap)
{
    printf("%s::%s(), chap: %p \n", file_tag, __FUNCTION__, chap);
    if (!chap) {
        FEATURE_LOG_ERROR("null chpter ptr!");
        return;
    }
    printf("chapter: { title: %s, page_count: %d }\n", chap->title, chap->page_count);
}

void null_test_wrap_setChapChangedCb(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId chap_changed)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    invoke_chapter_changed_cb(feature, chap_changed, 0, "monkey born from a mountain");
}

void null_test_wrap_setBook(FeatureInstanceHandle feature, AppendData append_data, null_test_Book* book)
{
    printf("%s::%s(), book: %p \n", file_tag, __FUNCTION__, book);
    if (!book) {
        FEATURE_LOG_ERROR("null book ptr!");
        return;
    }
    printf("book: { title: %s, page_count: %d, first_chap: %p, book_info: %p }\n",
        book->title, book->page_count, book->first_chap, book->book_info);
    invoke_chapter_changed_cb(feature, book->chap_changed, 1, "monkey born from a river");
}

void null_test_wrap_print(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params)
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
