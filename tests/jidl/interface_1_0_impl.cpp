// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "interface_1_0.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

static const char* file_tag = "[jidl_feature] interface_1_0_impl";

// vtalble functions implemented for cat
static FtString _Interface_cat_get_name(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(),\n", file_tag, __FUNCTION__);
    ft_context_ref ft_ctx = GetFeatureContext(handle);
    char* buf = (char*)FTMalloc(128, FT_CHAR);
    sprintf(buf, "cat name is: %s", "mimi");
    return buf;
}

static void _Interface_cat_set_name(FeatureInstanceHandle handle, AppendData data, FtString name)
{
    printf("%s::%s(): set cat name: %s,\n", file_tag, __FUNCTION__, name);
    ft_context_ref ft_ctx = GetFeatureContext(handle);
}

static FtInt _Interface_cat_get_legCount(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "cat leg count is 4");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
    return 4;
}

static void _Interface_cat_eatFood(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "cat eat little food");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
}

static void _Interface_cat_run(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "cat run swiftly");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
}

static void _Interface_cat_fly(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "cat can not fly");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
}

// vtalble functions implemented for dog
static FtString _Interface_dog_get_name(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(),\n", file_tag, __FUNCTION__);
    ft_context_ref ft_ctx = GetFeatureContext(handle);
    char* buf = (char*)FTMalloc(128, FT_CHAR);
    sprintf(buf, "dog name is: %s", "tommy");
    return buf;
}

static void _Interface_dog_set_name(FeatureInstanceHandle handle, AppendData data, FtString name)
{
    printf("%s::%s(): set dog name: %s,\n", file_tag, __FUNCTION__, name);
    ft_context_ref ft_ctx = GetFeatureContext(handle);
}

static FtInt _Interface_dog_get_legCount(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "dog leg count is 4");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
    return 4;
}

static void _Interface_dog_eatFood(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "dog eat a lot of food");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
}

static void _Interface_dog_run(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "dog run fastly");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
}

static void _Interface_dog_fly(FeatureInstanceHandle handle, AppendData data)
{
    printf("%s::%s(): %s,\n", file_tag, __FUNCTION__, "dog can not fly");
    ft_context_ref ft_ctx = GetFeatureContext(handle);
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

void Interface_wrap_flyFar(FeatureInstanceHandle feature, AppendData data, FtInt distance) {
    printf("%s::%s(),\n", file_tag, __FUNCTION__);
}

FeatureInstanceHandle Interface_wrap_createCat(FeatureInstanceHandle feature, AppendData data) {
    printf("%s::%s(),\n", file_tag, __FUNCTION__);
    // we should combine the vtable
    static NativeFunc cat_vtable[] = {
        nullptr,
        NativeFunc(_Interface_cat_get_name),
        NativeFunc(_Interface_cat_set_name),
        NativeFunc(_Interface_cat_get_legCount),
        NativeFunc(_Interface_cat_eatFood),
        NativeFunc(_Interface_cat_run),
        NativeFunc(_Interface_cat_fly)
    };
    return FeatureCreateInterface(feature, cat_vtable, countof(cat_vtable));
}

FeatureInstanceHandle Interface_wrap_createDog(FeatureInstanceHandle feature, AppendData data, FtInt type) {
    printf("%s::%s(),\n", file_tag, __FUNCTION__);
    // we should combine the vtable
    static NativeFunc dog_vtable[] = {
        nullptr,
        NativeFunc(_Interface_dog_get_name),
        NativeFunc(_Interface_dog_set_name),
        NativeFunc(_Interface_dog_get_legCount),
        NativeFunc(_Interface_dog_eatFood),
        NativeFunc(_Interface_dog_run),
        NativeFunc(_Interface_dog_fly)
    };
    return FeatureCreateInterface(feature, dog_vtable, countof(dog_vtable));
}

void Interface_wrap_setAnimal(FeatureInstanceHandle feature, AppendData data, FeatureInstanceHandle animal) {
    printf("%s::%s(),\n", file_tag, __FUNCTION__);
}

void Interface_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters variadicParameters)
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
