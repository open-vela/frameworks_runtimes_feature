#include "feature.h"
#include "feature_common.h"
#include "feature_context_qjs.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_instance.h"
#include "feature_instance_qjs.h"
#include "feature_main_exports.h"
#include "feature_manager.h"
#include "feature_manager_qjs.h"
#include "feature_prototype.h"
#include "feature_prototype_qjs.h"
#include "feature_types.h"
#include "quickjs/quickjs.h"
#include "uv.h"
#include "value_translator_qjs.h"
#include "gtest/gtest.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

static char* FeatureStrCopy(FeatureInstanceHandle instance, const char* str)
{
    char* ret = (char*)FeatureMalloc(strlen(str) + 1, FT_STRING);
    sprintf(ret, "%s", str);
    return ret;
}

namespace feature_framework {

class FeatureFrameworkTest : public ::testing::Test {
protected:
    FeatureInstanceHandle instance;
    FeatureManagerHandle g_manager_qjs;
    JSValue feature_obj;
    struct feature_env_t {
        JSRuntime* rt;
        JSContext* ctx;
    };
    feature_env_t js_env;
    ft_value_t param;
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
        g_manager_qjs = FeatureCreateManager(&ft_info);
        FeatureRegistryHandle hRegistry = FeatureGetRegistryFromManager(g_manager_qjs);
        static const MemberMethod unit_test_method = {
            .func_stub = nullptr,
            .parameters = nullptr,
            .return_type = FT_VOID,
        };
        static const Member testMembers[] = {
            {
                .type = MEMBER_METHOD,
                .name = "unitTest",
                .method = &unit_test_method,
            }
        };
        static const FeatureDescription pDesc = {
            .version = 1,
            .name = "unit_test",
            .description = "unit_test",
            .dynamic = false,
            .native_callbacks = nullptr,
            .member_count = 1,
            .members = testMembers,
        };
        FeatureRegisterFeature(hRegistry, &pDesc);
        *FT_VAL_GET_JS_VAL_PTR(param) = JS_UNDEFINED;
        auto res = FeatureRequire(g_manager_qjs, param, pDesc.name);
        feature_obj = FT_VAL_GET_JS_VAL(res);
        instance = (FeatureInstanceHandle)feature_get_opaque(feature_obj, FeatureManagerQjs::jsClassId());
        loop = uv_default_loop();
        FeatureSetUVLoop(g_manager_qjs, loop);
    }

    void TearDown() override
    {
        FeatureUnsetUVLoop(g_manager_qjs);
        JS_FreeValue(js_env.ctx, feature_obj);
        FeatureUninit(g_manager_qjs);
        FeatureFreeManager(g_manager_qjs);
        JS_FreeContext(js_env.ctx);
        JS_FreeRuntime(js_env.rt);
    }
};

//是否要对size为0和错误的featuretype做错误处理？
TEST_F(FeatureFrameworkTest, FeatureMalloc1)
{
    //测试是否能正确分配内存
    auto data = FeatureMalloc(getValueSize(FT_INT32), FT_INT32);
    EXPECT_NE(data, nullptr);
    *(int32_t*)data = 114514;
    EXPECT_EQ(*(int32_t*)data, 114514);
    auto header = (FTObjHeader*)((uintptr_t)data - sizeof(FTObjHeader));
    EXPECT_EQ(header->ref_count, (int32_t)1);
    FeatureFreeValue(data);
}

//是否要对非malloc出来的对象做错误处理？
TEST_F(FeatureFrameworkTest, FeatureDupValue1)
{
    //测试是否能正确dup对象
    auto str = FeatureStrCopy(instance, "hello");
    FeatureDupValue(str);
    auto header = (FTObjHeader*)(str - sizeof(FTObjHeader));
    EXPECT_EQ(header->ref_count, (int32_t)2);
    FeatureFreeValue(str);
}

// FeatureFreeValue

TEST_F(FeatureFrameworkTest, FeatureGetProtoHandle1)
{
    //测试是否能正确获取protoHandle
    auto protoType = FeatureGetProtoHandle(instance);
    EXPECT_EQ((((FeatureInstance*)instance)->prototype()), protoType);
}

TEST_F(FeatureFrameworkTest, FeatureGetProtoData1)
{
    //测试是否能正确获取protoData
    auto featureProtoTypeHandle = (FeaturePrototype*)(FeatureGetProtoHandle(instance));
    auto protoData = FeatureGetProtoData(FeatureGetProtoHandle(instance));
    EXPECT_EQ(protoData, featureProtoTypeHandle->native());
}

TEST_F(FeatureFrameworkTest, FeatureSetProtoData1)
{
    //测试是否能正确设置protoData
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    auto featureProtoTypeHandle = (FeaturePrototype*)(FeatureGetProtoHandle(instance));
    FeatureSetProtoData(featureProtoTypeHandle, str);
    EXPECT_EQ(strcmp(str, (char*)FeatureGetProtoData(FeatureGetProtoHandle(instance))), 0);
    free(str);
}

TEST_F(FeatureFrameworkTest, FeatureGetPackageName1)
{
    //测试是否能正确获取包名
    auto pkgName = FeatureGetPackageName(FeatureGetProtoHandle(instance));
    EXPECT_EQ(pkgName, ((FeaturePrototype*)FeatureGetProtoHandle(instance))->featureManager()->packageName());
}

TEST_F(FeatureFrameworkTest, FeatureGetPackageVersion1)
{
    //测试是否能正确获取包版本
    auto pkgVersion = FeatureGetPackageVersion(FeatureGetProtoHandle(instance));
    EXPECT_EQ(pkgVersion, ((FeaturePrototype*)FeatureGetProtoHandle(instance))->featureManager()->packageVesion());
}

// C++版本不再开放此两个接口
// FeatureGetObjectData
// FeatureSetObjectData

TEST_F(FeatureFrameworkTest, FeatureGetContext1)
{
    //测试是否能正确获取featurecontext
    auto ctx = FeatureGetContext(instance);
    EXPECT_EQ((void*)(ctx->data), (void*)js_env.ctx);
}

TEST_F(FeatureFrameworkTest, FeatureGetBindingObject1)
{
    //测试是否能正确获取绑定对象
    auto bindingObj = FeatureGetBindingObject(instance);
    EXPECT_EQ(memcmp(&bindingObj, &FT_VAL_GET_JS_VAL(param), sizeof(JSValue)), 0);
}

TEST_F(FeatureFrameworkTest, FeatureGetEnvironmentName1)
{
    //测试是否能正确获取环境名称
    auto envName = FeatureGetEnvironmentName(FeatureGetProtoHandle(instance));
    EXPECT_EQ(strcmp(envName, "quickjs"), 0);
}

//同时测试了FeatureRemoveCallback
TEST_F(FeatureFrameworkTest, FeatureInvokeCallback1)
{
    //测试是否能正确调用回调
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t a = 0;
            JS_ToInt32(ctx, &a, argv[0]);
            FEATURE_LOG_INFO("callback param is %d", ++a);
            FEATURE_LOG_INFO("hello callback");
            return JS_UNDEFINED;
        },
        "test", 1);
    auto cb_val = value_translator::toCallbackValue(callback);
    static const FeatureType callback_parameters[] = {
        FT_INT,
        FT_PARAM_END
    };
    static CallbackType callback_type = {
        .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
        .parameters = callback_parameters,
        .return_type = FT_VOID
    };
    FtCallbackId id = ((FeatureInstanceQjs*)(instance))->addCallback(cb_val, &callback_type);
    EXPECT_EQ(FeatureInvokeCallback(instance, id, 114513), true);
    EXPECT_EQ(FeatureRemoveCallback(instance, id), true);
    JS_FreeValue(js_env.ctx, callback);
}

TEST_F(FeatureFrameworkTest, FeatureInvokeCallbackCount1)
{
    //测试是否能正确调用变参回调
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t a = 0;
            JS_ToInt32(ctx, &a, argv[0]);
            FEATURE_LOG_INFO("callback fixed param is %d", ++a);
            double pi = 0;
            JS_ToFloat64(ctx, &pi, argv[1]);
            FEATURE_LOG_INFO("callback variadic param is %f", pi);
            FEATURE_LOG_INFO("hello variadic param callback");
            return JS_UNDEFINED;
        },
        "test", 1);
    auto cb_val = value_translator::toCallbackValue(callback);
    static const FeatureType callback_parameters[] = {
        FT_INT,
        FT_PARAM_REST_END
    };
    static CallbackType callback_type = {
        .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
        .parameters = callback_parameters,
        .return_type = FT_VOID
    };
    FtCallbackId id = ((FeatureInstanceQjs*)(instance))->addCallback(cb_val, &callback_type);
    auto rest = FeatureMalloc(getValueSize(FT_FLOAT), FT_FLOAT);
    *(float*)rest = 3.1415926535;
    EXPECT_EQ(FeatureInvokeCallbackCount(instance, id, 2, 114513, rest), true);
    EXPECT_EQ(FeatureRemoveCallback(instance, id), true);
    JS_FreeValue(js_env.ctx, callback);
    FeatureFreeValue(rest);
}

// FeaturePromiseResolve
// FeaturePromiseReject
// FeatureGetPromiseType
// FeatureCreateInterface

TEST_F(FeatureFrameworkTest, FeaturePost1)
{
    //测试是否能正确post异步任务，且异步任务能正确执行
    struct dataContext {
        char* str;
        uv_loop_t* loop;
        FeatureInstanceHandle instanceHandle;
    };
    auto data = (dataContext*)malloc(sizeof(dataContext));
    data->str = (char*)malloc(20);
    data->loop = loop;
    data->instanceHandle = instance;
    strcpy(data->str, "hello xiaomi");
    EXPECT_EQ(FeaturePost(
                  instance, [](int mode, void* data1) {
                      auto str = ((dataContext*)data1)->str;
                      auto loop1 = ((dataContext*)data1)->loop;
                      auto instanceHandle = ((dataContext*)data1)->instanceHandle;
                      if (mode == FEATURE_TASK_MODE_NORMAL) {
                          FEATURE_LOG_INFO("The outer FeaturePost data is %s", str);
                          struct dataContext1 {
                              int* number;
                              uv_loop_t* loop;
                          };
                          auto data2 = (dataContext1*)malloc(sizeof(dataContext1));
                          auto pa = (int*)malloc(sizeof(int));
                          *pa = 114514;
                          data2->loop = loop1;
                          data2->number = pa;
                          FeaturePost(
                              instanceHandle, [](int mode1, void* data3) {
                                  auto number = ((dataContext1*)data3)->number;
                                  auto loop2 = ((dataContext1*)data3)->loop;
                                  if (mode1 == FEATURE_TASK_MODE_NORMAL) {
                                      FEATURE_LOG_INFO("The inner FeaturePost data is %d", *number);
                                  }
                                  free(number);
                                  free(data3);
                                  uv_stop(loop2);
                              },
                              data2);
                      }
                      free(str);
                      free(data1);
                  },
                  data),
        true);
    uv_run(loop, UV_RUN_DEFAULT);
}

TEST_F(FeatureFrameworkTest, FeatureGetUVLoop1)
{
    //测试能否正确获取到uvloop
    auto managerHandle = FeatureGetManagerHandleFromInstance(instance);
    auto loop1 = FeatureGetUVLoop(managerHandle);
    EXPECT_EQ(loop1, ((FeatureManagerQjs*)(managerHandle))->getUVLoop());
    EXPECT_EQ(loop1, loop);
}

// 同时也测了FeatureSetManagerUserData
TEST_F(FeatureFrameworkTest, FeatureGetManagerUserData1)
{
    //测试能否正确设置和获取manager的userdata
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    FeatureSetManagerUserData(FeatureGetManagerHandleFromInstance(instance), "xiaomi", str);
    auto userData = FeatureGetManagerUserData(FeatureGetManagerHandleFromInstance(instance), "xiaomi");
    EXPECT_EQ(strcmp((char*)userData, "hello"), 0);
    free(str);
}

TEST_F(FeatureFrameworkTest, FeatureGetManagerHandleFromInstance1)
{
    //测试能否正确地从instanceHandle获取到managerHandle
    auto managerHandle = FeatureGetManagerHandleFromInstance(instance);
    EXPECT_EQ(managerHandle, g_manager_qjs);
    EXPECT_EQ(managerHandle, ((FeaturePrototype*)(FeatureGetProtoHandle(instance)))->featureManager());
}

TEST_F(FeatureFrameworkTest, FeatureGetManagerHandleFromProto1)
{
    //测试能否正确地从protoHandle获取到managerHandle
    auto protoHandle = FeatureGetProtoHandle(instance);
    auto managerHandle = FeatureGetManagerHandleFromProto(protoHandle);
    EXPECT_EQ(managerHandle, g_manager_qjs);
    EXPECT_EQ(managerHandle, ((FeaturePrototype*)(FeatureGetProtoHandle(instance)))->featureManager());
}

TEST_F(FeatureFrameworkTest, FeatureCheckCallbackId1)
{
    //测试能否正确检查callbackId是否存在
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t a = 0;
            JS_ToInt32(ctx, &a, argv[0]);
            FEATURE_LOG_INFO("callback param is %d", ++a);
            FEATURE_LOG_INFO("hello callback");
            return JS_UNDEFINED;
        },
        "test", 1);
    auto cb_val = value_translator::toCallbackValue(callback);
    static const FeatureType callback_parameters[] = {
        FT_INT,
        FT_PARAM_END
    };
    static CallbackType callback_type = {
        .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
        .parameters = callback_parameters,
        .return_type = FT_VOID
    };
    FtCallbackId id = ((FeatureInstanceQjs*)(instance))->addCallback(cb_val, &callback_type);
    EXPECT_EQ(FeatureCheckCallbackId(instance, id), true);
    EXPECT_EQ(FeatureCheckCallbackId(instance, 114514), false);
    FeatureRemoveCallback(instance, id);
    JS_FreeValue(js_env.ctx, callback);
}

// FeatureFreeInstanceHandle
TEST_F(FeatureFrameworkTest, FeatureDupInstanceHandle1)
{
    //测试能否正确地dup和free instanceHandle
    FeatureDupInstanceHandle(instance);
    FeatureFreeInstanceHandle(instance);
}

TEST_F(FeatureFrameworkTest, FeatureInstanceIsDetached1)
{
    //测试能否正确地判断instance是否已经detached
    EXPECT_EQ(FeatureInstanceIsDetached(instance), false);
    ((FeatureInstanceQjs*)(instance))->onDetached();
    EXPECT_EQ(FeatureInstanceIsDetached(instance), true);
}

// Event相关
// Worker相关

// manager相关一部分接口在SetUp和TearDown中已经测试过了，包括FeatureCreateManager，FeatureUninit和FeatureFreeManager

TEST_F(FeatureFrameworkTest, FeatureManagerGetContext1)
{
    //测试能否正确获取到feature context
    auto cxt_ref = FeatureManagerGetContext(g_manager_qjs);
    EXPECT_EQ(cxt_ref->data, js_env.ctx);
}

// FeatureSetArgsErrorCb

TEST_F(FeatureFrameworkTest, FeatureSetPackageVersion1)
{
    //测试能否正确设置和获取package版本
    FeatureSetPackageVersion(g_manager_qjs, "3.14");
    EXPECT_EQ(strcmp(FeatureGetPackageVersion(FeatureGetProtoHandle(instance)), "3.14"), 0);
}

// FeatureSetUVLoop和FeatureUnsetUVLoop在SetUp和TearDown中已经测试过了

// FeatureRequire在SetUp中已经测试过了

TEST_F(FeatureFrameworkTest, FeatureFindFeature)
{
    //测试能否正确查找feature
    auto undefined = FEATURE_VALUE_UNDEFINED;
    auto js_feature_prototype = FeatureFindFeature(g_manager_qjs, "unit_test");
    EXPECT_NE(memcmp(&js_feature_prototype, &undefined, sizeof(ft_value_t)), 0);
    EXPECT_EQ(memcmp(&(((FeaturePrototypeQjs*)FeatureGetProtoHandle(instance))->ft_proto()), &js_feature_prototype, sizeof(ft_value_t)), 0);
    JS_FreeValue(js_env.ctx, FT_VAL_GET_JS_VAL(js_feature_prototype));
    js_feature_prototype = FeatureFindFeature(g_manager_qjs, "fake_feature");
    EXPECT_EQ(memcmp(&js_feature_prototype, &undefined, sizeof(ft_value_t)), 0);
}

TEST_F(FeatureFrameworkTest, FeatureCreateFeature1)
{
    //测试能否正确创建feature
    auto js_feature_prototype = ((FeaturePrototypeQjs*)FeatureGetProtoHandle(instance))->ft_proto();
    auto managerHandle = FeatureGetManagerHandleFromInstance(instance);
    ft_value_t binding_obj;
    *(FT_VAL_GET_JS_VAL_PTR(binding_obj)) = JS_UNDEFINED;
    auto new_feature = FeatureCreateFeature(managerHandle, js_feature_prototype, binding_obj);
    JS_FreeValue(js_env.ctx, FT_VAL_GET_JS_VAL(new_feature));
}

TEST_F(FeatureFrameworkTest, FeatureHasFeature1)
{
    //测试能否正确判断feature或者method是否存在
    auto name = FeatureStrCopy(instance, "unit_test");
    auto res = FeatureHasFeature(g_manager_qjs, name);
    EXPECT_EQ(res, true);
    FeatureFreeValue(name);
    name = FeatureStrCopy(instance, "fake_feature");
    res = FeatureHasFeature(g_manager_qjs, name);
    EXPECT_EQ(res, false);
    FeatureFreeValue(name);
    name = FeatureStrCopy(instance, "unit_test.unitTest");
    res = FeatureHasFeature(g_manager_qjs, name);
    EXPECT_EQ(res, true);
    FeatureFreeValue(name);
    name = FeatureStrCopy(instance, "unit_test.fakeMthod");
    res = FeatureHasFeature(g_manager_qjs, name);
    EXPECT_EQ(res, false);
    FeatureFreeValue(name);
    name = FeatureStrCopy(instance, "fak_feature.unitTest");
    res = FeatureHasFeature(g_manager_qjs, name);
    EXPECT_EQ(res, false);
    FeatureFreeValue(name);
}

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
    EXPECT_EQ(FT_VAL_GET_JS_VAL(undefined_val), FEATURE_UNDEFINED);

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
    bool result;
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

// ft_from_typed_buffer
// ft_from_int_array
// ft_from_uint_array
// ft_from_int64_array
// ft_from_uint64_array
// ft_from_bool_array
// ft_from_double_array
// ft_from_string_array
// ft_parse_json
// ft_to_int
// ft_to_uint
// ft_to_int64
// ft_to_uint64
// ft_to_double
// ft_to_bool
// ft_to_string
// ft_to_buffer
// ft_array_size
// ft_array_at

// free函数需要关注异常参数
// ft_free_value
// ft_free_string

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
    EXPECT_NE(FT_VAL_GET_JS_VAL(result), FEATURE_UNDEFINED);
    const char* result_str = ft_to_string(ft_test_ctx, result);
    EXPECT_STREQ(result_str, "test_value");

    // 清理
    ft_free_value(ft_test_ctx, result);
    ft_free_string(ft_test_ctx, result_str);
    ft_free_value(ft_test_ctx, obj);
}

extern "C" int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace feature_framework
