// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "simple_1_0.h"
#include <inttypes.h>

static const char* file_tag = "[jidl_feature] simple_1_0_impl";

static const char* g_default_str = "unknown";
static const char* g_name = nullptr;
static const char* g_version = nullptr;

#define SET_PROP_CHAR_PTR(str)                \
    do {                                      \
        FeatureDupValue((void*)str);          \
        if (g_##str) {                        \
            FeatureFreeValue((void*)g_##str); \
            g_##str = nullptr;                \
        }                                     \
        g_##str = str;                        \
    } while (false)

#define GET_PROP_CHAR_PTR(str)                                                           \
    do {                                                                                 \
        if (g_##str) {                                                                   \
            FeatureDupValue((void*)g_##str);                                             \
            return g_##str;                                                              \
        }                                                                                \
        char* prop_ret_str = (char*)FeatureMalloc(strlen(g_default_str) + 1, FT_STRING); \
        sprintf(prop_ret_str, "%s", g_default_str);                                      \
        return prop_ret_str;                                                             \
    } while (false)

#define FREE_PROP_CHAR_PTR(str)               \
    do {                                      \
        if (g_##str) {                        \
            FeatureFreeValue((void*)g_##str); \
            g_##str = nullptr;                \
        }                                     \
    } while (false)

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
void Simple_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Simple_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Simple_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Simple_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Simple_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Simple_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    FREE_PROP_CHAR_PTR(name);
    FREE_PROP_CHAR_PTR(version);
}

void Simple_wrap_printStr(FeatureInstanceHandle feature, AppendData data, FtString str)
{
    printf("%s::%s(), str: %s\n", file_tag, __FUNCTION__, str);
}

void Simple_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params)
{
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

// Function wrappers to be implemented
int Simple_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtString c, FtDouble b)
{
    printf("%s::%s(), a: %d, c: %s, b: %f\n", file_tag, __FUNCTION__, a, c, b);
    return 0;
}

void Simple_wrap_bar(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Simple_wrap_bar5(FeatureInstanceHandle feature, AppendData data, FtInt a, FtVariParams vari_params)
{
    printf("%s::%s(), a: %d ", file_tag, __FUNCTION__, a);
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

FtString Simple_wrap_bar6(FeatureInstanceHandle feature, AppendData data, FtInt a, FtFloat b, FtBool c)
{
    printf("%s::%s(), a: %d, b: %f, c: %d\n", file_tag, __FUNCTION__, a, b, c);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "returned string: %d, %f, %d", a, b, c);
    return buf;
}

void Simple_wrap_goo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtInt b, FtCallbackId cb)
{
    // callback cb1(int x, string y, double z)
    printf("%s::%s(), a: %d, b: %d, will invoke cb\n", file_tag, __FUNCTION__, a, b);
    if (!FeatureInvokeCallback(feature, cb, a, "hello", (double)b)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void Simple_wrap_goo2(FeatureInstanceHandle feature, AppendData data, FtCallbackId cb, FtCallbackId cb3, FtCallbackId cb4)
{

    // callback cb3()
    printf("%s::%s(), will invoke cb3\n", file_tag, __FUNCTION__);
    if (!FeatureInvokeCallback(feature, cb3)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb3);

    // callback cb4(...)
    printf("%s::%s(), will invoke cb4\n", file_tag, __FUNCTION__);
    int32_t* var1 = (int32_t*)FeatureMalloc(sizeof(int32_t), FT_INT32);
    *var1 = 15;
    char* var2 = (char*)FeatureMalloc(sizeof("hello") + 1, FT_STRING);
    sprintf(var2, "%s", "hello");
    char* var3 = (char*)FeatureMalloc(sizeof("world") + 1, FT_STRING);
    sprintf(var3, "%s", "world");
    bool ret = FeatureInvokeCallbackCount(feature, cb4, 3, var1, var2, var3);
    FeatureFreeValue(var1);
    FeatureFreeValue(var2);
    FeatureFreeValue(var3);

    if (!ret) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb4);

    // callback cb2(int a, string b, ...)
    printf("%s::%s(), will invoke cb2\n", file_tag, __FUNCTION__);
    var1 = (int32_t*)FeatureMalloc(sizeof(int32_t), FT_INT32);
    *var1 = 50;
    double* var4 = (double*)FeatureMalloc(sizeof(double), FT_DOUBLE);
    *var4 = 4.5;
    ret = FeatureInvokeCallbackCount(feature, cb, 4, 20, "hello world", var1, var4);

    FeatureFreeValue(var1);
    FeatureFreeValue(var4);
    if (!ret) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void Simple_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FtCallbackId cb, FtCallbackId cb2)
{
    // callback cb1(int x, string y, double z)
    printf("%s::%s(), x: %d, y: %f, will invoke cb1\n", file_tag, __FUNCTION__, x, y);
    if (!FeatureInvokeCallback(feature, cb, x, "greeting", y)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);

    // callback cb2(int a, string b, ...)
    printf("%s::%s(), will invoke cb2\n", file_tag, __FUNCTION__);
    char* strValue = (char*)FeatureMalloc(sizeof("you") + 1, FT_STRING);
    sprintf(strValue, "%s", "you");
    bool ret = FeatureInvokeCallbackCount(feature, cb2, 3, x, "love", strValue);

    FeatureFreeValue(strValue);
    if (!ret) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb2);
}

void Simple_wrap_foo3(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FtCallbackId cb)
{
    // callback cb1(int x, string y, double z)
    printf("%s::%s(), x: %d, y: %f, will invoke cb1\n", file_tag, __FUNCTION__, x, y);
    if (!FeatureInvokeCallback(feature, cb, x, "greeting", y)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void Simple_wrap_justTestNeverCall1(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void Simple_wrap_justTestNeverCall2(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

FtInt Simple_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FtArray* values)
{
    FTArrayHelper<int> int_array(values);
    printf("%s::%s(), int_array size: %" PRIi32 "\n", file_tag, __FUNCTION__, int_array.size());
    printf("int_array = [\n");
    for (int32_t i = 0; i < int_array.size(); i++) {
        printf("  index %" PRIi32 ": %d\n", i, int_array[i]);
    }
    printf("]\n");
    return -1;
}

FtArray* Simple_wrap_bar3(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    FtArray* strArray = Simple_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "hello%d", i);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

// Property getters and setters to be implemented
const char* Simple_get_name(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    GET_PROP_CHAR_PTR(name);
}

void Simple_set_name(FeatureInstanceHandle feature, AppendData data, FtString name)
{
    printf("%s::%s(), name: %s\n", file_tag, __FUNCTION__, name);
    SET_PROP_CHAR_PTR(name);
}

const char* Simple_get_version(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    GET_PROP_CHAR_PTR(version);
}

FtArray* Simple_get_args(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    FtArray* strArray = Simple_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "hello%d", i);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}
