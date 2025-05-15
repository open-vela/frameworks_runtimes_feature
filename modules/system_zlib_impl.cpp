#include "system_zlib.h"

#include "zlib.h"
#include <vector>

#define TAG "[zlib_impl]"

#define ZLIB_LOG_DEBUG(fmt, ...) \
    FEATURE_LOG_DEBUG(TAG fmt, ##__VA_ARGS__)

#define ZLIB_LOG_INFO(fmt, ...) FEATURE_LOG_INFO(TAG fmt, ##__VA_ARGS__)

#define ZLIB_LOG_ERROR(fmt, ...) \
    FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

static uint8_t* GetDataBuff(ft_context_ref ft_ctx, ft_value_t data, size_t* size)
{
    ft_type type = ft_get_type(ft_ctx, data);
    if (type != FT_TYPE_TYPED_BUFFER) {
        ZLIB_LOG_ERROR("::%s() get data failed", __FUNCTION__);
        return nullptr;
    }
    uint8_t* buff = ft_to_buffer(ft_ctx, size, data);
    ZLIB_LOG_INFO(" get buffer, size: %ld", *size);
    return buff;
}

void system_zlib_onRegister(const char* feature_name)
{
    ZLIB_LOG_DEBUG("::%s() register feature: %s", __FUNCTION__, feature_name);
}
void system_zlib_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    ZLIB_LOG_DEBUG("::%s() create feature", __FUNCTION__);
}
void system_zlib_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    ZLIB_LOG_DEBUG("::%s() required feature", __FUNCTION__);
}
void system_zlib_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    ZLIB_LOG_DEBUG("::%s() detached feature", __FUNCTION__);
}
void system_zlib_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    ZLIB_LOG_DEBUG("::%s() destroy feature", __FUNCTION__);
}
void system_zlib_onUnregister(const char* feature_name)
{
    ZLIB_LOG_DEBUG("::%s() unregister feature: %s", __FUNCTION__, feature_name);
}

FtAny system_zlib_wrap_decompressSync(FeatureInstanceHandle feature, AppendData append_data, FtAny data)
{
    if (nullptr == data)
        return nullptr;
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    size_t size = 0;
    uint8_t* buff = GetDataBuff(ft_ctx, *data, &size);
    if (nullptr == buff)
        return nullptr;

    std::vector<uint8_t> output_vec;
    constexpr size_t CHUNK_SIZE = 16384;
    output_vec.resize(CHUNK_SIZE);

    int ret;
    z_stream d_stream = { 0 };
    d_stream.zalloc = Z_NULL;
    d_stream.zfree = Z_NULL;
    d_stream.opaque = Z_NULL;
    d_stream.next_in = (unsigned char*)buff;
    d_stream.avail_in = size;

    // inflat init
    // TODO: 优化写法
    const int window_bits_options[] = { MAX_WBITS, -MAX_WBITS, MAX_WBITS + 16 };
    bool succ = true;
    for (size_t i = 0; i < sizeof(window_bits_options) / sizeof(int); i++) {
        if (inflateInit2(&d_stream, window_bits_options[i]) != Z_OK)
            continue;
        // inflate decompress
        do {
            size_t remain = output_vec.size() - d_stream.total_out;
            if (remain < CHUNK_SIZE) {
                output_vec.resize(output_vec.size() + CHUNK_SIZE);
                remain = CHUNK_SIZE;
            }

            d_stream.avail_out = remain;
            d_stream.next_out = output_vec.data() + d_stream.total_out;

            ret = inflate(&d_stream, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                ZLIB_LOG_ERROR("Zlib mode: %d error: %s", i, zError(ret));
                inflateEnd(&d_stream);
                succ = false;
                break;
            }

        } while (ret != Z_STREAM_END);

        if (succ) {
            output_vec.resize(d_stream.total_out);

            // inflate end
            inflateEnd(&d_stream);
            FtAny out = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
            *out = ft_from_typed_buffer(ft_ctx, (uint8_t*)output_vec.data(),
                output_vec.size(), FT_Uint8Array);
            return out;
        }
    }
    ZLIB_LOG_ERROR("Failed to decompress, unknown error.");
    return nullptr;
}