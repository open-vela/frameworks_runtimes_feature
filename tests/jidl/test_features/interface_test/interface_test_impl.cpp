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

typedef struct interface_test_cat_data {
    int leg_count;
    FtString name;
    bool ran;
} interface_test_cat_data;

static interface_test_cat_data* create_cat_data()
{
    interface_test_cat_data* ret = (interface_test_cat_data*)malloc(sizeof(interface_test_cat_data));
    memset(ret, 0, sizeof(interface_test_cat_data));
    ret->leg_count = 4;
    return ret;
}

// vtalble functions implemented for cat
static void _Interface_cat_finalize(FeatureInterfaceHandle handle)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
    interface_test_cat_data* cat_data = (interface_test_cat_data*)FeatureGetObjectData(handle);
    if (cat_data) {
        if (cat_data->name) {
            FeatureFreeValue((void*)cat_data->name);
        }
        free(cat_data);
    }
    FeatureSetObjectData(handle, NULL);
}

static FtString _Interface_cat_get_name(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
    interface_test_cat_data* cat_data = (interface_test_cat_data*)FeatureGetObjectData(handle);
    if (cat_data == NULL) {
        FEATURE_LOG_ERROR("%s: cat_data_ptr is NULL!", file_tag);
        return NULL;
    }

    if (cat_data->name) {
        FeatureDupValue((void*)cat_data->name);
        return cat_data->name;
    }

    char* buf = (char*)FeatureMalloc(5, FT_STRING);
    sprintf(buf, "%s", "mimi");
    return buf;
}

static void _Interface_cat_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name)
{
    FEATURE_LOG_INFO("%s, interface: %p, set cat name: %s", file_tag, handle, name);
    interface_test_cat_data* cat_data = (interface_test_cat_data*)FeatureGetObjectData(handle);
    if (cat_data == NULL) {
        FEATURE_LOG_ERROR("%s: cat_data_ptr is NULL!", file_tag);
        return;
    }

    FeatureDupValue((void*)name);
    if (cat_data->name) {
        FeatureFreeValue((void*)cat_data->name);
    }
    cat_data->name = name;
}

static FtInt _Interface_cat_get_legCount(FeatureInterfaceHandle handle, AppendData data)
{
    interface_test_cat_data* cat_data = (interface_test_cat_data*)FeatureGetObjectData(handle);
    if (cat_data == NULL) {
        FEATURE_LOG_ERROR("%s: cat_data_ptr is NULL!", file_tag);
        return 0;
    }
    FEATURE_LOG_INFO("%s, interface: %p, cat leg count is: %d", file_tag, handle, cat_data->leg_count);
    return cat_data->leg_count;
}

static FtInt _Interface_cat_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray* foods)
{
    FtArrayHelper<const char*> string_array(foods);
    FEATURE_LOG_INFO("%s, interface: %p, cat eat food, array_size: %d", file_tag, handle, string_array.size());
    printf("food array = [\n");
    for (int i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

static FtString _Interface_cat_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination)
{
    FEATURE_LOG_INFO("%s, interface: %p, cat run, distance: %d, destination: %s", file_tag, handle, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cat run swiftly!");
    return buf;
}

// vtalble functions implemented for dog
void interface_test_Animal_interface_dog_finalize(FeatureInterfaceHandle handle)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
}

FtString interface_test_Animal_interface_dog_get_name(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "%s", "tommy");
    return buf;
}

void interface_test_Animal_interface_dog_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name)
{
    FEATURE_LOG_INFO("%s, interface: %p, set dog name: %s", file_tag, handle, name);
}

FtInt interface_test_Animal_interface_dog_get_legCount(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p, dog leg count: %d", file_tag, handle, 4);
    return 4;
}

FtInt interface_test_Animal_interface_dog_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray* foods)
{
    FtArrayHelper<const char*> string_array(foods);
    FEATURE_LOG_INFO("%s, interface: %p, dog eat food, array_size: %d", file_tag, handle, string_array.size());
    printf("food array = [\n");
    for (int i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

FtString interface_test_Animal_interface_dog_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination)
{
    FEATURE_LOG_INFO("%s, interface: %p, dog run, distance: %d, destination: %s", file_tag, handle, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "dog run quickly!");
    return buf;
}

// vtalble functions implemented for pigeon
void interface_test_Bird_interface_pigeon_finalize(FeatureInterfaceHandle handle)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
}

FtArray* interface_test_Bird_interface_pigeon_fly(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
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
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "%s", "Victoria Crown");
    return buf;
}

void interface_test_Bird_interface_pigeon_set_breed(FeatureInterfaceHandle handle, AppendData data, FtString breed)
{
    FEATURE_LOG_INFO("%s, interface: %p, set pigeon breed: %s", file_tag, handle, breed);
}

// vtable functions for interface constructor function 'createChicken'
void interface_test_Chicken_interface_cock_finalize(FeatureInterfaceHandle handle)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
}

FtString interface_test_Chicken_interface_cock_get_name(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "%s", "gugu");
    return buf;
}

void interface_test_Chicken_interface_cock_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name)
{
    FEATURE_LOG_INFO("%s, interface: %p, set cock name: %s", file_tag, handle, name);
}

FtInt interface_test_Chicken_interface_cock_get_legCount(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p, cock leg count: %d", file_tag, handle, 2);
    return 2;
}

FtInt interface_test_Chicken_interface_cock_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray* foods)
{
    FtArrayHelper<const char*> string_array(foods);
    FEATURE_LOG_INFO("%s, interface: %p, cock eat food, array_size: %d", file_tag, handle, string_array.size());
    printf("food array = [\n");
    for (int i = 0; i < string_array.size(); i++) {
        printf("  index %d: %s\n", i, string_array[i]);
    }
    printf("]\n");
    return string_array.size();
}

FtString interface_test_Chicken_interface_cock_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination)
{
    FEATURE_LOG_INFO("%s, interface: %p, cock run, distance: %d, destination: %s", file_tag, handle, distance, destination);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cock run with wings flip!");
    return buf;
}

FtArray* interface_test_Chicken_interface_cock_fly(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
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
    FEATURE_LOG_INFO("%s, interface: %p", file_tag, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "%s", "Plymouth Rock");
    return buf;
}

void interface_test_Chicken_interface_cock_set_breed(FeatureInterfaceHandle handle, AppendData data, FtString breed)
{
    FEATURE_LOG_INFO("%s, interface: %p, set cock breed: %s", file_tag, handle, breed);
}

FtInt interface_test_Chicken_interface_cock_get_weight(FeatureInterfaceHandle handle, AppendData data)
{
    FEATURE_LOG_INFO("%s, interface: %p, cock weight is: %d kg", file_tag, handle, 2);
    return 2;
}

void interface_test_Chicken_interface_cock_set_weight(FeatureInterfaceHandle handle, AppendData data, FtInt weight)
{
    FEATURE_LOG_INFO("%s, interface: %p, set cock weight to: %d kg", file_tag, handle, weight);
}

void interface_test_Chicken_interface_cock_walk(FeatureInterfaceHandle handle, AppendData data, FtPromiseId pid)
{
    FEATURE_LOG_INFO("%s, interface: %p, %s", file_tag, handle, "cock walk slowly");
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
    FEATURE_LOG_INFO("%s", file_tag);
}

void interface_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void interface_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle feature)
{
    FEATURE_LOG_INFO("%s, feature: %p", file_tag, feature);
}

void interface_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle feature)
{
    FEATURE_LOG_INFO("%s, feature: %p", file_tag, feature);
}

void interface_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void interface_test_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

// Function wrappers to be implemented
FeatureInterfaceHandle interface_test_wrap_createCat(FeatureInstanceHandle feature, AppendData data)
{
    static interface_test_Animal_interface_vtable cat_vtable = {
        .base = { .finalizer = (NativeFunc)_Interface_cat_finalize },
        .get_name = _Interface_cat_get_name,
        .set_name = _Interface_cat_set_name,
        .get_legCount = _Interface_cat_get_legCount,
        .eatFood = _Interface_cat_eatFood,
        .run = _Interface_cat_run,
    };

    void* cat_data = (void*)create_cat_data();
    FeatureInterfaceHandle handle = interface_test_Animal_create(feature, &cat_vtable, cat_data);
    FEATURE_LOG_INFO("%s, feature: %p, interface: %p", file_tag, feature, handle);
    return handle;
}

FeatureInterfaceHandle interface_test_wrap_createDog(FeatureInstanceHandle feature, AppendData data, FtInt type)
{
    FeatureInterfaceHandle handle = interface_test_createDog_instance(feature);
    FEATURE_LOG_INFO("%s, feature: %p, interface: %p", file_tag, feature, handle);
    // void* data = create_dog_data(type);
    // FeatureSetObjectData(handle, data);
    return handle;
}

FeatureInterfaceHandle interface_test_wrap_createPigeon(FeatureInstanceHandle feature, AppendData data)
{
    FeatureInterfaceHandle handle = interface_test_createPigeon_instance(feature);
    FEATURE_LOG_INFO("%s, feature: %p, interface: %p", file_tag, feature, handle);
    // void* data = create_pigeon_data();
    // FeatureSetObjectData(handle, data);
    return handle;
}

FeatureInterfaceHandle interface_test_wrap_createCock(FeatureInstanceHandle feature, AppendData data)
{
    FeatureInterfaceHandle handle = interface_test_createCock_instance(feature);
    FEATURE_LOG_INFO("%s, feature: %p, interface: %p", file_tag, feature, handle);
    // void* data = create_cock_data();
    // FeatureSetObjectData(handle, data);
    return handle;
}

void interface_test_wrap_setAnimal(FeatureInstanceHandle feature, AppendData data, FeatureInstanceHandle animal)
{
    FEATURE_LOG_INFO("%s, feature: %p, animal: %p", file_tag, feature, animal);
}

void interface_test_wrap_flyFar(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt distance)
{
    FEATURE_LOG_INFO("%s, feature: %p, distance: %d", file_tag, feature, distance);
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
