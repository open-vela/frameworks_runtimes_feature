#include "feature_qjs_exports.h"

namespace feature_framework {

class FeatureQJSContextTest : public ::testing::Test {
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
// ft_from_jsvalue Tests
// =============================================================================
TEST_F(FeatureQJSContextTest, ft_from_jsvalue_1)
{
    JSValue obj = JS_NewObject(js_env.ctx);
    JS_SetPropertyStr(js_env.ctx, obj, "data", JS_NewInt32(js_env.ctx, 1));
    ft_value_t val = ft_from_jsvalue(ft_test_ctx, obj);
    EXPECT_EQ(ft_get_type(ft_test_ctx, val), FT_TYPE_OBJECT);
    ft_value_t result = ft_obj_get_property(ft_test_ctx, val, "data");
    EXPECT_EQ(ft_get_type(ft_test_ctx, result), FT_TYPE_NUMBER);
    int32_t result_int;
    EXPECT_TRUE(ft_to_int(ft_test_ctx, result, &result_int));
    EXPECT_EQ(result_int, 1);
    ft_free_value(ft_test_ctx, result);
    JS_FreeValue(js_env.ctx, obj);
}

TEST_F(FeatureQJSContextTest, ft_from_jsvalue_null)
{
    ft_value_t val = ft_from_jsvalue(ft_test_ctx, JS_NULL);
    EXPECT_EQ(ft_get_type(ft_test_ctx, val), FT_TYPE_NULL);
}

TEST_F(FeatureQJSContextTest, ft_from_jsvalue_undefined)
{
    ft_value_t val = ft_from_jsvalue(ft_test_ctx, JS_UNDEFINED);
    EXPECT_EQ(ft_get_type(ft_test_ctx, val), FT_TYPE_UNDEF);
}

// =============================================================================
// ft_to_jsvalue Tests
// =============================================================================
TEST_F(FeatureQJSContextTest, ft_to_jsvalue1)
{
    ft_value_t obj = ft_new_object(ft_test_ctx);
    ft_obj_set_property(ft_test_ctx, obj, "data", ft_from_int(ft_test_ctx, 1));
    JSValue js_obj = ft_to_jsvalue(ft_test_ctx, obj);
    EXPECT_TRUE(JS_IsObject(js_obj));
    JSValue data = JS_GetPropertyStr(js_env.ctx, js_obj, "data");
    EXPECT_TRUE(JS_IsNumber(data));
    int32_t pres;
    JS_ToInt32(js_env.ctx, &pres, data);
    EXPECT_EQ(pres, 1);
    JS_FreeValue(js_env.ctx, data);
    ft_free_value(ft_test_ctx, obj);
}

TEST_F(FeatureQJSContextTest, ft_to_jsvalue_undefined)
{
    ft_value_t val = ft_undefined(ft_test_ctx);
    JSValue js_val = ft_to_jsvalue(ft_test_ctx, val);
    EXPECT_NE(JS_IsUndefined(js_val), true);
}

}