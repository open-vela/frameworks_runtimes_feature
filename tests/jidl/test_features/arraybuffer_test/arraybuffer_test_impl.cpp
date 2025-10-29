// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "arraybuffer_test.h"

static const char* file_tag = "[jidl_feature] arraybuffer_test_impl";

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

typedef struct _ArrayBufferInfo {
    FtArrayBuffer abuf;
} ArrayBufferInfo;

static void print_arraybuffer_data(uint8_t* data, size_t size)
{
    printf("[");
    for (size_t i = 0; i < size; ++i) {
        printf("%d ", data[i]);
    }
    printf("]\n");
}

static void consume_last_arraybuffer_data(FeatureInstanceHandle feature)
{
    ArrayBufferInfo* abuf_info = (ArrayBufferInfo*)FeatureGetObjectData(feature);
    if (!abuf_info) {
        FEATURE_LOG_INFO("%s, last arraybuffer not exist!", file_tag);
        return;
    }
    FEATURE_LOG_INFO("%s, last arraybuffer: %p", file_tag, abuf_info->abuf);
    size_t data_size;
    uint8_t* data = FeatureArrayBufferGetData(abuf_info->abuf, &data_size);
    if (data) {
        FEATURE_LOG_INFO("%s, last arraybuffer data: ", file_tag);
        print_arraybuffer_data(data, data_size);
    } else {
        FEATURE_LOG_ERROR("%s, last arraybuffer data is null!", file_tag);
    }
    FeatureFreeValue(abuf_info->abuf);
    free(abuf_info);
    FeatureSetObjectData(feature, NULL);
}

void arraybuffer_test_wrap_set_buffer(FeatureInstanceHandle feature, AppendData append_data, FtArrayBuffer buff)
{
    consume_last_arraybuffer_data(feature);
    FEATURE_LOG_INFO("%s, current arraybuffer: %p", file_tag, buff);
    size_t data_size;
    uint8_t* data = FeatureArrayBufferGetData(buff, &data_size);
    if (!data) {
        FEATURE_LOG_ERROR("%s, current arraybuffer data is null", file_tag);
        return;
    }
    FEATURE_LOG_INFO("%s, current arraybuffer data: ", file_tag);
    print_arraybuffer_data(data, data_size);

    ArrayBufferInfo* abuf_info = (ArrayBufferInfo*)malloc(sizeof(ArrayBufferInfo));
    abuf_info->abuf = (FtArrayBuffer)FeatureDupValue((void*)buff);
    FeatureSetObjectData(feature, abuf_info);
}

FtArrayBuffer arraybuffer_test_wrap_get_buffer_copy(FeatureInstanceHandle feature,
    AppendData data)
{
    uint8_t buf[6] = { 10, 9, 8, 7, 6, 5 };
    FtArrayBuffer ret = FeatureNewArrayBufferCopyData(feature, buf, 6);
    FEATURE_LOG_INFO("%s, attach success, arraybuffer: %p!", file_tag, ret);
    return ret;
}

typedef struct _BufferFreeInfo {
    uint8_t* data;
    size_t data_size;
} BufferFreeInfo;

static void free_buffer(void* opaque, void* buff)
{
    BufferFreeInfo* free_info = (BufferFreeInfo*)opaque;
    if (!free_info) {
        FEATURE_LOG_ERROR("%s, free_info is null", file_tag);
        return;
    }
    FEATURE_LOG_INFO("%s, free_info: %p, data: %p, buff: %p!", file_tag, free_info, free_info->data, buff);
    if (buff == free_info->data) {
        FEATURE_LOG_INFO("%s, free free_info and data!", file_tag);
        free(free_info->data);
        free(free_info);
    }
}

static FtArrayBuffer create_array_buffer(FeatureInstanceHandle feature, uint8_t* buff, int buff_size)
{
    uint8_t* data = (uint8_t*)malloc(sizeof(uint8_t) * buff_size);
    memcpy(data, buff, buff_size);
    BufferFreeInfo* free_info = (BufferFreeInfo*)malloc(sizeof(BufferFreeInfo));
    free_info->data = data;
    free_info->data_size = buff_size;
    FtArrayBuffer ret = FeatureNewArrayBufferFromData(feature, data, buff_size, free_buffer, free_info);
    FEATURE_LOG_INFO("%s, free_info: %p, data: %p!", file_tag, free_info, free_info->data);
    if (!ret) {
        FEATURE_LOG_ERROR("%s, new arraybuffer failed, arraybuffer: %p!", file_tag, ret);
        free(free_info->data);
        free(free_info);
        FeatureFreeValue(ret);
        return NULL;
    }
    FEATURE_LOG_INFO("%s, attach success, arraybuffer: %p!", file_tag, ret);
    return ret;
}

FtArrayBuffer arraybuffer_test_wrap_get_buffer_no_copy(FeatureInstanceHandle feature,
    AppendData adata)
{
    uint8_t buf[6] = { 5, 6, 7, 8, 9, 10 };
    return create_array_buffer(feature, buf, 6);
}

void arraybuffer_test_wrap_set_buffer_array(FeatureInstanceHandle feature, AppendData append_data, FtArray* buff_array)
{
    FEATURE_LOG_INFO("%s, buff_array: %p!", file_tag, buff_array);
    FTArrayHelper<FtArrayBuffer> ab_array(buff_array);
    FEATURE_LOG_INFO("%s, arraybuffer_array size: %d!", file_tag, ab_array.size());
    printf("arraybuffer_array: [\n");
    for (int32_t i = 0; i < ab_array.size(); i++) {
        FtArrayBuffer buff = ab_array[i];
        if (!buff) {
            FEATURE_LOG_ERROR("%s, null FtArrayBuffer element", file_tag);
            continue;
        }
        size_t data_size;
        uint8_t* data = FeatureArrayBufferGetData(buff, &data_size);
        if (!data) {
            FEATURE_LOG_ERROR("%s, current arraybuffer data is null", file_tag);
            continue;
        }
        print_arraybuffer_data(data, data_size);
    }
    printf("]\n");
}

FtArray* arraybuffer_test_wrap_get_buffer_array_copy(FeatureInstanceHandle feature,
    AppendData data)
{
    FtArray* abArray = arraybuffer_test_malloc_arraybuffer_array();
    abArray->_size = 2;
    abArray->_element = malloc(sizeof(FtArrayBuffer) * 2);
    uint8_t buf1[6] = { 10, 9, 8, 7, 6, 5 };
    FtArrayBuffer elem1 = FeatureNewArrayBufferCopyData(feature, buf1, 6);
    ((FtArrayBuffer*)abArray->_element)[0] = elem1;
    uint8_t buf2[4] = { 4, 3, 2, 1 };
    FtArrayBuffer elem2 = FeatureNewArrayBufferCopyData(feature, buf2, 4);
    ((FtArrayBuffer*)abArray->_element)[1] = elem2;
    return abArray;
}

FtArray* arraybuffer_test_wrap_get_buffer_array_no_copy(FeatureInstanceHandle feature,
    AppendData adata)
{
    FtArray* abArray = arraybuffer_test_malloc_arraybuffer_array();
    abArray->_size = 3;
    abArray->_element = malloc(sizeof(FtArrayBuffer) * 3);
    uint8_t buf1[1] = { 1 };
    FtArrayBuffer elem0 = create_array_buffer(feature, buf1, 1);
    ((FtArrayBuffer*)abArray->_element)[0] = elem0;
    uint8_t buf2[2] = { 2, 2 };
    FtArrayBuffer elem1 = create_array_buffer(feature, buf2, 2);
    ((FtArrayBuffer*)abArray->_element)[1] = elem1;
    uint8_t buf3[3] = { 3, 3, 3 };
    FtArrayBuffer elem2 = create_array_buffer(feature, buf3, 3);
    ((FtArrayBuffer*)abArray->_element)[2] = elem2;
    return abArray;
}

// FeatureCallbacks to be implemented
void arraybuffer_test_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void arraybuffer_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void arraybuffer_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void arraybuffer_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
    consume_last_arraybuffer_data(handle);
}

void arraybuffer_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void arraybuffer_test_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}
