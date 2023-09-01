// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "struct_1_0.h"


static const char* file_tag = "[jidl_feature] struct_1_0_impl";

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
void Struct_1_0_onRegister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Struct_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Struct_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Struct_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Struct_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Struct_1_0_onUnregister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

// Function wrappers to be implemented
void Struct_1_0_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, Struct_1_0_Chapter* b) {
    if (!b) {
        printf("%s::%s(), chapter ptr is null!\n", file_tag,  __FUNCTION__);
        return;
    }

    printf("%s::%s(), page_count: %d, title: %s\n",
        file_tag,  __FUNCTION__, b->_page_count, b->_title);
}

Struct_1_0_Chapter* Struct_1_0_wrap_bar(FeatureInstanceHandle feature, AppendData data, FtInt a) {
    printf("%s::%s(), a: %d\n", file_tag,  __FUNCTION__, a);
    Struct_1_0_Chapter* chap = mallocChapter();
    chap->_page_count = a;
    char* title = (char*)FTMalloc(128, FT_CHAR);
    sprintf(title, "title is: %s", "hello world");
    chap->_title = title;
    return chap;
}

void Struct_1_0_wrap_bar2(FeatureInstanceHandle feature, AppendData data, Struct_1_0_Book* a) {
    if (!a) {
        printf("%s::%s(), book ptr is null!\n", file_tag,  __FUNCTION__);
        return;
    }

    printf("%s::%s(), page_count: %d, title: %s\n",
        file_tag,  __FUNCTION__, a->_page_count, a->_title);

    if (!a->_chap_titles) {
        printf("%s::%s(), chap_titles ptr is null!\n", file_tag,  __FUNCTION__);
    } else {
        FTArrayHelper<const char*> chap_titles(a->_chap_titles);
        printf("%s::%s(), chap_titles: [", file_tag, __FUNCTION__);
        for (int32_t i = 0; i < chap_titles.size(); i++) {
            if (i > 0)
        	    printf(", ");
        	printf("%s", chap_titles[i]);
        }
        printf("]\n");
    }

    if (!a->_first_chap) {
        printf("%s::%s(), first_chap ptr is null!\n", file_tag,  __FUNCTION__);
    } else {
      printf("%s::%s(), first_chapter: [page_count: %d, title: %s]\n",
          file_tag,  __FUNCTION__, a->_first_chap->_page_count, a->_first_chap->_title);
    }

    int ret = InvokeFeatureCallback(feature, a->_chap_changed, 0, a->_title);
    if (ret) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    RemoveCallback(feature, a->_chap_changed);
}

void Struct_1_0_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters variadicParameters)
{
    printf("[jidl_feature] ");
    ft_context_ref ft_ctx = GetFeatureContext(feature);
    for (int i = 0; i < variadicParameters.variadic_count; i++) {
        ft_value_t param = variadicParameters.variadic_args[i];
        ft_type param_type = ft_get_type(ft_ctx, param);
        if (param_type == FT_TYPE_OBJECT) {
            const char* param_obj = ft_to_string(ft_ctx, param);
            printf("%s ", param_obj);
            ft_free_string(ft_ctx, param_obj);
        } else if (param_type == FT_TYPE_ARRAY) {
            uint32_t array_size = ft_array_size(ft_ctx, param);
            printf("[");
            for (int i = 0; i < array_size; ++i) {
                ft_value_t elem = ft_array_at(ft_ctx, param, i);
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
                    bool ret = ft_to_bool(ft_ctx, param, &param_bool);
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
            bool ret = ft_to_double(ft_ctx, param, &param_num);
            printf("%lf ", param_num);
        } else if (param_type == FT_TYPE_BOOL) {
            bool param_bool;
            bool ret = ft_to_bool(ft_ctx, param, &param_bool);
            printf("%d ", param_bool);
        } else {
            printf("invalid param type!");
            return;
        }
    }
    printf("\n");
}
