#include "unit_jsonobject_test.h"

#include "unit_jidl_util.h"

static const char test_json_str[] = "{\"test\":1}";

void unit_jsonobject_test_onRegister(const char* feature_name)
{
    return;
}

void unit_jsonobject_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}

void unit_jsonobject_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}

void unit_jsonobject_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}

void unit_jsonobject_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}

void unit_jsonobject_test_onUnregister(const char* feature_name)
{
    return;
}

FtBool unit_jsonobject_test_wrap_set_data(FeatureInstanceHandle feature, AppendData append_data, FtJsonObject data)
{
    if (!data) {
        return false;
    }
    const char* json_data = FeatureGetJsonString(data);
    if (strcmp(test_json_str, json_data) == 0) {
        return true;
    }
    return false;
}

FtJsonObject unit_jsonobject_test_wrap_get_data(FeatureInstanceHandle feature, AppendData append_data)
{
    FtJsonObject json_obj = FeatureNewJsonObject(test_json_str);
    return json_obj;
}

FtBool unit_jsonobject_test_wrap_set_data_array(FeatureInstanceHandle feature, AppendData append_data, FtArray* data_array)
{
    if (!data_array) {
        return false;
    }
    FTArrayHelper<FtJsonObject> arrayHelper(data_array);
    for (int i = 0; i < arrayHelper.size(); i++) {
        FtJsonObject json_obj = arrayHelper[i];
        if (!json_obj) {
            return false;
        }
        const char* json_data = FeatureGetJsonString(json_obj);
        if (strcmp(test_json_str, json_data) != 0) {
            return false;
        }
    }
    return true;
}

FtArray* unit_jsonobject_test_wrap_get_data_array(FeatureInstanceHandle feature, AppendData append_data)
{
    FtArray* array = FeatureCreateArray(feature, 2, FT_JSON_OBJ);
    FtJsonObject json_obj1 = FeatureNewJsonObject("1");
    FtJsonObject json_obj2 = FeatureNewJsonObject(test_json_str);
    FeatureArrayAppend(array, json_obj1);
    FeatureArrayAppend(array, json_obj2);
    return array;
}

FtBool unit_jsonobject_test_wrap_set_book(FeatureInstanceHandle feature, AppendData append_data, unit_jsonobject_test_Book* book)
{
    if (!book) {
        return false;
    }
    const char* meta = FeatureGetJsonString(book->meta);
    if (strcmp(test_json_str, meta) != 0) {
        return false;
    }
    FTArrayHelper<FtJsonObject> arrayHelper(book->chapters);
    for (int i = 0; i < arrayHelper.size(); i++) {
        FtJsonObject json_obj = arrayHelper[i];
        if (!json_obj) {
            return false;
        }
        const char* json_data = FeatureGetJsonString(json_obj);
        if (strcmp(test_json_str, json_data) != 0) {
            return false;
        }
    }
    return true;
}

void unit_jsonobject_test_wrap_json_promise(FeatureInstanceHandle feature, AppendData append_data,
    FtPromiseId pid, unit_jsonobject_test_Chapter* chapter)
{
    FtJsonObject json_obj = FeatureNewJsonObject(test_json_str);
    FeaturePromiseResolve(feature, pid, json_obj);
    FeatureFreeValue(json_obj);
    return;
}

void unit_jsonobject_test_wrap_json_array_promise(FeatureInstanceHandle feature, AppendData append_data,
    FtPromiseId pid, unit_jsonobject_test_Chapter* chapter)
{
    FtArray* array = FeatureCreateArray(feature, 2, FT_JSON_OBJ);
    FtJsonObject json_obj1 = FeatureNewJsonObject("1");
    FtJsonObject json_obj2 = FeatureNewJsonObject(test_json_str);
    FeatureArrayAppend(array, json_obj1);
    FeatureArrayAppend(array, json_obj2);
    FeaturePromiseResolve(feature, pid, array);
    FeatureFreeValue(array);
    return;
}
