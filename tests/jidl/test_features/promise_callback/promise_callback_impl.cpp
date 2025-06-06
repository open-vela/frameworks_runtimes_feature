// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "feature_main_exports.h"
#include "promise_callback.h"

static const char* file_tag = "[jidl_feature] promise_callback_impl";

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
void promise_callback_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void promise_callback_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented
void promise_callback_wrap_foo_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a)
{
    bool resolve = a;
    printf("%s::%s(), a: %d, resolve: %d\n", file_tag, __FUNCTION__, a, resolve);
    if (resolve) {
        FeaturePromiseResolve(feature, pid, a);
    } else {
        FeaturePromiseReject(feature, pid, 202, "foo rejected");
    }
}

void promise_callback_wrap_bar_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a)
{
    bool resolve = a;
    printf("%s::%s(), a: %d, resolve: %d\n", file_tag, __FUNCTION__, a, resolve);
    if (resolve) {
        FeaturePromiseResolve(feature, pid, "bar resolved");
    } else {
        FeaturePromiseReject(feature, pid, 202, "bar rejected");
    }
}

void promise_callback_wrap_void_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a)
{
    bool resolve = a;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (resolve) {
        FeaturePromiseResolve(feature, pid);
    } else {
        FeaturePromiseReject(feature, pid, 200, "void_cb rejected");
    }
}

void promise_callback_wrap_goo_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a)
{
    bool resolve = a;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (resolve) {
        FtArray* array = promise_callback_malloc_int_array();
        array->_size = 3;
        array->_element = malloc(sizeof(int) * array->_size);
        FTArrayHelper<int> int_array(array);
        for (int32_t i = 0; i < int_array.size(); i++) {
            int_array[i] = i;
        }
        FeaturePromiseResolve(feature, pid, array);
        FeatureFreeValue(array);
    } else {
        FeaturePromiseReject(feature, pid, 201, "goo_cb rejected");
    }
}

void promise_callback_wrap_moo_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a)
{
    bool resolve = a;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (resolve) {
        FtArray* array = promise_callback_malloc_string_array();
        array->_size = 3;
        array->_element = malloc(sizeof(char*) * array->_size);
        FTArrayHelper<char*> str_array(array);
        for (int32_t i = 0; i < str_array.size(); i++) {
            char* str = (char*)FeatureMalloc(8, FT_STRING);
            sprintf(str, "str_%" PRIi32, i);
            str_array[i] = str;
        }
        FeaturePromiseResolve(feature, pid, array);
        FeatureFreeValue(array);
    } else {
        FeaturePromiseReject(feature, pid, 201, "moo_cb rejected");
    }
}

void promise_callback_wrap_obj_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a, promise_callback_Chapter* chap)
{
    bool resolve = a;
    printf("%s::%s(), chap: %p, resolve: %d\n", file_tag, __FUNCTION__, chap, resolve);
    if (!chap) {
        FeaturePromiseReject(feature, pid, 202, "obj_cb rejected");
        return;
    }
    printf("%s::%s(), page_count: %d, title: %s, is_end: %d\n", file_tag, __FUNCTION__, chap->page_count, chap->title, chap->is_end);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    if (resolve) {
        ft_value_t page_count = ft_from_int(ft_ctx, 50);
        ft_value_t title = ft_from_string(ft_ctx, "hello");
        ft_value_t is_end = ft_from_bool(ft_ctx, false);
        ft_value_t chap_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, chap_obj, "page_count", page_count);
        ft_obj_set_property(ft_ctx, chap_obj, "title", title);
        ft_obj_set_property(ft_ctx, chap_obj, "is_end", is_end);
        FeaturePromiseResolve(feature, pid, &chap_obj);
        ft_free_value(ft_ctx, chap_obj);
    } else {
        FeaturePromiseReject(feature, pid, 202, "obj_cb rejected");
    }
}

void promise_callback_wrap_struct_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a)
{
    bool resolve = a;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (resolve) {
        promise_callback_Chapter* chap = promise_callbackMallocChapter();
        chap->page_count = 150;
        char* title = (char*)FeatureMalloc(8, FT_STRING);
        sprintf(title, "%s", "world");
        chap->title = title;
        chap->is_end = true;
        FeaturePromiseResolve(feature, pid, chap);
        FeatureFreeValue(chap);
    } else {
        FeaturePromiseReject(feature, pid, 202, "struct_cb rejected");
    }
}

static promise_callback_Chapter* make_chapter(int idx)
{
    promise_callback_Chapter* chap = promise_callbackMallocChapter();
    chap->page_count = (idx + 1) * 100;
    char* title = (char*)FeatureMalloc(8, FT_STRING);
    sprintf(title, "chap_%d", idx);
    chap->title = title;
    chap->is_end = idx % 2;
    return chap;
}

void promise_callback_wrap_struct_array_cb(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, FtBool a)
{
    bool resolve = a;
    printf("%s::%s(), resolve: %d\n", file_tag, __FUNCTION__, resolve);
    if (resolve) {
        FtArray* array = promise_callback_malloc_Chapter_struct_type_array();
        array->_size = 4;
        array->_element = malloc(sizeof(promise_callback_Chapter*) * array->_size);
        FTArrayHelper<promise_callback_Chapter*> chap_array(array);
        for (int32_t i = 0; i < chap_array.size(); i++) {
            chap_array[i] = make_chapter(i);
        }
        FeaturePromiseResolve(feature, pid, array);
        FeatureFreeValue(array);
    } else {
        FeaturePromiseReject(feature, pid, 202, "struct_array_cb rejected");
    }
}
