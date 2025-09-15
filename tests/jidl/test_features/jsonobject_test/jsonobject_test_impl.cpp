// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "jsonobject_test.h"

static const char* const file_tag = "[jidl_feature] jsonobject_test_impl";

// FeatureCallbacks to be implemented
void jsonobject_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void jsonobject_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void jsonobject_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void jsonobject_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void jsonobject_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void jsonobject_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

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

// Function wrappers to be implemented
void jsonobject_test_wrap_set_data(FeatureInstanceHandle feature, AppendData append_data, FtJsonObject data)
{
    if (!data) {
        printf("%s::%s(), jsonobject data is null\n", file_tag, __FUNCTION__);
        return;
    }
    const char* str = FeatureGetJsonString(data);
    printf("%s::%s(), jsonobject string: %s\n", file_tag, __FUNCTION__, str ? str : "null");
}

static const char g_chapter_json[] = "{\"chapter\": {\"title\": \"chap one\", \"page_count\": 50}}";
static const char g_book_json[] = "{\"book\": {\"name\": \"monkey king\", \"chap_count\": 30, \"page_count\": 500}}";

FtJsonObject jsonobject_test_wrap_get_data(FeatureInstanceHandle feature, AppendData append_data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    FtJsonObject json_obj = FeatureNewJsonObject(g_book_json);
    return json_obj;
}

void jsonobject_test_wrap_set_data_array(FeatureInstanceHandle feature, AppendData append_data, FtArray* data_array)
{
    if (!data_array) {
        printf("%s::%s(), data_array ptr is null\n", file_tag, __FUNCTION__);
        return;
    }
    FTArrayHelper<FtJsonObject> darray(data_array);
    printf("%s::%s(), data ptr: %p, darray size: %" PRIi32 "\n", file_tag, __FUNCTION__, data_array, darray.size());
    if (darray.size() <= 0) {
        return;
    }
    printf("darray = [\n");
    for (int32_t i = 0; i < darray.size(); i++) {
        const char* str = FeatureGetJsonString(darray[i]);
        printf("  index %" PRIi32 ": %s\n", i, str ? str : "null");
    }
    printf("]\n");
}

FtArray* jsonobject_test_wrap_get_data_array(FeatureInstanceHandle feature, AppendData append_data)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    FtArray* json_array = jsonobject_test_malloc_jsonobject_array();
    json_array->_size = 5;
    json_array->_element = malloc(sizeof(FtJsonObject) * 5);
    for (int i = 0; i < 5; i++) {
        char json_str[64] = { 0 };
        sprintf(json_str, "{\"hello\": %d}", i);
        FtJsonObject json_obj = FeatureNewJsonObject(json_str);
        ((FtJsonObject*)json_array->_element)[i] = json_obj;
    }
    return json_array;
}

void jsonobject_test_wrap_set_book(FeatureInstanceHandle feature, AppendData append_data, jsonobject_test_Book* book)
{
    if (!book) {
        printf("%s::%s(), book ptr is null\n", file_tag, __FUNCTION__);
        return;
    }
    const char* meta_str = FeatureGetJsonString(book->meta);
    printf("%s::%s(), meta: %s, title: %s, chapters: %p\n",
        file_tag, __FUNCTION__, meta_str ? meta_str : "null", book->title, book->chapters);
    if (!book->chapters) {
        return;
    }
    FTArrayHelper<FtJsonObject> chap_array(book->chapters);
    printf("%s::%s(), chap_array size: %" PRIi32 "\n", file_tag, __FUNCTION__, chap_array.size());
    if (chap_array.size() <= 0) {
        return;
    }
    printf("chap_array = [\n");
    for (int32_t i = 0; i < chap_array.size(); i++) {
        const char* json_str = FeatureGetJsonString(chap_array[i]);
        printf("  index %" PRIi32 ": %s\n", i, json_str ? json_str : "null");
    }
    printf("]\n");
}

void jsonobject_test_wrap_json_promise(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtInt val)
{
    int resolve = val % 2;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (resolve) {
        FtJsonObject json_obj = FeatureNewJsonObject(g_chapter_json);
        FeaturePromiseResolve(feature, pid, json_obj);
        FeatureFreeValue(json_obj);
    } else {
        FeaturePromiseReject(feature, pid, 202, "no json object");
    }
}

void jsonobject_test_wrap_json_array_promise(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtInt val)
{
    int resolve = val % 2;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (resolve) {
        FtArray* json_array = jsonobject_test_malloc_jsonobject_array();
        json_array->_size = 3;
        json_array->_element = malloc(sizeof(FtJsonObject) * 3);
        for (int i = 0; i < 3; i++) {
            char json_str[64] = { 0 };
            sprintf(json_str, "{\"hello\": %d}", i);
            FtJsonObject json_obj = FeatureNewJsonObject(json_str);
            ((FtJsonObject*)json_array->_element)[i] = json_obj;
        }
        FeaturePromiseResolve(feature, pid, json_array);
        FeatureFreeValue(json_array);
    } else {
        FeaturePromiseReject(feature, pid, 202, "no json object array");
    }
}
