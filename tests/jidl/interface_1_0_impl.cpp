// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "interface_1_0.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

static const char* file_tag = "[jidl_feature] interface_1_0_impl";

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

// vtalble functions implemented for cat
static FtString _Interface_cat_get_name(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    char* buf = (char*)FeatureMalloc(128, FT_CHAR);
    sprintf(buf, "cat name is: %s", "mimi");
    return buf;
}

static void _Interface_cat_set_name(FeatureInstanceHandle feature, AppendData data, FtString name)
{
    printf("%s::%s() set cat name: %s\n", file_tag, __FUNCTION__, name);
}

static FtInt _Interface_cat_get_legCount(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s() %s\n", file_tag, __FUNCTION__, "cat leg count is 4");
    return 4;
}

static FtInt _Interface_cat_eatFood(FeatureInstanceHandle feature, AppendData data, FtArray& foods)
{
    FTArrayHelper<const char*> string_array(&foods);
    printf("%s::%s() cat eat food, array_size: %d\n", file_tag,  __FUNCTION__, string_array.size());
    printf("food array = [\n");
    for (int32_t i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

static FtString _Interface_cat_run(FeatureInstanceHandle feature, AppendData data, FtInt distance, FtString destination)
{
    printf("%s::%s() cat run, distance: %d, destination: %s\n", file_tag, __FUNCTION__, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_CHAR);
    sprintf(buf, "cat run swiftly!");
    return buf;
}

static FtArray* _Interface_cat_fly(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s() %s\n", file_tag, __FUNCTION__, "cat can not fly");
    FtArray* strArray = Interface_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_CHAR));
        sprintf(str, "cat flip wings %d", i);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

void _Interface_cat_walk(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid)
{
    printf("%s::%s() %s\n", file_tag, __FUNCTION__, "cat walk slowly");
    char* buf = (char*)FeatureMalloc(128, FT_CHAR);
    sprintf(buf, "cat walk resolved!");
    FeaturePromiseResolve(feature, pid, buf);
}

// vtalble functions implemented for dog
FtString Interface_Animal_interface_dog_get_name(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    char* buf = (char*)FeatureMalloc(128, FT_CHAR);
    sprintf(buf, "dog name is: %s", "tommy");
    return buf;
}

void Interface_Animal_interface_dog_set_name(FeatureInstanceHandle feature, AppendData data, FtString name)
{
    printf("%s::%s() set dog name: %s\n", file_tag, __FUNCTION__, name);
}

FtInt Interface_Animal_interface_dog_get_legCount(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s() %s\n", file_tag, __FUNCTION__, "dog leg count is 4");
    return 4;
}

FtInt Interface_Animal_interface_dog_eatFood(FeatureInstanceHandle feature, AppendData data, FtArray& foods)
{
    FTArrayHelper<const char*> string_array(&foods);
    printf("%s::%s() dog eat food array_size: %d\n", file_tag,  __FUNCTION__, string_array.size());
    printf("food array = [\n");
    for (int32_t i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

FtString Interface_Animal_interface_dog_run(FeatureInstanceHandle feature, AppendData data, FtInt distance, FtString destination)
{
    printf("%s::%s() dog run, distance: %d, destination: %s\n", file_tag, __FUNCTION__, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_CHAR);
    sprintf(buf, "dog run swiftly!");
    return buf;
}

FtArray* Interface_Animal_interface_dog_fly(FeatureInstanceHandle feature, AppendData data)
{
    printf("%s::%s() %s\n", file_tag, __FUNCTION__, "dog can not fly");
    FtArray* strArray = Interface_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_CHAR));
        sprintf(str, "dog flip wings %d", i+4);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

void Interface_Animal_interface_dog_walk(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid)
{
    printf("%s::%s() %s\n", file_tag, __FUNCTION__, "dog walk fastly");
    FeaturePromiseReject(feature, pid, 5);
}

// FeatureCallbacks to be implemented
void Interface_onRegister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Interface_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Interface_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Interface_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Interface_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

void Interface_onUnregister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag,  __FUNCTION__);
}

// Function wrappers to be implemented

void Interface_wrap_flyFar(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt distance) {
    printf("%s::%s() distance: %d\n", file_tag, __FUNCTION__, distance);
    char* buf = (char*)FeatureMalloc(128, FT_CHAR);
    sprintf(buf, "flyFar resolved!");
    FeaturePromiseResolve(feature, pid, buf);
}

FeatureInstanceHandle Interface_wrap_createCat(FeatureInstanceHandle feature, AppendData data) {
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    // we should combine the vtable
    static NativeFunc cat_vtable[] = {
        nullptr,
        NativeFunc(_Interface_cat_get_name),
        NativeFunc(_Interface_cat_set_name),
        NativeFunc(_Interface_cat_get_legCount),
        NativeFunc(_Interface_cat_eatFood),
        NativeFunc(_Interface_cat_run),
        NativeFunc(_Interface_cat_fly),
        NativeFunc(_Interface_cat_walk)
    };
    return FeatureCreateInterface(feature, cat_vtable, countof(cat_vtable));
}

void Interface_wrap_setAnimal(FeatureInstanceHandle feature, AppendData data, FeatureInstanceHandle animal) {
    printf("%s::%s() animal: %p\n", file_tag, __FUNCTION__, animal);
}

void Interface_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params)
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
