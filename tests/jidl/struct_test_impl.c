// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "struct_test.h"

static const char* file_tag = "[jidl_feature] struct_1_0_impl";

// FeatureCallbacks to be implemented
void struct_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void struct_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void struct_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void struct_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void struct_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void struct_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented
void struct_test_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, struct_test_Chapter* b)
{
    if (!b) {
        printf("%s::%s(), chapter ptr is null!\n", file_tag, __FUNCTION__);
        return;
    }

    printf("%s::%s(), page_count: %d, title: %s, is_end: %d\n",
        file_tag, __FUNCTION__, b->page_count, b->title, b->is_end);
}

struct_test_Chapter* struct_test_wrap_bar(FeatureInstanceHandle feature, AppendData data, FtInt a)
{
    printf("%s::%s(), a: %d\n", file_tag, __FUNCTION__, a);
    struct_test_Chapter* chap = struct_testMallocChapter();
    chap->page_count = a;
    char* title = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(title, "title is: %s", "hello world");
    chap->title = title;
    return chap;
}

void struct_test_wrap_bar2(FeatureInstanceHandle feature, AppendData data, struct_test_Book* a)
{
    if (!a) {
        printf("%s::%s(), book ptr is null!\n", file_tag, __FUNCTION__);
        return;
    }
    if (!a->any_param) {
        printf("%s::%s(), any_param ptr is null!\n", file_tag, __FUNCTION__);
    } else {
        printf("%s::%s(), any_param: ", file_tag, __FUNCTION__);
        ft_context_ref ft_ctx = FeatureGetContext(feature);
        const char* str_json = ft_to_string(ft_ctx, *(a->any_param));
        printf("%s", str_json);
        ft_free_string(ft_ctx, str_json);
        printf("\n");
    }

    printf("%s::%s(), page_count: %d, title: %s\n",
        file_tag, __FUNCTION__, a->page_count, a->title);

    for (int i = 0; i < a->chap_titles->_size; i++) {
        char* elem = ((char**)a->chap_titles->_element)[i];
        printf("%s::%s(), elem[%d]: %s\n",
            file_tag, __FUNCTION__, i, elem);
    }

    if (!a->first_chap) {
        printf("%s::%s(), first_chap ptr is null!\n", file_tag, __FUNCTION__);
    } else {
        printf("%s::%s(), first_chapter: [page_count: %d, title: %s]\n",
            file_tag, __FUNCTION__, a->first_chap->page_count, a->first_chap->title);
    }

    if (!FeatureInvokeCallback(feature, a->chap_changed, 0, a->title)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, a->chap_changed);
}

void struct_test_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams var_params)
{
    printf("[jidl_feature] ");
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    for (int i = 0; i < var_params.vari_count; i++) {
        ft_value_t param = var_params.vari_args[i];
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
