#include "backend/qjs/feature_context_qjs.h"

static inline int get_ref_count(feature_value_t val)
{
    if (JS_VALUE_HAS_REF_COUNT(val)) {
        JSRefCountHeader* p = (JSRefCountHeader*)JS_VALUE_GET_PTR(val);
        return p->ref_count;
    }
    return -1;
}

namespace feature_framework {

// TEST for qjsContext
class FeatureContextTest : public ::testing::Test {
protected:
    ft_context_ref ft_test_ctx;
    struct feature_env_t {
        JSRuntime* rt;
        JSContext* ctx;
    };
    feature_env_t js_env;
    void SetUp() override
    {
        js_env.rt = JS_NewRuntime();
        js_env.ctx = JS_NewContext(js_env.rt);
        ft_test_ctx = CreateFeatureContextQjs(js_env.ctx);
    }

    void TearDown() override
    {
        ReleaseFeatureContextQjs(ft_test_ctx);
        JS_FreeContext(js_env.ctx);
        JS_FreeRuntime(js_env.rt);
    }
};

// =============================================================================
// ft_new_object Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_new_object_1)
{
    ft_value_t obj = ft_new_object(ft_test_ctx);
    EXPECT_TRUE(feature_is_object(FT_VAL_GET_JS_VAL(obj)));
    ft_free_value(ft_test_ctx, obj);
}

// =============================================================================
// ft_undefined Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_undefined_1)
{
    // 获取 undefined 值
    ft_value_t undefined_val = ft_undefined(ft_test_ctx);

    // 确保返回的值不为 nullptr，且是 undefined 类型
    EXPECT_EQ(JS_IsUndefined(FT_VAL_GET_JS_VAL(undefined_val)), true);

    // 清理
    ft_free_value(ft_test_ctx, undefined_val);
}

// =============================================================================
// ft_context_get_data Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_context_get_data_1)
{
    // 获取上下文数据
    void* data = ft_context_get_data(ft_test_ctx);

    // 检查数据是否有效
    EXPECT_NE(data, nullptr);

    EXPECT_EQ((JSContext*)data, js_env.ctx);
}

// =============================================================================
// ft_get_type Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_get_type_1)
{
    // 创建一个整数类型的值
    ft_value_t val = ft_from_int(ft_test_ctx, 42);

    // 获取类型
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保类型是number
    EXPECT_EQ(type, FT_TYPE_NUMBER);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_get_type_bool)
{
    // 获取FT_TYPE_BOOL类型
    // 创建一个bool类型的值
    ft_value_t val = ft_from_bool(ft_test_ctx, false);

    // 获取类型
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保类型是bool
    EXPECT_EQ(type, FT_TYPE_BOOL);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_get_type_string)
{
    // 获取FT_TYPE_STRING类型
    // 创建一个string类型的值
    ft_value_t val = ft_from_string(ft_test_ctx, "hello");

    // 获取类型
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保类型是string
    EXPECT_EQ(type, FT_TYPE_STRING);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_get_type_undefined)
{
    // 获取 FT_TYPE_UNDEF 类型
    ft_value_t val = ft_undefined(ft_test_ctx);

    // 获取类型
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保是 undefined 类型
    EXPECT_EQ(type, FT_TYPE_UNDEF);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_get_type_buffer)
{
    // 获取 FT_TYPE_BUFFER 类型
    size_t buff_size = 16;
    unsigned char* out_buff = (unsigned char*)alloca(buff_size);
    memset(out_buff, 0, buff_size);
    for (size_t i = 0; i < buff_size; ++i) {
        out_buff[i] = i;
    }
    ft_value_t val = ft_from_buffer(ft_test_ctx, out_buff, buff_size);

    // 获取类型
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保是 BUFFER 类型
    EXPECT_EQ(type, FT_TYPE_BUFFER);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_get_type_object)
{
    // 获取 FT_TYPE_OBJECT 类型
    ft_value_t val = ft_new_object(ft_test_ctx);

    // 获取类型
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保是 OBJECT 类型
    EXPECT_EQ(type, FT_TYPE_OBJECT);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_get_type_array)
{
    //获取 FT_TYPE_ARRAY 类型
    int32_t arr[] = { 1, 2, 3, 4, 5 };
    uint32_t size = sizeof(arr) / sizeof(arr[0]);

    ft_value_t val = ft_from_int_array(ft_test_ctx, arr, size);

    // 获取类型
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保是 ARRAY 类型
    EXPECT_EQ(type, FT_TYPE_ARRAY);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_int Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_from_int_1)
{
    // 从整数创建 ft_value_t
    ft_value_t val = ft_from_int(ft_test_ctx, 42);

    // 检查值是否正确
    int32_t result;
    EXPECT_TRUE(feature_to_int(js_env.ctx, &result, FT_VAL_GET_JS_VAL(val)));
    EXPECT_EQ(result, 42);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_int64 Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_from_int64_1)
{
    // 从 int64_t 创建 ft_value_t
    ft_value_t val = ft_from_int64(ft_test_ctx, 1234567890123456);

    // 检查值是否正确
    int64_t result;
    EXPECT_TRUE(feature_to_int64(js_env.ctx, &result, FT_VAL_GET_JS_VAL(val)));
    EXPECT_EQ(result, 1234567890123456);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_uint64 Tests (need CONFIG_BIGNUM)
// =============================================================================
// TEST_F(FeatureContextTest, ft_from_uint64_1) {
//     // 从 uint64_t 创建 ft_value_t
//     ft_value_t val = ft_from_uint64(ft_test_ctx, UINT64_MAX);

//     // 检查值是否正确
//     uint64_t result;
//     EXPECT_TRUE(feature_to_uint64(js_env.ctx, &result, FT_VAL_GET_JS_VAL(val)));
//     EXPECT_EQ(result, UINT64_MAX);

//     // 清理
//     ft_free_value(ft_test_ctx, val);
// }

// =============================================================================
// ft_from_double Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_from_double_1)
{
    // 从 double 创建 ft_value_t
    ft_value_t val = ft_from_double(ft_test_ctx, 42.42);

    // 检查值是否正确
    double result;
    EXPECT_TRUE(feature_to_double(js_env.ctx, &result, FT_VAL_GET_JS_VAL(val)));
    EXPECT_EQ(result, 42.42);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_bool Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_from_bool_1)
{
    // 从 bool 创建 ft_value_t
    ft_value_t val = ft_from_bool(ft_test_ctx, true);

    // 检查值是否正确
    bool result = false;
    EXPECT_TRUE(feature_to_boolean(js_env.ctx, &result, FT_VAL_GET_JS_VAL(val)));
    EXPECT_TRUE(result);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_string Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_from_string_1)
{
    // 从字符串创建 ft_value_t
    const char* test_str = "hello world";
    ft_value_t val = ft_from_string(ft_test_ctx, test_str);

    // 检查值是否正确
    const char* result = feature_to_cstring(js_env.ctx, FT_VAL_GET_JS_VAL(val));
    EXPECT_STREQ(result, test_str);

    // 清理
    feature_free_cstring(js_env.ctx, result);
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_from_string_EmptyString)
{
    //边界情况：空字符串
    const char* input = "";
    ft_value_t val = ft_from_string(ft_test_ctx, input);

    // 检查值是否正确
    const char* result = feature_to_cstring(js_env.ctx, FT_VAL_GET_JS_VAL(val));
    EXPECT_STREQ(result, input);

    // 清理
    feature_free_cstring(js_env.ctx, result);
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_buffer Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_from_buffer_1)
{
    // 创建一个字节缓冲区
    uint8_t buffer[] = { 1, 2, 3, 4 };
    ft_value_t val = ft_from_buffer(ft_test_ctx, buffer, sizeof(buffer));

    // 检查是否可以从缓冲区转换

    size_t result_size;
    uint8_t* result_buffer = feature_to_arraybuffer(js_env.ctx, &result_size, FT_VAL_GET_JS_VAL(val));
    EXPECT_EQ(result_size, sizeof(buffer));
    EXPECT_EQ(memcmp(result_buffer, buffer, result_size), 0);
    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_from_buffer_null)
{
    // 边界情况：缓冲区为空
    uint8_t buffer[] = {};
    ft_value_t val = ft_from_buffer(ft_test_ctx, buffer, sizeof(buffer));

    // 检查是否可以从缓冲区转换

    size_t result_size;
    uint8_t* result_buffer = feature_to_arraybuffer(js_env.ctx, &result_size, FT_VAL_GET_JS_VAL(val));
    EXPECT_EQ(result_size, sizeof(buffer));
    EXPECT_EQ(memcmp(result_buffer, buffer, result_size), 0);
    // 清理
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_typed_buffer Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_from_typed_buffer_1)
{
    // 创建一个字节缓冲区
    uint8_t buffer[] = { 1, 2, 3, 4 };
    ft_value_t val = ft_from_typed_buffer(ft_test_ctx, buffer, sizeof(buffer), FT_Uint8Array);
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保类型是buffer
    EXPECT_EQ(type, FT_TYPE_TYPED_BUFFER);
    size_t offset;
    size_t length;
    size_t byte_per_elem;
    // first get buffer ptr from a typedArray
    feature_value_t array_buffer = JS_GetTypedArrayBuffer(js_env.ctx, FT_VAL_GET_JS_VAL(val), &offset, &length, &byte_per_elem);
    EXPECT_TRUE(!feature_is_exception(array_buffer));
    EXPECT_EQ(byte_per_elem, sizeof(uint8_t));
    EXPECT_EQ(offset, 0U);
    EXPECT_EQ(length, sizeof(buffer) / sizeof(buffer[0]));

    size_t result_size;
    uint8_t* result_buffer = feature_to_arraybuffer(js_env.ctx, &result_size, array_buffer);
    EXPECT_EQ(result_size, sizeof(buffer));
    EXPECT_EQ(memcmp(result_buffer, buffer, result_size), 0);

    feature_free_value(js_env.ctx, array_buffer);
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_from_typed_buffer_null)
{
    // 边界情况：缓冲区为空
    uint8_t buffer[] = {};
    ft_value_t val = ft_from_typed_buffer(ft_test_ctx, buffer, sizeof(buffer), FT_Uint8Array);
    ft_type type = ft_get_type(ft_test_ctx, val);

    // 确保类型是buffer
    EXPECT_EQ(type, FT_TYPE_TYPED_BUFFER);
    size_t offset;
    size_t length;
    size_t byte_per_elem;
    // first get buffer ptr from a typedArray
    feature_value_t array_buffer = JS_GetTypedArrayBuffer(js_env.ctx, FT_VAL_GET_JS_VAL(val), &offset, &length, &byte_per_elem);
    EXPECT_TRUE(!feature_is_exception(array_buffer));
    EXPECT_EQ(byte_per_elem, sizeof(uint8_t));
    EXPECT_EQ(offset, 0U);
    EXPECT_EQ(length, sizeof(buffer) / sizeof(buffer[0]));

    size_t result_size;
    uint8_t* result_buffer = feature_to_arraybuffer(js_env.ctx, &result_size, array_buffer);
    EXPECT_EQ(result_size, sizeof(buffer));
    EXPECT_EQ(memcmp(result_buffer, buffer, result_size), 0);

    feature_free_value(js_env.ctx, array_buffer);
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_from_int_array
// =============================================================================
TEST_F(FeatureContextTest, ft_from_int_array_1)
{
    // 输入数据：一个整数数组
    int32_t val[] = { 1, 2, 3, 4, 5 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    // 调用函数
    ft_value_t result = ft_from_int_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    for (uint32_t i = 0; i < size; ++i) {
        feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
        int32_t ret;
        EXPECT_TRUE(feature_to_int(js_env.ctx, &ret, tmp));
        feature_free_value(js_env.ctx, tmp);
        EXPECT_EQ(ret, val[i]);
    }
    ft_free_value(ft_test_ctx, result);
}

TEST_F(FeatureContextTest, ft_from_int_array_null)
{
    // 边界情况：整数数组为空
    int32_t val[] = {};
    uint32_t size = 0;

    // 调用函数
    ft_value_t result = ft_from_int_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    ft_free_value(ft_test_ctx, result);
}

// =============================================================================
// ft_from_uint_array
// =============================================================================
TEST_F(FeatureContextTest, ft_from_uint_array_1)
{
    // 输入数据：一个无符号整数数组
    uint32_t val[] = { 1, 2, 3, 4, 5 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    // 调用函数
    ft_value_t result = ft_from_uint_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    for (uint32_t i = 0; i < size; ++i) {
        feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
        uint32_t ret;
        EXPECT_TRUE(feature_to_uint(js_env.ctx, &ret, tmp));
        feature_free_value(js_env.ctx, tmp);
        EXPECT_EQ(ret, val[i]);
    }
    ft_free_value(ft_test_ctx, result);
}

TEST_F(FeatureContextTest, ft_from_uint_array_null)
{
    // 边界情况：整数数组为空
    uint32_t val[] = {};
    uint32_t size = 0;

    // 调用函数
    ft_value_t result = ft_from_uint_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    ft_free_value(ft_test_ctx, result);
}

// =============================================================================
// ft_from_int64_array
// =============================================================================
TEST_F(FeatureContextTest, ft_from_int64_array_1)
{
    // 输入数据：一个 64 位整数数组
    int64_t val[] = { 10000000000, 20000000000, 30000000000, 40000000000, 50000000000 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    // 调用函数
    ft_value_t result = ft_from_int64_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    for (uint32_t i = 0; i < size; ++i) {
        feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
        int64_t ret;
        EXPECT_TRUE(feature_to_int64(js_env.ctx, &ret, tmp));
        feature_free_value(js_env.ctx, tmp);
        EXPECT_EQ(ret, val[i]);
    }
    ft_free_value(ft_test_ctx, result);
}

TEST_F(FeatureContextTest, ft_from_int64_array_null)
{
    // 边界情况：整数数组为空
    int64_t val[] = {};
    uint32_t size = 0;

    // 调用函数
    ft_value_t result = ft_from_int64_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    ft_free_value(ft_test_ctx, result);
}

// =============================================================================
// ft_from_uint64_array
// =============================================================================
// TEST_F(FeatureContextTest, ft_from_uint64_array_1)
// {
//     // 输入数据：一个无符号 64 位整数数组
//     uint64_t val[] = {10000000000, 20000000000, 30000000000, 40000000000, 50000000000};
//     uint32_t size = sizeof(val) / sizeof(val[0]);

//     // 调用函数
//     ft_value_t result = ft_from_uint64_array(ft_test_ctx, val, size);

//     // 验证 result 是否符合预期
//     EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
//     for (uint32_t i = 0; i < size; ++i) {
//         feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
//         uint64_t ret;
//         EXPECT_TRUE(feature_to_uint64(js_env.ctx, &ret, tmp));
//         feature_free_value(js_env.ctx, tmp);
//         EXPECT_EQ(ret, val[i]);
//     }
//     ft_free_value(ft_test_ctx, result);
// }
// =============================================================================
// ft_from_bool_array
// =============================================================================
TEST_F(FeatureContextTest, ft_from_bool_array_1)
{
    // 输入数据：一个布尔数组
    bool val[] = { true, false, true, false, true };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    // 调用函数
    ft_value_t result = ft_from_bool_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    for (uint32_t i = 0; i < size; ++i) {
        feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
        bool ret = false;
        EXPECT_TRUE(feature_to_boolean(js_env.ctx, &ret, tmp));
        feature_free_value(js_env.ctx, tmp);
        EXPECT_EQ(ret, val[i]);
    }
    ft_free_value(ft_test_ctx, result);
}

TEST_F(FeatureContextTest, ft_from_bool_array_null)
{
    //边界情况：数组为空
    // 输入数据：数组为空
    bool val[] = {};
    uint32_t size = 0;

    // 调用函数
    ft_value_t result = ft_from_bool_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    ft_free_value(ft_test_ctx, result);
}
// =============================================================================
// ft_from_double_array
// =============================================================================
TEST_F(FeatureContextTest, ft_from_double_array_1)
{
    // 输入数据：一个双精度浮点数数组
    double val[] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    // 调用函数
    ft_value_t result = ft_from_double_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    for (uint32_t i = 0; i < size; ++i) {
        feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
        double ret;
        EXPECT_TRUE(feature_to_double(js_env.ctx, &ret, tmp));
        feature_free_value(js_env.ctx, tmp);
        EXPECT_EQ(ret, val[i]);
    }
    ft_free_value(ft_test_ctx, result);
}

TEST_F(FeatureContextTest, ft_from_double_array_null)
{
    // 输入数据：数组为空
    double val[] = {};
    uint32_t size = 0;

    // 调用函数
    ft_value_t result = ft_from_double_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    ft_free_value(ft_test_ctx, result);
}
// =============================================================================
// ft_from_string_array
// =============================================================================
TEST_F(FeatureContextTest, ft_from_string_array_1)
{
    // 输入数据：一个字符串数组
    const char* val[] = { "Hello", "World", "Foo", "Bar", "Baz" };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    // 调用函数
    ft_value_t result = ft_from_string_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    for (uint32_t i = 0; i < size; ++i) {
        feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
        const char* ret = feature_to_cstring(js_env.ctx, tmp);
        EXPECT_STREQ(ret, val[i]);
        feature_free_cstring(js_env.ctx, ret);
        feature_free_value(js_env.ctx, tmp);
    }
    ft_free_value(ft_test_ctx, result);
}

TEST_F(FeatureContextTest, ft_from_string_array_null)
{
    //边界情况：字符数组为空
    // 输入数据：一个字符串数组
    const char* val[] = {};
    uint32_t size = 0;

    // 调用函数
    ft_value_t result = ft_from_string_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    ft_free_value(ft_test_ctx, result);
}

TEST_F(FeatureContextTest, ft_from_string_array_strEmpty)
{
    //边界情况：字符数组包含空字符串
    // 输入数据：一个字符串数组
    const char* val[] = { "", "World", "Foo", "Bar", "Baz" };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    // 调用函数
    ft_value_t result = ft_from_string_array(ft_test_ctx, val, size);

    // 验证 result 是否符合预期
    EXPECT_EQ(feature_get_array_length(js_env.ctx, FT_VAL_GET_JS_VAL(result)), size);
    for (uint32_t i = 0; i < size; ++i) {
        feature_value_t tmp = feature_get_array_idx_safe(js_env.ctx, FT_VAL_GET_JS_VAL(result), i);
        const char* ret = feature_to_cstring(js_env.ctx, tmp);
        EXPECT_STREQ(ret, val[i]);
        feature_free_cstring(js_env.ctx, ret);
        feature_free_value(js_env.ctx, tmp);
    }
    ft_free_value(ft_test_ctx, result);
}

// ft_parse_json
// =============================================================================
// ft_to_int Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_to_int_1)
{
    ft_value_t val = ft_from_int(ft_test_ctx, 42);
    int32_t int_val;
    EXPECT_TRUE(ft_to_int(ft_test_ctx, val, &int_val));
    EXPECT_EQ(int_val, 42);

    // 清理
    ft_free_value(ft_test_ctx, val);
}
// =============================================================================
// ft_to_uint Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_to_uint_1)
{
    ft_value_t val = ft_from_uint(ft_test_ctx, 42U);
    uint32_t uint_val;
    EXPECT_TRUE(ft_to_uint(ft_test_ctx, val, &uint_val));
    EXPECT_EQ(uint_val, 42U);

    // 清理
    ft_free_value(ft_test_ctx, val);
}
// =============================================================================
// ft_to_int64 Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_to_int64_1)
{
    ft_value_t val = ft_from_int64(ft_test_ctx, 1234567890L);
    int64_t int64_val;
    EXPECT_TRUE(ft_to_int64(ft_test_ctx, val, &int64_val));
    EXPECT_EQ(int64_val, 1234567890L);

    // 清理
    ft_free_value(ft_test_ctx, val);
}
// =============================================================================
// ft_to_uint64 Tests
// =============================================================================
// TEST_F(FeatureContextTest, ft_to_uint64_1)
// {
//     ft_value_t val = ft_from_uint64(ft_test_ctx, 1234567890123456789U);
//     uint64_t uint64_val;
//     EXPECT_TRUE(ft_to_uint64(ft_test_ctx, val, &uint64_val));
//     EXPECT_EQ(uint64_val, 1234567890123456789U);

//     // 清理
//     ft_free_value(ft_test_ctx, val);
// }
// =============================================================================
// ft_to_double Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_to_double_1)
{
    ft_value_t val = ft_from_double(ft_test_ctx, 3.14159);
    double double_val;
    EXPECT_TRUE(ft_to_double(ft_test_ctx, val, &double_val));
    EXPECT_NEAR(double_val, 3.14159, 1e-5);

    // 清理
    ft_free_value(ft_test_ctx, val);
}
// =============================================================================
// ft_to_bool Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_to_bool_1)
{
    ft_value_t val = ft_from_bool(ft_test_ctx, true);
    bool bool_val;
    EXPECT_TRUE(ft_to_bool(ft_test_ctx, val, &bool_val));
    EXPECT_TRUE(bool_val);

    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_to_bool_2)
{
    ft_value_t val = ft_from_bool(ft_test_ctx, false);
    bool bool_val;
    EXPECT_TRUE(ft_to_bool(ft_test_ctx, val, &bool_val));
    EXPECT_FALSE(bool_val);

    // 清理
    ft_free_value(ft_test_ctx, val);
}
// =============================================================================
// ft_to_string Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_to_string_1)
{
    const char* str_val = "Hello, World!";
    ft_value_t val = ft_from_string(ft_test_ctx, str_val);
    const char* result_str;
    result_str = ft_to_string(ft_test_ctx, val);
    EXPECT_STREQ(result_str, str_val);

    // 清理
    ft_free_string(ft_test_ctx, result_str);
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_to_string_null)
{
    //边界情况：空字符串
    const char* str_val = "";
    ft_value_t val = ft_from_string(ft_test_ctx, str_val);
    const char* result_str;
    result_str = ft_to_string(ft_test_ctx, val);
    EXPECT_STREQ(result_str, str_val);

    // 清理
    ft_free_string(ft_test_ctx, result_str);
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_to_buffer
// =============================================================================
TEST_F(FeatureContextTest, ft_to_buffer_1)
{
    // 创建一个字节缓冲区
    uint8_t buffer[] = { 1, 2, 3, 4 };
    ft_value_t val = ft_from_buffer(ft_test_ctx, buffer, sizeof(buffer));

    // 检查是否可以从缓冲区转换

    size_t result_size;
    uint8_t* result_buffer = ft_to_buffer(ft_test_ctx, &result_size, val);
    EXPECT_EQ(result_size, sizeof(buffer));
    EXPECT_EQ(memcmp(result_buffer, buffer, result_size), 0);
    // 清理
    ft_free_value(ft_test_ctx, val);
}

TEST_F(FeatureContextTest, ft_to_buffer_null)
{
    // 边界情况：缓冲区为空
    uint8_t buffer[] = {};
    ft_value_t val = ft_from_buffer(ft_test_ctx, buffer, sizeof(buffer));

    // 检查是否可以从缓冲区转换

    size_t result_size;
    uint8_t* result_buffer = ft_to_buffer(ft_test_ctx, &result_size, val);
    EXPECT_EQ(result_size, sizeof(buffer));
    EXPECT_EQ(memcmp(result_buffer, buffer, result_size), 0);
    // 清理
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_array_size
// =============================================================================
TEST_F(FeatureContextTest, ft_array_size_1)
{
    // 输入数据：一个整数数组
    int32_t val[] = { 1, 2, 3, 4, 5 };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_int_array(ft_test_ctx, val, size);

    // 检查数组大小
    uint32_t array_size = ft_array_size(ft_test_ctx, array);
    EXPECT_EQ(array_size, size);
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_size_null)
{
    // 边界情况：数组为空
    int32_t val[] = {};
    uint32_t size = 0;
    ft_value_t array = ft_from_int_array(ft_test_ctx, val, size);

    // 检查数组大小
    uint32_t array_size = ft_array_size(ft_test_ctx, array);
    EXPECT_EQ(array_size, size);
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_size_int64)
{
    // int64数组
    int64_t val[] = { 10000000000, 20000000000, 30000000000, 40000000000, 50000000000 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    ft_value_t array = ft_from_int64_array(ft_test_ctx, val, size);

    // 检查数组大小
    uint32_t array_size = ft_array_size(ft_test_ctx, array);
    EXPECT_EQ(array_size, size);
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_size_uint64)
{
    // 无符号整数数组
    uint32_t val[] = { 1, 2, 3, 4, 5 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    ft_value_t array = ft_from_uint_array(ft_test_ctx, val, size);

    // 检查数组大小
    uint32_t array_size = ft_array_size(ft_test_ctx, array);
    EXPECT_EQ(array_size, size);
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_size_double)
{
    // double数组
    double val[] = { 1.11, 2.22, 3.33, 4.44, 5.55 };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_double_array(ft_test_ctx, val, size);

    // 检查数组大小
    uint32_t array_size = ft_array_size(ft_test_ctx, array);
    EXPECT_EQ(array_size, size);
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_size_bool)
{
    // bool数组
    bool val[] = { true, false, true, false, true };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_bool_array(ft_test_ctx, val, size);

    // 检查数组大小
    uint32_t array_size = ft_array_size(ft_test_ctx, array);
    EXPECT_EQ(array_size, size);
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_size_string)
{
    // string数组
    const char* val[] = { "Hello", "World", "Foo", "Bar", "Baz" };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_string_array(ft_test_ctx, val, size);

    // 检查数组大小
    uint32_t array_size = ft_array_size(ft_test_ctx, array);
    EXPECT_EQ(array_size, size);
    ft_free_value(ft_test_ctx, array);
}

// =============================================================================
// ft_array_at
// =============================================================================
TEST_F(FeatureContextTest, ft_array_at_1)
{
    // 输入数据：一个整数数组
    int32_t val[] = { 1, 2, 3, 4, 5 };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_int_array(ft_test_ctx, val, size);

    // 检查数组元素
    for (uint32_t i = 0; i < size; i++) {
        ft_value_t element = ft_array_at(ft_test_ctx, array, i);
        int32_t element_val;
        EXPECT_TRUE(ft_to_int(ft_test_ctx, element, &element_val));
        EXPECT_EQ(element_val, val[i]);
        ft_free_value(ft_test_ctx, element);
    }

    // 清理
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_at_int64)
{
    // int64数组
    int64_t val[] = { 10000000000, 20000000000, 30000000000, 40000000000, 50000000000 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    ft_value_t array = ft_from_int64_array(ft_test_ctx, val, size);

    // 检查数组元素
    for (uint32_t i = 0; i < size; i++) {
        ft_value_t element = ft_array_at(ft_test_ctx, array, i);
        int64_t element_val;
        EXPECT_TRUE(ft_to_int64(ft_test_ctx, element, &element_val));
        EXPECT_EQ(element_val, val[i]);
        ft_free_value(ft_test_ctx, element);
    }

    // 清理
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_at_uint64)
{
    // 无符号整数数组
    uint32_t val[] = { 1, 2, 3, 4, 5 };
    uint32_t size = sizeof(val) / sizeof(val[0]);

    ft_value_t array = ft_from_uint_array(ft_test_ctx, val, size);

    // 检查数组元素
    for (uint32_t i = 0; i < size; i++) {
        ft_value_t element = ft_array_at(ft_test_ctx, array, i);
        uint32_t element_val;
        EXPECT_TRUE(ft_to_uint(ft_test_ctx, element, &element_val));
        EXPECT_EQ(element_val, val[i]);
        ft_free_value(ft_test_ctx, element);
    }

    // 清理
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_at_double)
{
    // double数组
    double val[] = { 1.11, 2.22, 3.33, 4.44, 5.55 };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_double_array(ft_test_ctx, val, size);

    // 检查数组元素
    for (uint32_t i = 0; i < size; i++) {
        ft_value_t element = ft_array_at(ft_test_ctx, array, i);
        double element_val;
        EXPECT_TRUE(ft_to_double(ft_test_ctx, element, &element_val));
        EXPECT_EQ(element_val, val[i]);
        ft_free_value(ft_test_ctx, element);
    }

    // 清理
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_at_bool)
{
    // bool数组
    bool val[] = { true, false, true, false, true };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_bool_array(ft_test_ctx, val, size);

    // 检查数组元素
    for (uint32_t i = 0; i < size; i++) {
        ft_value_t element = ft_array_at(ft_test_ctx, array, i);
        bool element_val;
        EXPECT_TRUE(ft_to_bool(ft_test_ctx, element, &element_val));
        EXPECT_EQ(element_val, val[i]);
        ft_free_value(ft_test_ctx, element);
    }

    // 清理
    ft_free_value(ft_test_ctx, array);
}

TEST_F(FeatureContextTest, ft_array_at_string)
{
    // string数组
    const char* val[] = { "Hello", "World", "Foo", "Bar", "Baz" };
    uint32_t size = sizeof(val) / sizeof(val[0]);
    ft_value_t array = ft_from_string_array(ft_test_ctx, val, size);

    // 检查数组元素
    for (uint32_t i = 0; i < size; i++) {
        const ft_value_t element = ft_array_at(ft_test_ctx, array, i);
        const char* element_val = ft_to_string(ft_test_ctx, element);
        EXPECT_STREQ(element_val, val[i]);
        ft_free_string(ft_test_ctx, element_val);
        ft_free_value(ft_test_ctx, element);
    }

    // 清理
    ft_free_value(ft_test_ctx, array);
}

// =============================================================================
// ft_free_value
// =============================================================================
TEST_F(FeatureContextTest, ft_free_value_1)
{
    // 创建一个整数值
    ft_value_t obj = ft_new_object(ft_test_ctx);
    feature_dup_value(js_env.ctx, FT_VAL_GET_JS_VAL(obj));

    EXPECT_EQ(get_ref_count(FT_VAL_GET_JS_VAL(obj)), 2);
    ft_free_value(ft_test_ctx, obj);
    EXPECT_EQ(get_ref_count(FT_VAL_GET_JS_VAL(obj)), 1);
    ft_free_value(ft_test_ctx, obj);
}

// =============================================================================
// ft_free_string
// =============================================================================
TEST_F(FeatureContextTest, ft_free_string_1)
{
    // 创建一个字符串
    const char* str = "Hello, World!";
    ft_value_t val = ft_from_string(ft_test_ctx, str);
    const char* result_str;
    result_str = ft_to_string(ft_test_ctx, val);
    EXPECT_EQ(get_ref_count(FT_VAL_GET_JS_VAL(val)), 2);
    ft_free_string(ft_test_ctx, result_str);
    EXPECT_EQ(get_ref_count(FT_VAL_GET_JS_VAL(val)), 1);
    ft_free_value(ft_test_ctx, val);
}

// =============================================================================
// ft_obj_set_property Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_obj_set_property_1)
{
    // 创建对象
    ft_value_t obj = ft_new_object(ft_test_ctx);

    // 设置属性
    const char* prop_name = "property1";
    ft_value_t prop_value = ft_from_int(ft_test_ctx, 123);
    bool success = ft_obj_set_property(ft_test_ctx, obj, prop_name, prop_value);
    // 确保属性设置成功
    EXPECT_TRUE(success);
    // 获取属性并验证
    feature_value_t result = feature_get_object_property(js_env.ctx, FT_VAL_GET_JS_VAL(obj), prop_name);
    int32_t result_value;
    EXPECT_TRUE(feature_to_int(js_env.ctx, &result_value, result));
    EXPECT_EQ(result_value, 123);

    // 清理
    feature_free_value(js_env.ctx, result);
    ft_free_value(ft_test_ctx, obj);
}

TEST_F(FeatureContextTest, ft_obj_set_property_null)
{
    // 边界情况：空字符串
    ft_value_t obj = ft_new_object(ft_test_ctx);

    // 设置属性
    const char* prop_name = "";
    ft_value_t prop_value = ft_from_int(ft_test_ctx, 123);
    bool success = ft_obj_set_property(ft_test_ctx, obj, prop_name, prop_value);
    // 确保属性设置成功
    EXPECT_TRUE(success);
    // 获取属性并验证
    feature_value_t result = feature_get_object_property(js_env.ctx, FT_VAL_GET_JS_VAL(obj), prop_name);
    int32_t result_value;
    EXPECT_TRUE(feature_to_int(js_env.ctx, &result_value, result));
    EXPECT_EQ(result_value, 123);

    // 清理
    feature_free_value(js_env.ctx, result);
    ft_free_value(ft_test_ctx, obj);
}

// =============================================================================
// ft_obj_get_property Tests
// =============================================================================
TEST_F(FeatureContextTest, ft_obj_get_property_1)
{
    // 创建对象并设置属性
    ft_value_t obj = ft_new_object(ft_test_ctx);
    const char* prop_name = "name";
    ft_value_t prop_value = ft_from_string(ft_test_ctx, "test_value");
    ft_obj_set_property(ft_test_ctx, obj, prop_name, prop_value);

    // 获取属性值
    ft_value_t result = ft_obj_get_property(ft_test_ctx, obj, prop_name);

    // 确保获取到的属性值等于设置的值
    EXPECT_NE(JS_IsUndefined(FT_VAL_GET_JS_VAL(result)), true);
    const char* result_str = ft_to_string(ft_test_ctx, result);
    EXPECT_STREQ(result_str, "test_value");

    // 清理
    ft_free_value(ft_test_ctx, result);
    ft_free_string(ft_test_ctx, result_str);
    ft_free_value(ft_test_ctx, obj);
}

} // feature_framework_test