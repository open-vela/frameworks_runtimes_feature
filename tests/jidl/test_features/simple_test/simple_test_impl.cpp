// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "simple_test.h"
#include <inttypes.h>

static const char* file_tag = "[jidl_feature] simple_test_impl";

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

typedef struct property_data {
    FtString name;
    FtString version;
} property_data;

static property_data* create_property_data()
{
    property_data* ret = (property_data*)malloc(sizeof(property_data));
    memset(ret, 0, sizeof(property_data));
    return ret;
}

// FeatureCallbacks to be implemented
void simple_test_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s,", file_tag);
}

void simple_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s,", file_tag);
}

void simple_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s,", file_tag);
    void* prop_data = (void*)create_property_data();
    FeatureSetObjectData(handle, prop_data);
}

void simple_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s,", file_tag);
    property_data* prop_data = (property_data*)FeatureGetObjectData(handle);
    if (prop_data == NULL)
        return;

    if (prop_data->name) {
        FeatureFreeValue((void*)prop_data->name);
    }
    if (prop_data->version) {
        FeatureFreeValue((void*)prop_data->version);
    }
    free(prop_data);
    FeatureSetObjectData(handle, NULL);
}

void simple_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s,", file_tag);
}

void simple_test_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s,", file_tag);
}

void simple_test_wrap_printStr(FeatureInstanceHandle feature, AppendData data, FtString str)
{
    FEATURE_LOG_INFO("%s, str: %s", file_tag, str);
}

// Function wrappers to be implemented
int simple_test_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtString c, FtDouble b)
{
    FEATURE_LOG_INFO("%s, a: %d, c: %s, b: %f", file_tag, a, c, b);
    return 0;
}

void simple_test_wrap_bar(FeatureInstanceHandle feature, AppendData data)
{
    FEATURE_LOG_INFO("%s,", file_tag);
}

void simple_test_wrap_bar5(FeatureInstanceHandle feature, AppendData data, FtInt a, FtVariParams vari_params)
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

FtString simple_test_wrap_bar6(FeatureInstanceHandle feature, AppendData data, FtInt a, FtFloat b, FtBool c)
{
    FEATURE_LOG_INFO("%s, a: %d, b: %f, c: %d", file_tag, a, b, c);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "returned string: %d, %f, %d", a, b, c);
    return buf;
}

void simple_test_wrap_goo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtInt b, FtCallbackId cb)
{
    // callback cb1(int x, string y, double z)
    FEATURE_LOG_INFO("%s, a: %d, b: %d, will invoke cb1", file_tag, a, b);
    if (!simple_test_cb1_invoke(feature, cb, a, "hello", (double)b)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void simple_test_wrap_goo2(FeatureInstanceHandle feature, AppendData data, FtCallbackId cb, FtCallbackId cb3, FtCallbackId cb4)
{

    // callback cb3()
    FEATURE_LOG_INFO("%s, will invoke cb3", file_tag);
    if (!simple_test_cb3_invoke(feature, cb3)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb3);

    // callback cb4(...)
    FEATURE_LOG_INFO("%s, will invoke cb4", file_tag);
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
    FEATURE_LOG_INFO("%s, will invoke cb2", file_tag);
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

void simple_test_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FtCallbackId cb, FtCallbackId cb2)
{
    // callback cb1(int x, string y, double z)
    FEATURE_LOG_INFO("%s, x: %d, y: %f, will invoke cb1", file_tag, x, y);
    if (!simple_test_cb1_invoke(feature, cb, x, "greeting", y)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);

    // callback cb2(int a, string b, ...)
    FEATURE_LOG_INFO("%s, will invoke cb2", file_tag);
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

void simple_test_wrap_foo3(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FtCallbackId cb)
{
    // callback cb1(int x, string y, double z)
    FEATURE_LOG_INFO("%s, x: %d, y: %f, will invoke cb1", file_tag, x, y);
    if (!simple_test_cb1_invoke(feature, cb, x, "greeting", y)) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void simple_test_wrap_justTestNeverCall1(FeatureInstanceHandle feature, AppendData data)
{
    FEATURE_LOG_INFO("%s,", file_tag);
}

void simple_test_wrap_justTestNeverCall2(FeatureInstanceHandle feature, AppendData data)
{
    FEATURE_LOG_INFO("%s,", file_tag);
}

FtInt simple_test_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FtArray* values)
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

FtArray* simple_test_wrap_bar3(FeatureInstanceHandle feature, AppendData data)
{
    FEATURE_LOG_INFO("%s,", file_tag);
    FtArray* strArray = simple_test_malloc_string_array();
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
const char* simple_test_get_name(FeatureInstanceHandle feature, AppendData data)
{
    FEATURE_LOG_INFO("%s, feature: %p", file_tag, feature);
    property_data* prop_data = (property_data*)FeatureGetObjectData(feature);
    if (prop_data == NULL) {
        FEATURE_LOG_ERROR("%s: prop_data is NULL!", file_tag);
        return NULL;
    }

    if (prop_data->name == NULL) {
        char* name_buf = (char*)FeatureMalloc(5, FT_STRING);
        sprintf(name_buf, "%s", "Tomy");
        prop_data->name = name_buf;
    }
    FeatureDupValue((void*)prop_data->name);

    return prop_data->name;
}

void simple_test_set_name(FeatureInstanceHandle feature, AppendData data, FtString name)
{
    FEATURE_LOG_INFO("%s, feature: %p, set name: %s", file_tag, feature, name);
    property_data* prop_data = (property_data*)FeatureGetObjectData(feature);
    if (prop_data == NULL) {
        FEATURE_LOG_ERROR("%s: prop_data is NULL!", file_tag);
        return;
    }

    FeatureDupValue((void*)name);
    if (prop_data->name) {
        FeatureFreeValue((void*)prop_data->name);
    }
    prop_data->name = name;
}

const char* simple_test_get_version(FeatureInstanceHandle feature, AppendData data)
{
    FEATURE_LOG_INFO("%s, feature: %p", file_tag, feature);
    property_data* prop_data = (property_data*)FeatureGetObjectData(feature);
    if (prop_data == NULL) {
        FEATURE_LOG_ERROR("%s: prop_data is NULL!", file_tag);
        return NULL;
    }

    if (prop_data->version == NULL) {
        char* version_buf = (char*)FeatureMalloc(10, FT_STRING);
        sprintf(version_buf, "%s", "version 1");
        prop_data->version = version_buf;
    }
    FeatureDupValue((void*)prop_data->version);
    return prop_data->version;
}

FtArray* simple_test_get_args(FeatureInstanceHandle feature, AppendData data)
{
    FEATURE_LOG_INFO("%s,", file_tag);
    FtArray* strArray = simple_test_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "hello%d", i);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}
