
#include "ajs_features_init.h"
#include "feature_exports.h"
#include "feature_description.h"
#include "feature_log.h"
#include <ffi.h>
#include <string.h>

using namespace ferry;
using namespace FEATURE;

#define countof(x) (sizeof(x) / sizeof(x[0]))

FeatureInstanceHandle __createDog(FeatureInstanceHandle handle, AppendData data);
FeatureInstanceHandle __createCat(FeatureInstanceHandle handle, AppendData data);

void __print(FeatureInstanceHandle handle, AppendData data, FtVariParams vari_params)
{
    ft_context_ref ft_ctx = FeatureGetContext(handle);
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

void __printNameCat(FeatureInstanceHandle handle, AppendData data)
{
    ft_context_ref ft_ctx = FeatureGetContext(handle);
    printf("I'm a cat !\n");
}

void __printNameDog(FeatureInstanceHandle handle, AppendData data)
{
    ft_context_ref ft_ctx = FeatureGetContext(handle);
    printf("I'm a dog !\n");
}

void __receiveInterface(FeatureInstanceHandle handle, AppendData data, FeatureInstanceHandle instance)
{
    printf("%s: handle: %p, data: %ld, instance: %p\n", __func__, handle, data.i64, instance);
}

static char* __init_nameCat(FeatureInstanceHandle handle, int64_t data)
{
    printf("%s: enter...\n", __func__);
    char* name = static_cast<char*>(FeatureMalloc(strlen("cat mimi") + 1, FT_CHAR));
    strcpy(name, "cat mimi");
    return name;
}

static char* __init_nameDog(FeatureInstanceHandle handle, int64_t data)
{
    printf("%s: enter...\n", __func__);
    char* name = static_cast<char*>(FeatureMalloc(strlen("dog wangwang") + 1, FT_CHAR));
    strcpy(name, "dog wangwang");
    return name;
}

static FeatureType print_parameters[] = {
    FT_PARAM_REST_END
};

extern const InterfaceType animal_interface_type;

static FeatureType receive_interface_parameters[] = {
    FT_MK_COMPLEX_REF(&animal_interface_type),
    FT_PARAM_END
};

static FeatureType create_dog_parameters[] = {
    FT_PARAM_REST_END
};

static FeatureType create_cat_parameters[] = {
    FT_PARAM_REST_END
};

static const Member g_interface_members[] = {
    { .type = MEMBER_METHOD, .name = "printName", .method = { .func = { .vtable_idx = 0 }, .parameters = print_parameters, .return_type = FT_VOID, .data = { 0 } } },
    { .type = MEMBER_METHOD, .name = "receiveInterface", .method = { .func = { .vtable_idx = 1 }, .parameters = receive_interface_parameters, .return_type = FT_VOID, .data = { 0 } } },
    { .type = MEMBER_CONST, .name = "name", .value = { .type = FT_STRING, .func = { .vtable_idx = 2 }, .data = { .ptr = nullptr } } },
};

static const FeatureDescription animal_description = {
    .version = 1,
    .name = "Animal",
    .description = "Animal description",
    { .dynamic = true },
    nullptr,
    countof(g_interface_members),
    g_interface_members,
};

const InterfaceType animal_interface_type {
    .header = { .type = COMPLEX_INTERFACE, .size = 0 },
    .desc = &animal_description
};

static const Member g_members[] = {
    { .type = MEMBER_METHOD, .name = "print", .method = { .func = { .callback = FFI_FN(__print) }, .parameters = print_parameters, .return_type = FT_VOID, .data = { 0 } } },
    { .type = MEMBER_METHOD, .name = "createDog", .method = { .func = { .callback = FFI_FN(__createDog) }, .parameters = create_dog_parameters, .return_type = FT_MK_COMPLEX_REF(&animal_interface_type), .data = { 0 } } },
    { .type = MEMBER_METHOD, .name = "createCat", .method = { .func = { .callback = FFI_FN(__createCat) }, .parameters = create_cat_parameters, .return_type = FT_MK_COMPLEX_REF(&animal_interface_type), .data = { 0 } } },
};

// callbacks
static const struct FeatureCallbacks callbacks {
    [](const char* feature_name) {
        FEATURE_LOG_INFO("onRegister");
    },
        [](FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
            FEATURE_LOG_INFO("onCreate");
        },
        [](FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
            FEATURE_LOG_INFO("onRequired");
        },
        [](FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
            FEATURE_LOG_INFO("onDetached");
        },
        [](FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
            FEATURE_LOG_INFO("onDestroy");
        },
        [](const char* feature_name) {
            FEATURE_LOG_INFO("onUnregister");
        }
};

static FeatureDescription interface_description = { .version = 1, .name = "interface", .description = "interface demo description", { .dynamic = false }, .native_callbacks = &callbacks, .member_count = countof(g_members), .members = g_members };

FeatureInterfaceHandle __createDog(FeatureInstanceHandle handle, AppendData data)
{
    // we should combine the vtable
    static const NativeFunc dog_vtable_members[] = {
        NativeFunc(__printNameDog),
        NativeFunc(__receiveInterface),
        NativeFunc(__init_nameDog),
    };
    static VTable dog_vtable = {
        .size = 3,
        .finalizer = nullptr,
        .members = dog_vtable_members
    };
    return FeatureCreateInterface(handle, &dog_vtable);
}

FeatureInterfaceHandle __createCat(FeatureInstanceHandle handle, AppendData data)
{
    // we should combine the vtable
    static const NativeFunc cat_vtable_members[] = {
        NativeFunc(__printNameCat),
        NativeFunc(__receiveInterface),
        NativeFunc(__init_nameCat),
    };
    static VTable cat_vtable = {
        .size = 3,
        .finalizer = nullptr,
        .members = cat_vtable_members
    };
    return FeatureCreateInterface(handle, &cat_vtable);
}

QAPPFEATURE_INIT(interface)
{
    bool ret = false;
    ret = mgr->registerFeature(features, &interface_description);
    return ret;
}
