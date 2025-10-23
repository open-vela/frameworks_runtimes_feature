#include "unit_interface_test.h"

#include "unit_jidl_util.h"

typedef struct interface_test_animal_data {
    int leg_count;
    FtString name;
    int breed;
    int weight;
} interface_test_animal_data;

void* create_animal_data(FtString name)
{
    interface_test_animal_data* data = (interface_test_animal_data*)malloc(sizeof(interface_test_animal_data));
    data->leg_count = 4;
    data->name = (char*)FeatureMalloc(64, FT_STRING);
    strcpy((char*)data->name, name);
    data->breed = 0;
    data->weight = 0;
    return data;
}

void destory_animal_data(FeatureInterfaceHandle handle)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    if (data) {
        if (data->name) {
            FeatureFreeValue((char*)data->name);
        }
        free(data);
    }
    FeatureSetObjectData(handle, NULL);
}

static void unit_interface_cat_finalize(FeatureInterfaceHandle handle)
{
    destory_animal_data(handle);
}

static FtString unit_interface_cat_get_name(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    FeatureDupValue((void*)data->name);
    return data->name;
}

static void unit_interface_cat_set_name(FeatureInterfaceHandle handle, AppendData append_data, FtString name)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    memset((char*)data->name, 0, 64);
    strcpy((char*)data->name, name);
}

static FtInt unit_interface_cat_get_legCount(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    return data->leg_count;
}

static FtBool unit_interface_cat_eatFood(FeatureInterfaceHandle handle, AppendData append_data, FtArray* foods)
{
    if (!foods) {
        return false;
    }
    FTArrayHelper<const char*> arrayHelper(foods);
    for (int i = 0; i < arrayHelper.size(); i++) {
        if (strcmp(arrayHelper[i], "food"))
            return false;
    }
    return true;
}

static FtBool unit_interface_cat_run(FeatureInterfaceHandle handle, AppendData append_data, FtInt distance, FtString destination)
{
    return distance == 100 && strcmp(destination, "home") == 0;
}

void unit_interface_test_onRegister(const char* feature_name)
{
    return;
}
void unit_interface_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}
void unit_interface_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    auto* name = (char*)malloc(64);
    memset(name, 0, 64);
    FeatureSetObjectData(handle, (void*)name);
    return;
}
void unit_interface_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    auto* name = (char*)FeatureGetObjectData(handle);
    free(name);
    FeatureSetObjectData(handle, nullptr);
    return;
}
void unit_interface_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}
void unit_interface_test_onUnregister(const char* feature_name)
{
    return;
}

FeatureInterfaceHandle unit_interface_test_wrap_createDog(FeatureInstanceHandle feature, AppendData append_data, FtInt type)
{
    // test for null ret
    if (type == 0) {
        return nullptr;
    } else if (type == -1) {
        FeatureThrowError(feature, "createDog error");
        return nullptr;
    }
    FeatureInterfaceHandle handle = unit_interface_test_createDog_instance(feature);
    void* data = create_animal_data("dog");
    FeatureSetObjectData(handle, data);
    return handle;
}

FeatureInterfaceHandle unit_interface_test_wrap_createPigeon(FeatureInstanceHandle feature, AppendData append_data)
{
    FeatureInterfaceHandle handle = unit_interface_test_createPigeon_instance(feature);
    void* data = create_animal_data("pigeon");
    FeatureSetObjectData(handle, data);
    return handle;
}

FeatureInterfaceHandle unit_interface_test_wrap_createCock(FeatureInstanceHandle feature, AppendData append_data)
{
    FeatureInterfaceHandle handle = unit_interface_test_createCock_instance(feature);
    void* data = create_animal_data("cock");
    FeatureSetObjectData(handle, data);
    return handle;
}
FeatureInterfaceHandle unit_interface_test_wrap_createCat(FeatureInstanceHandle feature, AppendData append_data)
{
    static unit_interface_test_Animal_interface_vtable cat_vtable = {
        .base = { .finalizer = (NativeFunc)unit_interface_cat_finalize },
        .get_name = unit_interface_cat_get_name,
        .set_name = unit_interface_cat_set_name,
        .get_legCount = unit_interface_cat_get_legCount,
        .eatFood = unit_interface_cat_eatFood,
        .run = unit_interface_cat_run,
    };
    void* data = create_animal_data("cat");
    FeatureInterfaceHandle handle = unit_interface_test_Animal_create(feature, &cat_vtable, data);
    return handle;
}

void unit_interface_test_wrap_setAnimal(FeatureInstanceHandle feature, AppendData append_data, FeatureInterfaceHandle animal)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(animal);
    char* instance_name = (char*)FeatureGetObjectData(feature);
    memset(instance_name, 0, 64);
    strcpy((char*)instance_name, data->name);
}

FtBool unit_interface_test_wrap_testAnimal(FeatureInstanceHandle feature, AppendData append_data, FtString name)
{
    char* instance_name = (char*)FeatureGetObjectData(feature);
    return strcmp(instance_name, name) == 0;
}

void unit_interface_test_Animal_interface_dog_finalize(FeatureInterfaceHandle handle)
{
    destory_animal_data(handle);
}
FtString unit_interface_test_Animal_interface_dog_get_name(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    FeatureDupValue((void*)data->name);
    return data->name;
}
void unit_interface_test_Animal_interface_dog_set_name(FeatureInterfaceHandle handle, AppendData append_data, FtString name)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    memset((char*)data->name, 0, 64);
    strcpy((char*)data->name, name);
}
FtInt unit_interface_test_Animal_interface_dog_get_legCount(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    return data->leg_count;
}

FtBool unit_interface_test_Animal_interface_dog_eatFood(FeatureInterfaceHandle handle, AppendData append_data, FtArray* foods)
{
    if (!foods) {
        return false;
    }
    FTArrayHelper<const char*> arrayHelper(foods);
    for (int i = 0; i < arrayHelper.size(); i++) {
        if (strcmp(arrayHelper[i], "food"))
            return false;
    }
    return true;
}
FtBool unit_interface_test_Animal_interface_dog_run(FeatureInterfaceHandle handle, AppendData append_data, FtInt distance, FtString destination)
{
    return distance == 100 && strcmp(destination, "home") == 0;
}

// vtable functions for interface constructor function 'createPigeon'
void unit_interface_test_Bird_interface_pigeon_finalize(FeatureInterfaceHandle handle)
{
    destory_animal_data(handle);
}
FtArray* unit_interface_test_Bird_interface_pigeon_fly(FeatureInterfaceHandle handle, AppendData append_data)
{
    FtArray* ret = FeatureCreateArray(handle, 2, FT_STRING);
    FeatureArrayAppendRaw(ret, "fly");
    FeatureArrayAppendRaw(ret, "away");
    return ret;
}
FtInt unit_interface_test_Bird_interface_pigeon_get_breed(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    return data->breed;
}
void unit_interface_test_Bird_interface_pigeon_set_breed(FeatureInterfaceHandle handle, AppendData append_data, FtInt breed)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    data->breed = breed;
}

// vtable functions for interface constructor function 'createCock'
void unit_interface_test_Chicken_interface_cock_finalize(FeatureInterfaceHandle handle)
{
    destory_animal_data(handle);
}
FtString unit_interface_test_Chicken_interface_cock_get_name(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    FeatureDupValue((void*)data->name);
    return data->name;
}
void unit_interface_test_Chicken_interface_cock_set_name(FeatureInterfaceHandle handle, AppendData append_data, FtString name)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    memset((char*)data->name, 0, 64);
    strcpy((char*)data->name, name);
}
FtInt unit_interface_test_Chicken_interface_cock_get_legCount(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    return data->leg_count;
}

FtBool unit_interface_test_Chicken_interface_cock_eatFood(FeatureInterfaceHandle handle, AppendData append_data, FtArray* foods)
{
    if (!foods) {
        return false;
    }
    FTArrayHelper<const char*> arrayHelper(foods);
    for (int i = 0; i < arrayHelper.size(); i++) {
        if (strcmp(arrayHelper[i], "food"))
            return false;
    }
    return true;
}
FtBool unit_interface_test_Chicken_interface_cock_run(FeatureInterfaceHandle handle, AppendData append_data, FtInt distance, FtString destination)
{
    return distance == 100 && strcmp(destination, "home") == 0;
}
FtArray* unit_interface_test_Chicken_interface_cock_fly(FeatureInterfaceHandle handle, AppendData append_data)
{
    FtArray* ret = FeatureCreateArray(handle, 2, FT_STRING);
    FeatureArrayAppendRaw(ret, "fly");
    FeatureArrayAppendRaw(ret, "away");
    return ret;
}
FtInt unit_interface_test_Chicken_interface_cock_get_breed(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    return data->breed;
}
void unit_interface_test_Chicken_interface_cock_set_breed(FeatureInterfaceHandle handle, AppendData append_data, FtInt breed)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    data->breed = breed;
}
FtInt unit_interface_test_Chicken_interface_cock_get_weight(FeatureInterfaceHandle handle, AppendData append_data)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    return data->weight;
}
void unit_interface_test_Chicken_interface_cock_set_weight(FeatureInterfaceHandle handle, AppendData append_data, FtInt weight)
{
    interface_test_animal_data* data = (interface_test_animal_data*)FeatureGetObjectData(handle);
    data->weight = weight;
}
void unit_interface_test_Chicken_interface_cock_walk(FeatureInterfaceHandle handle, AppendData append_data, FtPromiseId pid)
{
    FtArray* ret = FeatureCreateArray(handle, 2, FT_STRING);
    FeatureArrayAppendRaw(ret, "walk");
    FeatureArrayAppendRaw(ret, "away");
    FeaturePromiseResolve(handle, pid, ret);
    FeatureFreeValue(ret);
    return;
}
