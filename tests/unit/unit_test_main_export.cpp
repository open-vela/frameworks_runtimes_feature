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

static char* test_set_uri_convert_cb(const char* package_name, const char* uri)
{
    return strdup(uri);
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
TEST_F(FeatureMainExportTestQjs, FeatureManagerGetContext_handleIsNull)
{
    auto cxt_ref = FeatureManagerGetContext(nullptr);
    EXPECT_EQ(cxt_ref, nullptr);
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
TEST_F(FeatureMainExportTestQjs, FeatureSetArgsErrorCb_handleIsNull)
{
    // FeatureSetArgsErrorCb没有返回值，无法判断是否成功
    FeatureSetArgsErrorCb(nullptr, test_args_error_cb, js_env.ctx);
    EXPECT_TRUE(true);
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
TEST_F(FeatureMainExportTestQjs, FeatureSetPackageVersion_handleIsNull)
{
    FeatureSetPackageVersion(nullptr, "3.14");
}

// =============================================================================
// FeatureFreeManager Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureFreeManager1)
{
    FeatureManagerHandle handle = new FeatureManager(nullptr);
    // 测试无handle泄露
    FeatureFreeManager(handle);
}
TEST_F(FeatureMainExportTestQjs, FeatureFreeManager_handleIsNull)
{
    FeatureFreeManager(nullptr);
}

// =============================================================================
// FeatureSetUVLoop Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureSetUVLoop1)
{
    FeatureSetUVLoop(manager_handle_qjs, loop);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->getUVLoop(), loop);
}
TEST_F(FeatureMainExportTestQjs, FeatureSetUVLoop_loopIsNull)
{
    FeatureSetUVLoop(manager_handle_qjs, nullptr);
}
TEST_F(FeatureMainExportTestQjs, FeatureSetUVLoop_handleIsNull)
{
    FeatureSetUVLoop(nullptr, loop);
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
TEST_F(FeatureMainExportTestQjs, FeatureUnsetUVLoop_handleIsNull)
{
    FeatureUnsetUVLoop(nullptr);
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
TEST_F(FeatureMainExportTestQjs, FeatureUninit_handleIsNull)
{
    FeatureUninit(nullptr);
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
    EXPECT_NE(JS_IsUndefined(js_obj), true);
    JSValue unit_test = JS_GetPropertyStr(js_env.ctx, js_obj, "unitTest");
    EXPECT_NE(JS_IsUndefined(unit_test), true);
    JS_FreeValue(js_env.ctx, unit_test);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), ft_obj);
}
TEST_F(FeatureMainExportTestQjs, FeatureRequire_handleIsNull)
{
    ft_value_t ft_value;
    FT_VAL_GET_JS_VAL(ft_value) = JS_UNDEFINED;
    auto ft_obj = FeatureRequire(nullptr, ft_value, pDesc1.name);
    ft_value_t zero_value = {}; // 全零的结构体
    EXPECT_TRUE(memcmp(&ft_obj, &zero_value, sizeof(ft_value_t)) == 0);
}
TEST_F(FeatureMainExportTestQjs, FeatureRequire_nameIsNull)
{
    ft_value_t ft_value;
    FT_VAL_GET_JS_VAL(ft_value) = JS_UNDEFINED;
    auto ft_obj = FeatureRequire(manager_handle_qjs, ft_value, nullptr);
    ft_value_t zero_value = {}; // 全零的结构体
    EXPECT_TRUE(memcmp(&ft_obj, &zero_value, sizeof(ft_value_t)) == 0);
}
// =============================================================================
// FeatureFindFeature Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureFindFeature1)
{
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    auto ft_obj = FeatureFindFeature(manager_handle_qjs, pDesc1.name);
    auto js_obj = FT_VAL_GET_JS_VAL(ft_obj);
    EXPECT_NE(JS_IsUndefined(js_obj), true);
    auto pDesc2 = manager->getFeatureRegistry()->findFeature(pDesc1.name);
    EXPECT_EQ(pDesc2, &pDesc1);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), ft_obj);
}
TEST_F(FeatureMainExportTestQjs, FeatureFindFeature_handleIsNull)
{
    auto ft_obj = FeatureFindFeature(nullptr, pDesc1.name);
    ft_value_t zero_value = {}; // 全零的结构体
    EXPECT_TRUE(memcmp(&ft_obj, &zero_value, sizeof(ft_value_t)) == 0);
}
TEST_F(FeatureMainExportTestQjs, FeatureFindFeature_nameIsNull)
{
    auto ft_obj = FeatureFindFeature(manager_handle_qjs, nullptr);
    ft_value_t zero_value = {}; // 全零的结构体
    EXPECT_TRUE(memcmp(&ft_obj, &zero_value, sizeof(ft_value_t)) == 0);
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
TEST_F(FeatureMainExportTestQjs, FeatureSetManagerUserData_userDataUpdata)
{
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    char* str1 = (char*)malloc(6);
    strcpy(str1, "world");
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    FeatureSetManagerUserData(manager_handle_qjs, "data", str);
    EXPECT_EQ(manager->getUserData("data"), str);
    FeatureSetManagerUserData(manager_handle_qjs, "data", str1);
    EXPECT_EQ(manager->getUserData("data"), str1);
    FeatureSetManagerUserData(manager_handle_qjs, "data", nullptr);
    free(str);
    free(str1);
}
TEST_F(FeatureMainExportTestQjs, FeatureSetManagerUserData_handleIsNull)
{
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    FeatureSetManagerUserData(nullptr, "data", str);
    free(str);
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
    EXPECT_NE(JS_IsUndefined(js_obj), true);
    JSValue unit_test = JS_GetPropertyStr(js_env.ctx, js_obj, "unitTest");
    EXPECT_NE(JS_IsUndefined(unit_test), true);
    JS_FreeValue(js_env.ctx, unit_test);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), new_feature);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), js_feature_prototype);
}
TEST_F(FeatureMainExportTestQjs, FeatureCreateFeature_handleIsNull)
{
    auto js_feature_prototype = FeatureFindFeature(manager_handle_qjs, pDesc1.name);
    ft_value_t binding_obj;
    FT_VAL_GET_JS_VAL(binding_obj) = JS_UNDEFINED;
    auto ft_obj = FeatureCreateFeature(nullptr, js_feature_prototype, binding_obj);
    ft_value_t zero_value = {}; // 全零的结构体
    EXPECT_TRUE(memcmp(&ft_obj, &zero_value, sizeof(ft_value_t)) == 0);
    ft_free_value(FeatureManagerGetContext(manager_handle_qjs), js_feature_prototype);
}
TEST_F(FeatureMainExportTestQjs, FeatureCreateFeature_prototypeIsUdefined)
{
    ft_value_t js_feature_prototype;
    FT_VAL_GET_JS_VAL(js_feature_prototype) = JS_UNDEFINED;
    ft_value_t binding_obj;
    FT_VAL_GET_JS_VAL(binding_obj) = JS_UNDEFINED;
    auto ft_obj = FeatureCreateFeature(manager_handle_qjs, js_feature_prototype, binding_obj);
    auto js_obj = FT_VAL_GET_JS_VAL(ft_obj);
    EXPECT_EQ(JS_IsUndefined(js_obj), true);
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
TEST_F(FeatureMainExportTestQjs, FeatureHasFeature_handleIsNull)
{
    const char* name = "unit_test";
    auto res = FeatureHasFeature(nullptr, name);
    EXPECT_EQ(res, false);
}
TEST_F(FeatureMainExportTestQjs, FeatureHasFeature_nameIsNull)
{
    auto res = FeatureHasFeature(manager_handle_qjs, nullptr);
    EXPECT_EQ(res, false);
}

// =============================================================================
// FeatureSetUriConvertCb Tests
// =============================================================================
TEST_F(FeatureMainExportTestQjs, FeatureSetUriConvertCb1)
{
    FeatureSetUriConvertCb(manager_handle_qjs, test_set_uri_convert_cb);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->uriConvertCb(), test_set_uri_convert_cb);
    FeatureSetUriConvertCb(manager_handle_qjs, nullptr);
    EXPECT_EQ(manager->uriConvertCb(), nullptr);
}
TEST_F(FeatureMainExportTestQjs, FeatureSetUriConvertCb_handleIsNull)
{
    FeatureSetUriConvertCb(nullptr, test_set_uri_convert_cb);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->uriConvertCb(), nullptr);
}
TEST_F(FeatureMainExportTestQjs, FeatureSetUriConvertCb_cbIsNull)
{
    FeatureSetUriConvertCb(manager_handle_qjs, nullptr);
    FeatureManager* manager = static_cast<FeatureManager*>(manager_handle_qjs);
    EXPECT_EQ(manager->uriConvertCb(), nullptr);
}
} // namespace feature_framework