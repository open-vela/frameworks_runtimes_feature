// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "interface_test.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

static const char* file_tag = "[jidl_feature] interface_1_0_impl";

template <typename T>
class FtArrayHelper {
private:
    FtArray* _data;

public:
    FtArrayHelper(FtArray* data)
    {
        _data = data;
    }

    ~FtArrayHelper()
    {
    }

    T& operator[](int32_t index)
    {
        return ((T*)_data->_element)[index];
    }

    int32_t size() const { return _data->_size; }
};

// vtalble functions implemented for cat

static void _Interface_cat_finalize(FeatureInterfaceHandle handle)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
}

static FtString _Interface_cat_get_name(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cat name is: %s", "mimi");
    return buf;
}

static void _Interface_cat_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name)
{
    printf("%s::%s(), interface: %p, set cat name: %s\n", file_tag, __FUNCTION__, handle, name);
}

static FtInt _Interface_cat_get_legCount(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p, cat leg count is: %d\n", file_tag, __FUNCTION__, handle, 4);
    return 4;
}

static FtInt _Interface_cat_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray* foods)
{
    FtArrayHelper<const char*> string_array(foods);
    printf("%s::%s(), interface: %p, cat eat food, array_size: %d\n", file_tag, __FUNCTION__, handle, string_array.size());
    printf("food array = [\n");
    for (int32_t i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

static FtString _Interface_cat_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination)
{
    printf("%s::%s(), interface: %p, cat run, distance: %d, destination: %s\n", file_tag, __FUNCTION__, handle, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cat run swiftly!");
    return buf;
}

// vtalble functions implemented for dog
void interface_test_Animal_interface_dog_finalize(FeatureInterfaceHandle handle)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
}

FtString interface_test_Animal_interface_dog_get_name(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "dog name is: %s", "tommy");
    return buf;
}

void interface_test_Animal_interface_dog_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name)
{
    printf("%s::%s(), interface: %p, set dog name: %s\n", file_tag, __FUNCTION__, handle, name);
}

FtInt interface_test_Animal_interface_dog_get_legCount(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p, dog leg count is: %d\n", file_tag, __FUNCTION__, handle, 4);
    return 4;
}

FtInt interface_test_Animal_interface_dog_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray* foods)
{
    FtArrayHelper<const char*> string_array(foods);
    printf("%s::%s(), interface: %p, dog eat food, array_size: %d\n", file_tag, __FUNCTION__, handle, string_array.size());
    printf("food array = [\n");
    for (int32_t i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

FtString interface_test_Animal_interface_dog_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination)
{
    printf("%s::%s(), interface: %p, dog run, distance: %d, destination: %s\n", file_tag, __FUNCTION__, handle, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "dog run swiftly!");
    return buf;
}

// vtable functions for interface constructor function 'createBird'
void interface_test_Bird_interface_pigeon_finalize(FeatureInterfaceHandle handle)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
}

FtArray* interface_test_Bird_interface_pigeon_fly(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p, %s\n", file_tag, __FUNCTION__, handle, "pigeon fly");
    FtArray* strArray = interface_test_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "pigeon flip wings %d", i + 4);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

FtString interface_test_Bird_interface_pigeon_get_breed(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "pigeon breed is: %s", "Victoria Crown");
    return buf;
}

void interface_test_Bird_interface_pigeon_set_breed(FeatureInterfaceHandle handle, AppendData data, FtString breed)
{
    printf("%s::%s(), interface: %p, set pigeon breed: %s\n", file_tag, __FUNCTION__, handle, breed);
}

// vtable functions for interface constructor function 'createChicken'
void interface_test_Chicken_interface_cock_finalize(FeatureInterfaceHandle handle)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
}

FtString interface_test_Chicken_interface_cock_get_name(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cock name is: %s", "gugu");
    return buf;
}

void interface_test_Chicken_interface_cock_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name)
{
    printf("%s::%s(), interface: %p, set cock name: %s\n", file_tag, __FUNCTION__, handle, name);
}

FtInt interface_test_Chicken_interface_cock_get_legCount(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p, cock leg count is: %d\n", file_tag, __FUNCTION__, handle, 2);
    return 2;
}

FtInt interface_test_Chicken_interface_cock_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray* foods)
{
    FtArrayHelper<const char*> string_array(foods);
    printf("%s::%s(), interface: %p, cock eat food, array_size: %d\n", file_tag, __FUNCTION__, handle, string_array.size());
    printf("food array = [\n");
    for (int32_t i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

FtString interface_test_Chicken_interface_cock_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination)
{
    printf("%s::%s(), interface: %p, cock run, distance: %d, destination: %s\n", file_tag, __FUNCTION__, handle, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cock run with wings flip!");
    return buf;
}

FtArray* interface_test_Chicken_interface_cock_fly(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p, %s\n", file_tag, __FUNCTION__, handle, "cock fly");
    FtArray* strArray = interface_test_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "cock flip wings %d", i + 4);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

FtString interface_test_Chicken_interface_cock_get_breed(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cock breed is: %s", "Plymouth Rock");
    return buf;
}

void interface_test_Chicken_interface_cock_set_breed(FeatureInterfaceHandle handle, AppendData data, FtString breed)
{
    printf("%s::%s(), interface: %p, set cock breed: %s\n", file_tag, __FUNCTION__, handle, breed);
}

FtInt interface_test_Chicken_interface_cock_get_weight(FeatureInterfaceHandle handle, AppendData data)
{
    printf("%s::%s(), interface: %p, cock weight is: %d kg\n", file_tag, __FUNCTION__, handle, 2);
    return 2;
}

void interface_test_Chicken_interface_cock_set_weight(FeatureInterfaceHandle handle, AppendData data, FtInt weight)
{
    printf("%s::%s(), interface: %p, set cock weight to: %d kg\n", file_tag, __FUNCTION__, handle, weight);
}

void interface_test_Chicken_interface_cock_walk(FeatureInterfaceHandle handle, AppendData data, FtPromiseId pid)
{
    printf("%s::%s(), interface: %p, %s\n", file_tag, __FUNCTION__, handle, "cock walk slowly");
    FtArray* strArray = interface_test_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "cock walk %d", i);
        ((char**)strArray->_element)[i] = str;
    }
    FeaturePromiseResolve(handle, pid, strArray);
    FeatureFreeValue(strArray);
}

// FeatureCallbacks to be implemented
void interface_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void interface_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void interface_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle feature)
{
    printf("%s::%s(), feature: %p\n", file_tag, __FUNCTION__, feature);
}

void interface_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle feature)
{
    printf("%s::%s(), feature: %p\n", file_tag, __FUNCTION__, feature);
}

void interface_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void interface_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented

FeatureInterfaceHandle interface_test_wrap_createDog(FeatureInstanceHandle feature, AppendData data, FtInt type)
{
    FeatureInterfaceHandle handle = interface_test_createDog_instance(feature);
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);
    // void* data = create_dog_data(type);
    // FeatureSetObjectData(handle, data);
    return handle;
}

FeatureInterfaceHandle interface_test_wrap_createPigeon(FeatureInstanceHandle feature, AppendData data)
{
    FeatureInterfaceHandle handle = interface_test_createPigeon_instance(feature);
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);
    // void* data = create_pigeon_data();
    // FeatureSetObjectData(handle, data);
    return handle;
}

FeatureInterfaceHandle interface_test_wrap_createCock(FeatureInstanceHandle feature, AppendData data)
{
    FeatureInterfaceHandle handle = interface_test_createCock_instance(feature);
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);
    // void* data = create_cock_data();
    // FeatureSetObjectData(handle, data);
    return handle;
}

FeatureInterfaceHandle interface_test_wrap_createCat(FeatureInstanceHandle feature, AppendData data)
{
    static const NativeFunc cat_vtable_members[] = {
        NativeFunc(_Interface_cat_get_name),
        NativeFunc(_Interface_cat_set_name),
        NativeFunc(_Interface_cat_get_legCount),
        NativeFunc(_Interface_cat_eatFood),
        NativeFunc(_Interface_cat_run),
    };
    static VTable cat_vtable = {
        .size = 5,
        .finalizer = NativeFunc(_Interface_cat_finalize),
        .members = cat_vtable_members
    };
    printf("call interface_test_wrap_createCat\n");
    FeatureInterfaceHandle handle = FeatureCreateInterface(feature, &cat_vtable);
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);
    // void* data = create_cat_data();
    // FeatureSetObjectData(handle, data);
    return handle;
}

void interface_test_wrap_setAnimal(FeatureInstanceHandle feature, AppendData data, FeatureInstanceHandle animal)
{
    printf("%s::%s(), feature: %p, animal: %p\n", file_tag, __FUNCTION__, feature, animal);
}

void interface_test_wrap_flyFar(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt distance)
{
    printf("%s::%s(), feature: %p, distance: %d\n", file_tag, __FUNCTION__, feature, distance);
    FtArray* strArray = interface_test_malloc_string_array();
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "flip wings %d", i);
        ((char**)strArray->_element)[i] = str;
    }
    FeaturePromiseResolve(feature, pid, strArray);
    FeatureFreeValue(strArray);
}

void interface_test_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params)
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
