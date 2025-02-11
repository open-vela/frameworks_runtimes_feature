#include "feature_description.h"
#include "feature_exports.h"
#include "feature_main_exports.h"
#include "feature_manager.h"

namespace feature_framework {
FtString test_const1 = "hello world";

static const MemberMethod unit_test_method1 = {
    .func_stub = nullptr,
    .parameters = nullptr,
    .return_type = FT_VOID,
};

static const MemberConst unit_test_require1 = {
    .type = FT_STRING,
    .func = { .callback = nullptr },
    .data = { .str = test_const1 }
};

static const Member testMembers1[] = {
    {
        .type = MEMBER_METHOD,
        .name = "unitTest",
        .method = &unit_test_method1,
    },
    {
        .type = MEMBER_CONST,
        .name = "constTest",
        .value = &unit_test_require1,
    }
};

static const FeatureDescription pDesc1 = {
    .version = 1,
    .name = "unit_test",
    .description = "unit_test",
    .dynamic = false,
    .native_callbacks = nullptr,
    .member_count = 1,
    .members = testMembers1,
};

static bool test_args_error_cb(void* data, ArgsErrorInfo* error_info)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s: runtime context is null!", __func__);
        return false;
    }
    if (!error_info) {
        FEATURE_LOG_ERROR("%s: error_info is null!", __func__);
        return false;
    }
    return true;
}

static char* FTStringCopy(const char* str)
{
    char* ret = (char*)FeatureMalloc(strlen(str) + 1, FT_STRING);
    sprintf(ret, "%s", str);
    return ret;
}

class FeatureMainExportTestQjs : public ::testing::Test {
protected:
    FeatureManagerHandle manager_handle_qjs;
    struct feature_env_t {
        JSRuntime* rt;
        JSContext* ctx;
    };
    feature_env_t js_env;
    uv_loop_t* loop;
    void SetUp() override
    {
        js_env.rt = JS_NewRuntime();
        js_env.ctx = JS_NewContext(js_env.rt);
        FeatureManagerCreateInfo ft_info;
        ft_info.raw_ctx = (FeatureRawContextHandle)(js_env.ctx);
        ft_info.release_cb = nullptr;
        ft_info.manager_type = FEATURE_MANAGER_JS;
        ft_info.package_name = "com.feature.test";
        manager_handle_qjs = FeatureCreateManager(&ft_info);
        loop = uv_default_loop();
        FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
        FeatureRegisterFeature(manager->getFeatureRegistry(), &pDesc1);
    }

    void TearDown() override
    {
        FeatureUnsetUVLoop(manager_handle_qjs);
        int closed = 0;
        for (int i = 0; i < 200; i++) {
            if (uv_loop_close(loop) == 0) {
                closed = 1;
                break;
            }
            uv_run(loop, UV_RUN_NOWAIT);
        }

        if (!closed) {
            FILE* fp = fopen("/dev/log", "wb");
            fp = fp ? fp : stderr;
            uv_print_all_handles(loop, fp);
            if (fp != stderr) {
                fclose(fp);
            }
            // assert directly if we can't stop uv loop successfuly.
            assert(0);
        }
        FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
        if (manager->getFeatureContext()) {
            FeatureUninit(manager_handle_qjs);
        }
        FeatureFreeManager(manager_handle_qjs);
        JS_FreeContext(js_env.ctx);
        JS_FreeRuntime(js_env.rt);
    }
};

// =============================================================================
// FeatureCreateManager Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureCreateManager_pinfoIsNull)
{
    ASSERT_NE(manager_handle_qjs, nullptr);
    FeatureManagerHandle g_manager_qjs_test;
    g_manager_qjs_test = FeatureCreateManager(nullptr);
    ASSERT_EQ(g_manager_qjs_test, nullptr);
}
// =============================================================================
// FeatureManagerGetContext Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureManagerGetContext1)
{
    auto cxt_ref = FeatureManagerGetContext(manager_handle_qjs);
    EXPECT_EQ(cxt_ref->data, js_env.ctx);
}
// =============================================================================
// FeatureSetArgsErrorCb Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureSetArgsErrorCb1)
{
    FeatureSetArgsErrorCb(manager_handle_qjs, test_args_error_cb, js_env.ctx);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->argsErrorCb(), test_args_error_cb);
    EXPECT_EQ(static_cast<JSContext*>(manager->argsErrorData()), js_env.ctx);
    ArgsErrorInfo error_info;
    error_info.error_code = FT_ERR_ARGS;
    error_info.error_msg = "test";
    error_info.argc = 1;
    error_info.argv = nullptr;
    EXPECT_TRUE(manager->argsErrorCb()(manager->argsErrorData(), &error_info));
}
// =============================================================================
// FeatureSetPackageVersion Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureSetPackageVersion1)
{
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->packageVesion(), nullptr);
    FeatureSetPackageVersion(manager_handle_qjs, "3.14");
    EXPECT_EQ(strcmp(manager->packageVesion(), "3.14"), 0);
}
// =============================================================================
// FeatureFreeManager Tests
// =============================================================================
// =============================================================================
// FeatureSetUVLoop Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureSetUVLoop1)
{
    FeatureSetUVLoop(manager_handle_qjs, loop);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->getUVLoop(), loop);
}
// =============================================================================
// FeatureUnsetUVLoop Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureUnsetUVLoop1)
{
    FeatureSetUVLoop(manager_handle_qjs, loop);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->getUVLoop(), loop);
    FeatureUnsetUVLoop(manager_handle_qjs);
    EXPECT_EQ(manager->getUVLoop(), nullptr);
}
// =============================================================================
// FeatureUninit Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureUninit1)
{
    FeatureUninit(manager_handle_qjs);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->getFeatureContext(), nullptr);
    EXPECT_TRUE(feature_list_is_empty(manager->getFeatureNodeList()));
}
// =============================================================================
// FeatureRequire Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureRequire1)
{
    ft_value_t ft_value;
    FT_VAL_GET_JS_VAL(ft_value) = JS_UNDEFINED;
    auto ft_obj = FeatureRequire(manager_handle_qjs, ft_value, pDesc1.name);
    auto js_obj = FT_VAL_GET_JS_VAL(ft_obj);
    EXPECT_NE(js_obj, JS_UNDEFINED);
    JSValue unit_test = JS_GetPropertyStr(js_env.ctx, js_obj, "unitTest");
    EXPECT_NE(unit_test, JS_UNDEFINED);
    JS_FreeValue(js_env.ctx, unit_test);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), ft_obj);
}
// =============================================================================
// FeatureFindFeature Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureFindFeature1)
{
    ft_value_t ft_value;
    FT_VAL_GET_JS_VAL(ft_value) = JS_UNDEFINED;
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    auto ft_obj = FeatureFindFeature(manager_handle_qjs, pDesc1.name);
    auto js_obj = FT_VAL_GET_JS_VAL(ft_obj);
    EXPECT_NE(js_obj, JS_UNDEFINED);
    auto feature_pair = manager->getFeatureRegistry()->findFeature(pDesc1.name);
    auto& prototype = feature_pair->second;
    EXPECT_EQ(FT_VAL_GET_JS_VAL(((FeaturePrototypeQjs*)prototype)->ft_proto()), js_obj);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), ft_obj);
}
// =============================================================================
// FeatureSetManagerUserData Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureSetManagerUserData1)
{
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    char* str1 = (char*)malloc(6);
    strcpy(str1, "world");
    FeatureSetManagerUserData(manager_handle_qjs, "data", str);
    FeatureSetManagerUserData(manager_handle_qjs, "data1", str1);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->getUserData("data"), str);
    EXPECT_EQ(manager->getUserData("data1"), str1);
    FeatureSetManagerUserData(manager_handle_qjs, "data", nullptr);
    FeatureSetManagerUserData(manager_handle_qjs, "data1", nullptr);
    free(str);
    free(str1);
}
// =============================================================================
// FeatureCreateFeature Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureCreateFeature1)
{
    //测试能否正确创建feature
    auto js_feature_prototype = FeatureFindFeature(manager_handle_qjs, pDesc1.name);
    ft_value_t binding_obj;
    FT_VAL_GET_JS_VAL(binding_obj) = JS_UNDEFINED;
    auto new_feature = FeatureCreateFeature(manager_handle_qjs, js_feature_prototype, binding_obj);
    auto js_obj = FT_VAL_GET_JS_VAL(new_feature);
    EXPECT_NE(js_obj, JS_UNDEFINED);
    JSValue unit_test = JS_GetPropertyStr(js_env.ctx, js_obj, "unitTest");
    EXPECT_NE(unit_test, JS_UNDEFINED);
    JS_FreeValue(js_env.ctx, unit_test);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), new_feature);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), js_feature_prototype);
}
// =============================================================================
// FeatureHasFeature Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureHasFeature1)
{
    auto name = FTStringCopy("unit_test");
    auto res = FeatureHasFeature(manager_handle_qjs, name);
    EXPECT_EQ(res, true);
    FeatureFreeValue(name);
    name = FTStringCopy("unit_test.unitTest");
    res = FeatureHasFeature(manager_handle_qjs, name);
    EXPECT_EQ(res, true);
    FeatureFreeValue(name);
}
} // namespace feature_framework