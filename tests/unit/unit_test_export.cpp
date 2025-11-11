#include "backend/qjs/feature_context_qjs.h"
#include "backend/qjs/feature_instance_qjs.h"
#include "backend/qjs/feature_manager_qjs.h"
#include "backend/qjs/feature_prototype_qjs.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_main_exports.h"

namespace feature_framework {
typedef struct _UnitEventData {
    bool data_changed_added;
    bool state_changed_added;
} UnitEventData;

typedef struct _structSimple {
    FtInt int_test;
    FtBool boolean_test;
} structSimple;

FtString test_const = "hello world";

// for event
static const FeatureType event_data_changed_parameters[] = {
    FT_STRING,
    FT_PARAM_END
};
static const MemberEvent data_changed_member_event = {
    .parameters = event_data_changed_parameters,
    .id = 1,
    .name = "data_changed",
};
static const FeatureType event_state_changed_parameters[] = {
    FT_INT,
    FT_PARAM_END
};
static const MemberEvent state_changed_member_event = {
    .parameters = event_state_changed_parameters,
    .id = 2,
    .name = "state_changed",
};

static const MemberMethod unit_test_method = {
    .func_stub = nullptr,
    .parameters = nullptr,
    .return_type = FT_VOID,
};
static const MemberConst unit_test_require = {
    .type = FT_STRING,
    .func = { .callback = nullptr },
    .data = { .str = test_const }
};

static const Member testMembers[] = {
    {
        .type = MEMBER_METHOD,
        .name = "unitTest",
        .method = &unit_test_method,
    },
    {
        .type = MEMBER_CONST,
        .name = "constTest",
        .value = &unit_test_require,
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

// for callback
static const FeatureType callback_parameters[] = {
    FT_INT,
    FT_PARAM_END
};
static CallbackType callback_type = {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = callback_parameters,
    .return_type = FT_VOID
};
static const FeatureType variable_callback_parameters[] = {
    FT_INT,
    FT_PARAM_REST_END
};
static CallbackType variable_callback_type = {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = variable_callback_parameters,
    .return_type = FT_VOID
};

// for promise
static const PromiseType promise_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveType = FT_INT32
};

// for array
static const ArrayType simple_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_INT32
};

// for struct
static ObjectMember simple_struct_members[] = {
    { "int_test", FT_INT, offsetof(structSimple, int_test), sizeof(FtInt) },
    { "boolean_test", FT_BOOLEAN, offsetof(structSimple, boolean_test), sizeof(FtBool) },
    { NULL },
};

static bool test_featureMalloc(size_t size, FeatureType featureType)
{
    //测试是否能正确分配内存
    auto data = FeatureMalloc(size, featureType);
    if (data == nullptr) {
        return false;
    }
    // 检查分配的内存是否已正确初始化为 0
    for (size_t i = 0; i < size; ++i) {
        if (((char*)data)[i] != 0) {
            return false;
        }
    }
    auto header = (FTObjHeader*)((char*)data - FT_OBJ_HEADER_SIZE);
    if (header->ref_count != 1)
        return false;
    auto type = *(FeatureType*)((char*)header - sizeof(FeatureType));
    if (type != featureType)
        return false;
    FeatureFreeValue(data);
    return true;
}
static void test_eventChange(FeatureInstanceHandle handle, FtEventId eid, FeatureEventStatus status)
{
    const char* event_name = FeatureGetEventName(handle, eid);
    UnitEventData* data = (UnitEventData*)FeatureGetObjectData(handle);
    if (!data) {
        return;
    }
    int added = (status == FEATURE_EVENT_ADDED ? 1 : 0);
    if (strcmp(event_name, "data_changed") == 0) {
        data->data_changed_added = added;
    } else if (strcmp(event_name, "state_changed") == 0) {
        data->state_changed_added = added;
    }
}

static void test_userdata_free(void* data)
{
    free(data);
}

static char* test_uri_convert_cb(const char* package_name, const char* uri)
{
    if (!uri || !package_name) {
        FEATURE_LOG_ERROR("%s: uri or package_name is null!", __func__);
        return nullptr;
    }
    return strdup(uri);
}

class FeatureExportTestQjs : public ::testing::Test {
protected:
    FeatureInstanceHandle instance_handle;
    FeatureManagerHandle manager_handle_qjs;
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
        manager_handle_qjs = FeatureCreateManager(&ft_info);
        FeatureRegistryHandle hRegistry = FeatureGetRegistryFromManager(manager_handle_qjs);
        FeatureRegisterFeature(hRegistry, &pDesc);
        FT_VAL_GET_JS_VAL(param) = JS_UNDEFINED;
        auto res = FeatureRequire(manager_handle_qjs, param, pDesc.name);
        feature_obj = FT_VAL_GET_JS_VAL(res);
        instance_handle = (FeatureInstanceHandle)feature_get_opaque(feature_obj, FeatureManagerQjs::jsClassId());
        loop = uv_default_loop();
        FeatureSetUVLoop(manager_handle_qjs, loop);
        FeatureSetEventChangeListener(instance_handle, test_eventChange);
        FeatureSetUriConvertCb(manager_handle_qjs, test_uri_convert_cb);
    }

    void TearDown() override
    {
        FeatureUnsetUVLoop(manager_handle_qjs);
        FeatureSetEventChangeListener(instance_handle, NULL);
        FeatureSetUriConvertCb(manager_handle_qjs, NULL);
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
        JS_FreeValue(js_env.ctx, feature_obj);
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
// FeatureMalloc Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureMalloc_basetypeAlloc)
{
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_INT8), FT_INT8));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_INT16), FT_INT16));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_INT32), FT_INT32));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_INT64), FT_INT64));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_UINT8), FT_UINT8));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_UINT16), FT_UINT16));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_UINT32), FT_UINT32));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_UINT64), FT_UINT64));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_FLOAT), FT_FLOAT));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_DOUBLE), FT_DOUBLE));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_BOOLEAN), FT_BOOLEAN));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_STRING), FT_STRING));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_CHAR), FT_CHAR));
    EXPECT_TRUE(test_featureMalloc(getValueSize(FT_ANY_REF), FT_ANY_REF));
}
// TEST_F(FeatureExportTestQjs, FeatureMalloc_failAlloc) {
//     // 模拟分配失败的情况
//     size_t largeSize = std::numeric_limits<size_t>::max();
//     void* ptr = FeatureMalloc(largeSize, FT_STRING);
//     EXPECT_EQ(ptr, nullptr);
// }
TEST_F(FeatureExportTestQjs, FeatureMalloc_structAlloc)
{
    const ObjectMapType simple_struct_type = {
        .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(structSimple) },
        .members = simple_struct_members
    };

    structSimple* simple_struct = (structSimple*)FeatureMalloc(
        sizeof(structSimple), FT_MK_COMPLEX(&simple_struct_type));

    auto header = (FTObjHeader*)((char*)simple_struct - FT_OBJ_HEADER_SIZE);
    // EXPECT_TRUE(FT_IS_COMPLEX(header->featureType));
    EXPECT_EQ(header->ref_count, (uint32_t)1);
    // EXPECT_EQ(header->featureType, (uintptr_t)&simple_struct_type);
    FeatureFreeValue(simple_struct);
}
TEST_F(FeatureExportTestQjs, FeatureMalloc_arrayAlloc)
{
    FtArray* simple_array_test = (FtArray*)FeatureMalloc(sizeof(FtArray), FT_MK_COMPLEX(&simple_array));
    auto header = (FTObjHeader*)((char*)simple_array_test - FT_OBJ_HEADER_SIZE);
    // EXPECT_TRUE(FT_IS_COMPLEX(header->featureType));
    EXPECT_EQ(header->ref_count, (uint32_t)1);
    // EXPECT_EQ(header->featureType, (uintptr_t)&simple_array);
    FeatureFreeValue(simple_array_test);
}

// =============================================================================
// FeatureDupValue Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureDupValue1)
{
    //测试是否能正确dup对象
    auto data = FeatureMalloc(getValueSize(FT_INT32), FT_INT32);
    auto dup_data = FeatureDupValue(data);
    EXPECT_EQ(dup_data, data);
    auto header = (FTObjHeader*)((char*)data - FT_OBJ_HEADER_SIZE);
    EXPECT_EQ(header->ref_count, (uint32_t)2);
    FeatureFreeValue(dup_data);
    FeatureFreeValue(data);
}
TEST_F(FeatureExportTestQjs, FeatureDupValue_dataIsNull)
{
    void* data = nullptr;
    auto dup_data = FeatureDupValue(data);
    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(dup_data, nullptr);
}

// =============================================================================
// FeatureFreeValue Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureFreeValue1)
{
    auto data = FeatureMalloc(getValueSize(FT_INT32), FT_INT32);
    FeatureDupValue(data);
    auto header = (FTObjHeader*)((char*)data - FT_OBJ_HEADER_SIZE);
    FeatureFreeValue(data);
    EXPECT_EQ(header->ref_count, (uint32_t)1);
    FeatureFreeValue(data);
}
TEST_F(FeatureExportTestQjs, FeatureFreeValue_dataIsNull)
{
    void* data = nullptr;
    FeatureFreeValue(data);
}

// =============================================================================
// FeatureGetProtoHandle Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetProtoHandle1)
{
    //测试是否能正确获取protoHandle
    auto proto_type = FeatureGetProtoHandle(instance_handle);
    EXPECT_EQ((((FeatureInstance*)instance_handle)->prototype()), proto_type);
}
TEST_F(FeatureExportTestQjs, FeatureGetProtoHandle_handleIsNull)
{
    auto proto_type = FeatureGetProtoHandle(nullptr);
    EXPECT_EQ(proto_type, nullptr);
}

// =============================================================================
// FeatureSetProtoData Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureSetProtoData1)
{
    //测试是否能正确设置protoData
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    auto feature_proto_type_handle = (FeaturePrototype*)(FeatureGetProtoHandle(instance_handle));
    FeatureSetProtoData(feature_proto_type_handle, str);
    EXPECT_EQ(strcmp(str, (char*)feature_proto_type_handle->native()), 0);
    FeatureSetProtoData(feature_proto_type_handle, nullptr);
    EXPECT_EQ(feature_proto_type_handle->native(), nullptr);
    free(str);
}
TEST_F(FeatureExportTestQjs, FeatureSetProtoData_handleIsNull)
{
    FeatureSetProtoData(nullptr, nullptr);
}

// =============================================================================
// FeatureGetProtoData Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetProtoData1)
{
    //测试是否能正确获取protoData
    auto feature_proto_type_handle = (FeaturePrototype*)(FeatureGetProtoHandle(instance_handle));
    auto proto_data = FeatureGetProtoData(FeatureGetProtoHandle(instance_handle));
    EXPECT_EQ(proto_data, feature_proto_type_handle->native());
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    FeatureSetProtoData(feature_proto_type_handle, str);
    proto_data = FeatureGetProtoData(FeatureGetProtoHandle(instance_handle));
    EXPECT_EQ(strcmp(str, (char*)proto_data), 0);
    FeatureSetProtoData(feature_proto_type_handle, nullptr);
    free(str);
}
TEST_F(FeatureExportTestQjs, FeatureGetProtoData_handleIsNull)
{
    auto proto_data = FeatureGetProtoData(nullptr);
    EXPECT_EQ(proto_data, nullptr);
}

// =============================================================================
// FeatureGetPackageName Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetPackageName1)
{
    //测试是否能正确获取包名com.feature.test
    auto pkg_name = FeatureGetPackageName(FeatureGetProtoHandle(instance_handle));
    EXPECT_STREQ(pkg_name, "com.feature.test");
}
TEST_F(FeatureExportTestQjs, FeatureGetPackageName_handleIsNull)
{
    auto pkg_name = FeatureGetPackageName(nullptr);
    EXPECT_EQ(pkg_name, nullptr);
}

// =============================================================================
// FeatureGetPackageVersion Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetPackageVersion1)
{
    //测试是否能正确获取包版本
    FeatureSetPackageVersion(manager_handle_qjs, "3.14");
    auto pkg_version = FeatureGetPackageVersion(FeatureGetProtoHandle(instance_handle));
    EXPECT_STREQ(pkg_version, "3.14");
}
TEST_F(FeatureExportTestQjs, FeatureGetPackageVersion_handleIsNull)
{
    auto pkg_version = FeatureGetPackageVersion(nullptr);
    EXPECT_EQ(pkg_version, nullptr);
}

// =============================================================================
// FeatureSetObjectData Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureSetObjectData1)
{
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    FeatureSetObjectData(instance_handle, str);
    EXPECT_EQ(strcmp(str, (char*)((FeatureInstance*)instance_handle)->native()), 0);
    FeatureSetObjectData(instance_handle, nullptr);
    EXPECT_EQ(((FeatureInstance*)instance_handle)->native(), nullptr);
    free(str);
}
TEST_F(FeatureExportTestQjs, FeatureSetObjectData_handleIsNull)
{
    FeatureSetObjectData(nullptr, nullptr);
}

// =============================================================================
// FeatureGetObjectData Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetObjectData1)
{
    char* str = (char*)malloc(6);
    strcpy(str, "hello");
    FeatureSetObjectData(instance_handle, str);
    auto obj_data = FeatureGetObjectData(instance_handle);
    EXPECT_EQ(strcmp(str, (char*)obj_data), 0);
    FeatureSetObjectData(instance_handle, nullptr);
    free(str);
}
TEST_F(FeatureExportTestQjs, FeatureGetObjectData_handleIsNull)
{
    auto obj_data = FeatureGetObjectData(nullptr);
    EXPECT_EQ(obj_data, nullptr);
}

// =============================================================================
// FeatureGetContext Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetContext1)
{
    //测试是否能正确获取featurecontext
    auto ctx = FeatureGetContext(instance_handle);
    EXPECT_EQ(static_cast<JSContext*>(ctx->data), js_env.ctx);
}
TEST_F(FeatureExportTestQjs, FeatureGetContext_handleIsNull)
{
    auto ctx = FeatureGetContext(nullptr);
    EXPECT_EQ(ctx, nullptr);
}

// =============================================================================
// FeatureGetEnvironmentName Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetEnvironmentName1)
{
    //测试是否能正确获取环境名称
    auto env_name = FeatureGetEnvironmentName(FeatureGetProtoHandle(instance_handle));
    EXPECT_EQ(strcmp(env_name, "quickjs"), 0);
}
TEST_F(FeatureExportTestQjs, FeatureGetEnvironmentName_handleIsNull)
{
    auto env_name = FeatureGetEnvironmentName(nullptr);
    EXPECT_EQ(env_name, nullptr);
}

// =============================================================================
// FeatureInvokeCallback Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureInvokeCallback1)
{
    //测试是否能正确调用回调
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t local_var = 0;
            JS_ToInt32(ctx, &local_var, argv[0]);
            EXPECT_EQ(local_var, 114513);
            return JS_UNDEFINED;
        },
        "test", 1);
    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &callback_type);
    EXPECT_TRUE(FeatureInvokeCallback(instance_handle, id, 114513));
    ((FeatureInstanceQjs*)(instance_handle))->eraseCallback(id);
    JS_FreeValue(js_env.ctx, callback);
}
TEST_F(FeatureExportTestQjs, FeatureInvokeCallback_handleIsNull)
{
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t local_var = 0;
            JS_ToInt32(ctx, &local_var, argv[0]);
            EXPECT_EQ(local_var, 114513);
            return JS_UNDEFINED;
        },
        "test", 1);
    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &callback_type);
    EXPECT_FALSE(FeatureInvokeCallback(nullptr, 1, 114513));
    ((FeatureInstanceQjs*)(instance_handle))->eraseCallback(id);
    JS_FreeValue(js_env.ctx, callback);
}
TEST_F(FeatureExportTestQjs, FeatureInvokeCallback_callbackIdIsInvalid)
{
    EXPECT_FALSE(FeatureInvokeCallback(instance_handle, 0));
}

// =============================================================================
// FeatureRemoveCallback Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureRemoveCallback1)
{
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t local_var = 0;
            JS_ToInt32(ctx, &local_var, argv[0]);
            EXPECT_EQ(local_var, 1);
            return JS_UNDEFINED;
        },
        "test", 1);
    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &callback_type);
    EXPECT_TRUE(FeatureInvokeCallback(instance_handle, id, 1));
    EXPECT_TRUE(FeatureRemoveCallback(instance_handle, id));
    EXPECT_FALSE(FeatureInvokeCallback(instance_handle, id, 2));
    JS_FreeValue(js_env.ctx, callback);
}
TEST_F(FeatureExportTestQjs, FeatureRemoveCallback_handleIsNull)
{
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t local_var = 0;
            JS_ToInt32(ctx, &local_var, argv[0]);
            EXPECT_EQ(local_var, 1);
            return JS_UNDEFINED;
        },
        "test", 1);
    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &callback_type);
    EXPECT_FALSE(FeatureRemoveCallback(nullptr, id));
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
}
TEST_F(FeatureExportTestQjs, FeatureRemoveCallback_callbackIdIsInvalid)
{
    EXPECT_FALSE(FeatureRemoveCallback(instance_handle, 0));
}

// =============================================================================
// FeatureInvokeCallbackCount Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureInvokeCallbackCount1)
{
    //测试是否能正确调用变参回调
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            double d_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            JS_ToFloat64(ctx, &d_var, argv[1]);
            EXPECT_EQ(i_var, 114513);
            EXPECT_DOUBLE_EQ(d_var, 3.1415926535);
            return JS_UNDEFINED;
        },
        "test", 1);
    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &variable_callback_type);
    auto rest = FeatureMalloc(getValueSize(FT_DOUBLE), FT_DOUBLE);
    *(double*)rest = 3.1415926535;
    EXPECT_EQ(FeatureInvokeCallbackCount(instance_handle, id, 2, 114513, rest), true);
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
    FeatureFreeValue(rest);
}

TEST_F(FeatureExportTestQjs, FeatureInvokeCallbackCount_InvalidHandle)
{
    // 异常情况：测试无效的句柄
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            double d_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            JS_ToFloat64(ctx, &d_var, argv[1]);
            EXPECT_EQ(i_var, 114513);
            EXPECT_DOUBLE_EQ(d_var, 3.1415926535);
            return JS_UNDEFINED;
        },
        "test", 1);
    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &variable_callback_type);
    auto rest = FeatureMalloc(getValueSize(FT_DOUBLE), FT_DOUBLE);
    *(double*)rest = 3.1415926535;
    EXPECT_FALSE(FeatureInvokeCallbackCount(nullptr, id, 2, 114513, rest));
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
    FeatureFreeValue(rest);
}

TEST_F(FeatureExportTestQjs, FeatureInvokeCallbackCount_InvalidCallbackId)
{
    // 异常情况：测试无效的回调 ID
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            double d_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            JS_ToFloat64(ctx, &d_var, argv[1]);
            EXPECT_EQ(i_var, 114513);
            EXPECT_DOUBLE_EQ(d_var, 3.1415926535);
            return JS_UNDEFINED;
        },
        "test", 1);

    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &variable_callback_type);
    auto rest = FeatureMalloc(getValueSize(FT_DOUBLE), FT_DOUBLE);
    *(double*)rest = 3.1415926535;
    EXPECT_FALSE(FeatureInvokeCallbackCount(instance_handle, -1, 2, 114513, rest));
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
    FeatureFreeValue(rest);
}

TEST_F(FeatureExportTestQjs, FeatureInvokeCallbackCount_Mismatch)
{
    //异常情况： 测试 count 参数与实际传入的参数数量不匹配的情况
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            double d_var = 0;
            if (argc >= 1) {
                JS_ToInt32(ctx, &i_var, argv[0]);
            }
            if (argc >= 2) {
                JS_ToFloat64(ctx, &d_var, argv[1]);
            }
            EXPECT_EQ(i_var, 114513);
            if (argc >= 2) {
                EXPECT_DOUBLE_EQ(d_var, 3.1415926535);
            }
            return JS_UNDEFINED;
        },
        "test", 1);
    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &variable_callback_type);
    auto rest = FeatureMalloc(getValueSize(FT_DOUBLE), FT_DOUBLE);
    *(double*)rest = 3.1415926535;

    // 调用时 count 参数设为 1 < 实际传入的参数个数 2  // 预期返回 true
    EXPECT_TRUE(FeatureInvokeCallbackCount(instance_handle, id, 1, 114513, rest));

    // 清理资源
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
    FeatureFreeValue(rest);
}

TEST_F(FeatureExportTestQjs, FeatureInvokeCallbackCount_ZeroCount)
{
    // 边界情况：count小于最小入参个数
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            // 验证无参数传入
            EXPECT_EQ(argc, 0); //参数数量应为0
            return JS_UNDEFINED;
        },
        "test_no_args", 0); // 参数个数设为0

    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &variable_callback_type);

    // 调用时count参数设为0，不传后续参数
    EXPECT_FALSE(FeatureInvokeCallbackCount(instance_handle, id, 0));

    // 清理资源
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
}

// =============================================================================
// FeaturePromiseResolve Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeaturePromiseResolve1)
{
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    EXPECT_EQ(FeaturePromiseResolve(instance_handle, pid, 1), true);
    // FeaturePromiseResolve后确认promise已被释放
    EXPECT_EQ(JS_IsUndefined(((FeatureInstanceQjs*)(instance_handle))->getPromise(pid)), true);
}

TEST_F(FeatureExportTestQjs, FeaturePromiseResolve_handleIsNull)
{
    //异常情况： handle为空
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    EXPECT_EQ(FeaturePromiseResolve(nullptr, pid, 1), false);
}

TEST_F(FeatureExportTestQjs, FeaturePromiseResolve_promiseIdIsInvalid)
{
    // 异常情况：FtPromiseId无效
    FtPromiseId pid = -1;
    EXPECT_EQ(FeaturePromiseResolve(instance_handle, pid, 1), false);
}

// =============================================================================
// FeaturePromiseReject Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeaturePromiseReject1)
{
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    EXPECT_EQ(FeaturePromiseReject(instance_handle, pid, 400, "reject"), true);
    // FeaturePromiseReject
    EXPECT_EQ(JS_IsUndefined(((FeatureInstanceQjs*)(instance_handle))->getPromise(pid)), true);
}

TEST_F(FeatureExportTestQjs, FeaturePromiseReject_CompatibleWithFailCb)
{
    // 兼容性测试： 测试FtPromiseId替换Failcb的情况
    auto fail_cb = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[1]);
            const char* s_var = JS_ToCString(ctx, argv[0]);
            EXPECT_EQ(i_var, 400);
            EXPECT_STREQ(s_var, "reject");
            JS_FreeCString(ctx, s_var);
            return JS_UNDEFINED;
        },
        "test", 2);
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addAsyncCallbacks(promise_type.resolveType, JS_UNDEFINED, fail_cb, JS_UNDEFINED);
    EXPECT_EQ(FeaturePromiseReject(instance_handle, pid, 400, "reject"), true);
    // FeaturePromiseReject
    EXPECT_EQ(JS_IsUndefined(((FeatureInstanceQjs*)(instance_handle))->getPromise(pid)), true);
}

TEST_F(FeatureExportTestQjs, FeaturePromiseReject_handleIsNull)
{
    //异常情况： handle为空
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    EXPECT_EQ(FeaturePromiseReject(nullptr, pid, 400, "reject"), false);
}

TEST_F(FeatureExportTestQjs, FeaturePromiseReject_promiseIdIsInvalid)
{
    //异常情况： FtPromiseId无效
    FtPromiseId pid = -1;
    EXPECT_EQ(FeaturePromiseReject(instance_handle, pid, 400, "reject"), false);
}

TEST_F(FeatureExportTestQjs, FeaturePromiseReject_EmptyMessage)
{
    // 边界情况： message为空
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    // 调用 FeaturePromiseReject 函数，传入空的 message
    EXPECT_TRUE(FeaturePromiseReject(instance_handle, pid, 404, ""));
    // 清理资源
    ((FeatureInstanceQjs*)(instance_handle))->removePromise(pid);
}

TEST_F(FeatureExportTestQjs, FeaturePromiseReject_NegativeCode)
{
    // 边界情况： code为负
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    // 调用 FeaturePromiseReject 函数拒绝 Promise，传入负的拒绝代码
    EXPECT_TRUE(FeaturePromiseReject(instance_handle, pid, -1, "Error"));
    // 清理资源
    ((FeatureInstanceQjs*)(instance_handle))->removePromise(pid);
}

// =============================================================================
// FeatureGetPromiseType Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetPromiseType1)
{
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addAsyncCallbacks(promise_type.resolveType, JS_UNDEFINED, JS_UNDEFINED, JS_UNDEFINED);
    FtPromiseId pid1 = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    EXPECT_EQ(FeatureGetPromiseType(instance_handle, pid1), FEATURE_PROMISE_TYPE_PROMISE);
    EXPECT_EQ(FeatureGetPromiseType(instance_handle, pid), FEATURE_PROMISE_TYPE_CALLBACKS);
}

TEST_F(FeatureExportTestQjs, FeatureGetPromiseType_handleIsNull)
{
    //异常情况： handle为空
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addAsyncCallbacks(promise_type.resolveType, JS_UNDEFINED, JS_UNDEFINED, JS_UNDEFINED);
    EXPECT_EQ(FeatureGetPromiseType(nullptr, pid), FEATURE_PROMISE_TYPE_INVALID);
}

TEST_F(FeatureExportTestQjs, FeatureGetPromiseType_ftPromiseIdIsInvalid)
{
    //异常情况： FtPromiseId无效
    FtPromiseId pid = -1;
    EXPECT_EQ(FeatureGetPromiseType(instance_handle, pid), FEATURE_PROMISE_TYPE_INVALID);
}

// =============================================================================
// FeatureCreateInterface Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureCreateInterface1)
{
    static NativeFunc test_vtable_members[] = {
        NativeFunc(nullptr)
    };
    static VTable uploadtask_vtable = {
        .size = 1,
        .finalizer = NativeFunc(nullptr),
        .members = test_vtable_members
    };
    FeatureInterfaceHandle interface_handle = FeatureCreateInterface(instance_handle, &uploadtask_vtable);
    EXPECT_NE(interface_handle, nullptr);
    FeatureInstance* interface_instance = static_cast<FeatureInstance*>(interface_handle);
    EXPECT_EQ(interface_instance->isInterface(), true);
    interface_instance->release();
}

TEST_F(FeatureExportTestQjs, FeatureCreateInterface_handleIsNull)
{
    //异常情况： handle为空
    static NativeFunc test_vtable_members[] = {
        NativeFunc(nullptr)
    };
    static VTable uploadtask_vtable = {
        .size = 1,
        .finalizer = NativeFunc(nullptr),
        .members = test_vtable_members
    };
    FeatureInterfaceHandle interface_handle = FeatureCreateInterface(nullptr, &uploadtask_vtable);
    EXPECT_EQ(interface_handle, nullptr);
}

TEST_F(FeatureExportTestQjs, FeatureCreateInterface_ValidHandle_InvalidVTable)
{
    //  边界情况： 空 VTable
    VTable* invalid_vtable = nullptr;

    FeatureInterfaceHandle interface_handle = FeatureCreateInterface(instance_handle, invalid_vtable);

    // 验证返回值
    EXPECT_NE(interface_handle, nullptr);

    FeatureInstance* interface_instance = static_cast<FeatureInstance*>(interface_handle);
    EXPECT_EQ(interface_instance->isInterface(), false);
    interface_instance->release();
}

TEST_F(FeatureExportTestQjs, FeatureCreateInterface_ValidHandle_EmptyVTable)
{
    // 边界情况：空 VTable
    static NativeFunc test_vtable_members[] = {
        NativeFunc(nullptr)
    };
    static VTable empty_vtable = {
        .size = 0,
        .finalizer = NativeFunc(nullptr),
        .members = test_vtable_members
    };

    FeatureInterfaceHandle interface_handle = FeatureCreateInterface(instance_handle, &empty_vtable);

    // 验证返回值
    EXPECT_NE(interface_handle, nullptr);

    FeatureInstance* interface_instance = static_cast<FeatureInstance*>(interface_handle);
    EXPECT_EQ(interface_instance->isInterface(), true);
    interface_instance->release();
}

TEST_F(FeatureExportTestQjs, FeatureCreateInterface_ValidHandle_NullVTableMembers)
{
    // 边界情况：VTable 成员为 nullptr
    static VTable null_members_vtable = {
        .size = 1,
        .finalizer = NativeFunc(nullptr),
        .members = nullptr
    };

    FeatureInterfaceHandle interface_handle = FeatureCreateInterface(instance_handle, &null_members_vtable);

    // 验证返回值
    EXPECT_NE(interface_handle, nullptr);

    FeatureInstance* interface_instance = static_cast<FeatureInstance*>(interface_handle);
    EXPECT_EQ(interface_instance->isInterface(), true);
    interface_instance->release();
}

// =============================================================================
// FeaturePost Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeaturePost1)
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
    data->instanceHandle = instance_handle;
    strcpy(data->str, "hello xiaomi");
    EXPECT_EQ(FeaturePost(
                  instance_handle, [](int mode, void* data1) {
                      auto str = ((dataContext*)data1)->str;
                      auto loop1 = ((dataContext*)data1)->loop;
                      auto instanceHandle = ((dataContext*)data1)->instanceHandle;
                      if (mode == FEATURE_TASK_MODE_NORMAL) {
                          FEATURE_LOG_INFO("The outer FeaturePost data is %s", str);
                          strcpy(str, "xiaomi hello");
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
                  },
                  data),
        true);
    uv_run(loop, UV_RUN_DEFAULT);
    EXPECT_STREQ(data->str, "xiaomi hello");
    free(data->str);
    free(data);
}

TEST_F(FeatureExportTestQjs, FeaturePost_HandleNullptr)
{
    // 异常值： handle 为 nullptr
    struct dataContext {
        char* str;
        uv_loop_t* loop;
        FeatureInstanceHandle instanceHandle;
    };
    auto data = (dataContext*)malloc(sizeof(dataContext));
    data->str = (char*)malloc(20);
    data->loop = loop;
    data->instanceHandle = instance_handle;
    strcpy(data->str, "hello xiaomi");

    EXPECT_EQ(FeaturePost(
                  nullptr, [](int mode, void* data1) {
                      auto str = ((dataContext*)data1)->str;
                      if (mode == FEATURE_TASK_MODE_NORMAL) {
                          FEATURE_LOG_INFO("The outer FeaturePost data is %s", str);
                          strcpy(str, "xiaomi hello");
                      }
                  },
                  data),
        false);

    free(data->str);
    free(data);
}

TEST_F(FeatureExportTestQjs, FeaturePost_TaskCallbackNullptr)
{
    // 异常值： task_cb 为 nullptr
    struct dataContext {
        char* str;
        uv_loop_t* loop;
        FeatureInstanceHandle instanceHandle;
    };
    auto data = (dataContext*)malloc(sizeof(dataContext));
    data->str = (char*)malloc(20);
    data->loop = loop;
    data->instanceHandle = instance_handle;
    strcpy(data->str, "hello xiaomi");

    EXPECT_EQ(FeaturePost(instance_handle, nullptr, data), false);

    free(data->str);
    free(data);
}

TEST_F(FeatureExportTestQjs, FeaturePost_DataNullptr)
{
    // 边界情况： data 为 nullptr
    EXPECT_EQ(FeaturePost(
                  instance_handle, [](int mode, void* data) {
                      if (mode == FEATURE_TASK_MODE_NORMAL) {
                          FEATURE_LOG_ERROR("The outer FeaturePost data is null");
                      }
                  },
                  nullptr),
        true);
}

// TODO: 这个用例没有意义，并不能达成测试目的，需要重写
#if 0
TEST_F(FeatureExportTestQjs, FeaturePost_IllegalMode)
{
    // 异常情况： mode 为非法值
    struct dataContext {
        char* str;
    };
    auto data = (dataContext*)malloc(sizeof(dataContext));
    data->str = (char*)malloc(20);
    strcpy(data->str, "hello xiaomi");

    EXPECT_EQ(FeaturePost(
                  instance_handle, [](int mode, void* data) {
                      if (mode != FEATURE_TASK_MODE_NORMAL) {
                          FEATURE_LOG_ERROR("Callback should not be called with illegal mode");
                      }
                  },
                  data),
        true);

    free(data->str);
    free(data);
}
#endif

TEST_F(FeatureExportTestQjs, FeaturePost_DataMemoryAllocationFailure)
{
    // 边界情况： data 内存分配失败
    struct dataContext {
        char* str;
        uv_loop_t* loop;
        FeatureInstanceHandle instanceHandle;
    };
    auto dataCtx = (dataContext*)malloc(sizeof(dataContext));
    dataCtx->str = nullptr; // 模拟内存分配失败
    dataCtx->loop = loop;
    dataCtx->instanceHandle = instance_handle;

    EXPECT_EQ(FeaturePost(
                  instance_handle, [](int mode, void* data) {
                      // 这个回调不应该被执行
                      FEATURE_LOG_ERROR("Callback should not be called with nullptr data");
                  },
                  dataCtx),
        true);

    free(dataCtx);
}

TEST_F(FeatureExportTestQjs, FeaturePost_MultipleCalls)
{
    // 边界情况： FeaturePost 被调用多次
    struct dataContext {
        char* str;
        uv_loop_t* loop;
        FeatureInstanceHandle instanceHandle;
    };
    auto data = (dataContext*)malloc(sizeof(dataContext));
    data->str = (char*)malloc(20);
    data->loop = loop;
    data->instanceHandle = instance_handle;
    strcpy(data->str, "hello xiaomi");

    EXPECT_EQ(FeaturePost(
                  instance_handle, [](int mode, void* data1) {
                      auto str = ((dataContext*)data1)->str;
                      if (mode == FEATURE_TASK_MODE_NORMAL) {
                          FEATURE_LOG_INFO("The outer FeaturePost data is %s", str);
                          strcpy(str, "xiaomi hello");
                      }
                  },
                  data),
        true);

    EXPECT_EQ(FeaturePost(
                  instance_handle, [](int mode, void* data1) {
                      auto str = ((dataContext*)data1)->str;
                      if (mode == FEATURE_TASK_MODE_NORMAL) {
                          FEATURE_LOG_INFO("The outer FeaturePost data is %s", str);
                          strcpy(str, "hello again");
                          // 在第二次回调中停止事件循环
                          uv_stop(((dataContext*)data1)->loop);
                      }
                  },
                  data),
        true);

    uv_run(loop, UV_RUN_DEFAULT);
    EXPECT_STREQ(data->str, "hello again");

    free(data->str);
    free(data);
}

// =============================================================================
// FeatureGetManagerHandleFromInstance Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetManagerHandleFromInstance1)
{
    //测试能否正确地从instanceHandle获取到managerHandle
    auto manager_handle = FeatureGetManagerHandleFromInstance(instance_handle);
    EXPECT_EQ(manager_handle, manager_handle_qjs);
    EXPECT_EQ(manager_handle, ((FeaturePrototype*)(FeatureGetProtoHandle(instance_handle)))->featureManager());
}

TEST_F(FeatureExportTestQjs, FeatureGetManagerHandleFromInstance_HandleNullptr)
{
    // 异常情况： handle 为 nullptr
    FeatureManagerHandle manager_handle = FeatureGetManagerHandleFromInstance(nullptr);
    EXPECT_EQ(manager_handle, nullptr);
}

// =============================================================================
// FeatureGetUVLoop Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetUVLoop1)
{
    //测试能否正确获取到uvloop
    auto manager_handle = FeatureGetManagerHandleFromInstance(instance_handle);
    auto loop1 = FeatureGetUVLoop(manager_handle);
    EXPECT_EQ(loop1, ((FeatureManagerQjs*)(manager_handle))->getUVLoop());
    EXPECT_EQ(loop1, loop);
}

TEST_F(FeatureExportTestQjs, FeatureGetUVLoop_HandleNullptr)
{
    // 异常情况： handle 为 nullptr
    uv_loop_t* loop1 = FeatureGetUVLoop(nullptr);
    EXPECT_EQ(loop1, nullptr);
}

// =============================================================================
// FeatureGetManagerUserData Tests
// FeatureSetManagerUserData 在main_export.h中已经测试过了
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetManagerUserData1)
{
    //测试能否获取manager的userdata
    auto userData = FeatureGetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), "xiaomi");
    EXPECT_EQ(userData, nullptr);
    char* str = (char*)malloc(20);
    strcpy(str, "hello");
    FeatureSetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), "xiaomi", str);
    userData = FeatureGetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), "xiaomi");
    EXPECT_EQ(strcmp((char*)userData, "hello"), 0);
    free(str);
}

TEST_F(FeatureExportTestQjs, FeatureGetManagerUserData_HandleNullptr)
{
    // 异常情况： handle 为 nullptr
    char* str = (char*)malloc(20);
    strcpy(str, "hello");
    FeatureSetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), "xiaomi", str);
    void* userData = FeatureGetManagerUserData(nullptr, "xiaomi");
    EXPECT_EQ(userData, nullptr);
    free(str);
}

TEST_F(FeatureExportTestQjs, FeatureGetManagerUserData_NameNullptr)
{
    // 异常情况： name 为 nullptr
    char* str = (char*)malloc(20);
    strcpy(str, "hello");
    FeatureSetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), "xiaomi", str);
    void* userData = FeatureGetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), nullptr);
    EXPECT_EQ(userData, nullptr);
    free(str);
}

TEST_F(FeatureExportTestQjs, FeatureGetManagerUserData_NameEmpty)
{
    // 异常情况： name 为空字符串
    char* str = (char*)malloc(20);
    strcpy(str, "hello");
    FeatureSetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), "xiaomi", str);
    void* userData = FeatureGetManagerUserData(FeatureGetManagerHandleFromInstance(instance_handle), "");
    EXPECT_EQ(userData, nullptr);
    free(str);
}

// =============================================================================
// FeatureGetManagerHandleFromProto Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetManagerHandleFromProto1)
{
    //测试能否正确地从protoHandle获取到managerHandle
    auto protoHandle = FeatureGetProtoHandle(instance_handle);
    auto manager_handle = FeatureGetManagerHandleFromProto(protoHandle);
    EXPECT_EQ(manager_handle, manager_handle_qjs);
    EXPECT_EQ(manager_handle, ((FeaturePrototype*)(FeatureGetProtoHandle(instance_handle)))->featureManager());
}

TEST_F(FeatureExportTestQjs, FeatureGetManagerHandleFromProto_HandleNullptr)
{
    //异常情况： handle 为 nullptr
    EXPECT_EQ(FeatureGetManagerHandleFromProto(nullptr), nullptr);
}

// =============================================================================
// FeatureCheckCallbackId Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureCheckCallbackId1)
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

    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &callback_type);
    EXPECT_EQ(FeatureCheckCallbackId(instance_handle, id), true);
    EXPECT_EQ(FeatureCheckCallbackId(instance_handle, 114514), false);
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
}

TEST_F(FeatureExportTestQjs, FeatureCheckCallbackId_HandleNullptr)
{
    //异常情况： handle 为 nullptr
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t a = 0;
            JS_ToInt32(ctx, &a, argv[0]);
            FEATURE_LOG_INFO("callback param is %d", ++a);
            FEATURE_LOG_INFO("hello callback");
            return JS_UNDEFINED;
        },
        "test", 1);

    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &callback_type);
    EXPECT_EQ(FeatureCheckCallbackId(nullptr, id), false);
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
}

TEST_F(FeatureExportTestQjs, FeatureCheckCallbackId_IllegalCallbackId)
{
    // 异常情况： cid 为非法值
    auto callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t a = 0;
            JS_ToInt32(ctx, &a, argv[0]);
            FEATURE_LOG_INFO("callback param is %d", ++a);
            FEATURE_LOG_INFO("hello callback");
            return JS_UNDEFINED;
        },
        "test", 1);

    FtCallbackId id = ((FeatureInstanceQjs*)(instance_handle))->addCallback(callback, &callback_type);
    EXPECT_EQ(FeatureCheckCallbackId(instance_handle, -1), false);
    FeatureRemoveCallback(instance_handle, id);
    JS_FreeValue(js_env.ctx, callback);
}

// =============================================================================
// FeatureDupInstanceHandle Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureDupInstanceHandle1)
{
    //测试能否正确地dup和instanceHandle
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    EXPECT_EQ(instance_qjs->getRefCount(), 1);
    auto dup_instance = FeatureDupInstanceHandle(instance_handle);
    EXPECT_EQ(dup_instance, instance_handle);
    EXPECT_EQ(instance_qjs->getRefCount(), 2);
    FeatureFreeInstanceHandle(dup_instance);
}

TEST_F(FeatureExportTestQjs, FeatureDupInstanceHandle_HandleNullptr)
{
    //异常情况： handle 为 nullptr
    EXPECT_EQ(FeatureDupInstanceHandle(nullptr), nullptr);
}

// =============================================================================
// FeatureFreeInstanceHandle Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureFreeInstanceHandle1)
{
    //测试能否正确地freeinstanceHandle
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    FeatureDupInstanceHandle(instance_handle);
    FeatureFreeInstanceHandle(instance_handle);
    EXPECT_EQ(instance_qjs->getRefCount(), 1);
}

TEST_F(FeatureExportTestQjs, FeatureFreeInstanceHandle_HandleNullptr)
{
    // 异常情况：handle 为 nullptr
    FeatureFreeInstanceHandle(nullptr);
    // 期望不会崩溃或抛出异常
}

TEST_F(FeatureExportTestQjs, FeatureInstanceIsDetached1)
{
    //测试能否正确地判断instance是否已经detached
    EXPECT_EQ(FeatureInstanceIsDetached(instance_handle), false);
    ((FeatureInstanceQjs*)(instance_handle))->onDetached();
    EXPECT_EQ(FeatureInstanceIsDetached(instance_handle), true);
}

TEST_F(FeatureExportTestQjs, FeatureInstanceIs_HandleNullptr)
{
    //异常情况：handle 为 nullptr
    EXPECT_EQ(FeatureInstanceIsDetached(nullptr), false);
}

// =============================================================================
// FeatureSetEventChangeListener Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureSetEventChangeListener1)
{
    //测试能否正确地设置eventChangeListener
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    UnitEventData* data = (UnitEventData*)malloc(sizeof(UnitEventData));
    memset(data, 0, sizeof(UnitEventData));
    data->data_changed_added = false;
    data->state_changed_added = false;
    FeatureSetObjectData(instance_handle, data);
    // MemberEvent* member_event
    feature_value_t undefined = FEATURE_UNDEFINED;
    instance_qjs->addEventCallback(&data_changed_member_event, undefined);
    instance_qjs->addEventCallback(&state_changed_member_event, undefined);
    UnitEventData* out_data = (UnitEventData*)FeatureGetObjectData(instance_handle);
    // add event callback listenercb 正确赋值为true
    EXPECT_EQ(out_data->data_changed_added, true);
    EXPECT_EQ(out_data->state_changed_added, true);
    free(out_data);
    FeatureSetObjectData(instance_handle, nullptr);
}

TEST_F(FeatureExportTestQjs, FeatureSetEventChangeListener_cancelListener)
{
    //取消监听：测试能否正确地取消eventChangeListener
    FeatureSetEventChangeListener(instance_handle, nullptr);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    UnitEventData* data = (UnitEventData*)malloc(sizeof(UnitEventData));
    memset(data, 0, sizeof(UnitEventData));
    data->data_changed_added = false;
    data->state_changed_added = false;
    FeatureSetObjectData(instance_handle, data);
    // MemberEvent* member_event
    feature_value_t undefined = FEATURE_UNDEFINED;
    instance_qjs->addEventCallback(&data_changed_member_event, undefined);
    instance_qjs->addEventCallback(&state_changed_member_event, undefined);
    UnitEventData* out_data = (UnitEventData*)FeatureGetObjectData(instance_handle);
    // add event callback 状态未改变
    EXPECT_EQ(out_data->data_changed_added, false);
    EXPECT_EQ(out_data->state_changed_added, false);
    free(out_data);
    FeatureSetObjectData(instance_handle, nullptr);
}

TEST_F(FeatureExportTestQjs, FeatureSetEventChangeListener_HandleNullptr)
{
    //异常情况：handle 为 nullptr
    FeatureSetEventChangeListener(nullptr, test_eventChange);
}

// =============================================================================
// FeatureGetEventId Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetEventId1)
{
    //测试能否正确地获取eventId
    feature_value_t undefined = FEATURE_UNDEFINED;
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, undefined);
    instance_qjs->addEventCallback(&state_changed_member_event, undefined);
    EXPECT_EQ(FeatureGetEventId(instance_handle, "data_changed"), 1);
    EXPECT_EQ(FeatureGetEventId(instance_handle, "state_changed"), 2);
}

// =============================================================================
// FeatureGetEventName Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetEventName1)
{
    //测试能否正确地获取eventName
    feature_value_t undefined = FEATURE_UNDEFINED;
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, undefined);
    instance_qjs->addEventCallback(&state_changed_member_event, undefined);
    EXPECT_STREQ(FeatureGetEventName(instance_handle, 1), "data_changed");
    EXPECT_STREQ(FeatureGetEventName(instance_handle, 2), "state_changed");
}

TEST_F(FeatureExportTestQjs, FeatureGetEventName_HandleNullptr)
{
    //异常情况：handle 为 nullptr
    EXPECT_EQ(FeatureGetEventName(nullptr, 1), nullptr);
    EXPECT_EQ(FeatureGetEventName(nullptr, 2), nullptr);
}

TEST_F(FeatureExportTestQjs, FeatureGetEventName_IllegalEventId)
{
    // 异常情况：eid 为非法值
    EXPECT_EQ(FeatureGetEventName(instance_handle, -1), nullptr);
    EXPECT_EQ(FeatureGetEventName(instance_handle, 0), nullptr);
}

// =============================================================================
// FeatureGetEventCallbackCount Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetEventCallbackCount1)
{
    //测试能否正确地获取eventCount
    feature_value_t undefined = FEATURE_UNDEFINED;
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, undefined);
    instance_qjs->addEventCallback(&state_changed_member_event, undefined);
    EXPECT_EQ(FeatureGetEventCallbackCount(instance_handle, 1), 1);
    EXPECT_EQ(FeatureGetEventCallbackCount(instance_handle, 2), 1);
}

TEST_F(FeatureExportTestQjs, FeatureGetEventCallbackCount_HandleNullptr)
{
    //异常情况：handle 为 nullptr
    EXPECT_EQ(FeatureGetEventCallbackCount(nullptr, 1), 0);
    EXPECT_EQ(FeatureGetEventCallbackCount(nullptr, 2), 0);
}

TEST_F(FeatureExportTestQjs, FeatureGetEventCallbackCount_IllegalEventId)
{
    // 异常情况：eid 为非法值
    EXPECT_EQ(FeatureGetEventCallbackCount(instance_handle, -1), 0);
    EXPECT_EQ(FeatureGetEventCallbackCount(instance_handle, 0), 0);
}
// =============================================================================
// FeatureGetEventCallbackCountByName Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetEventCallbackCountByName1)
{
    //测试能否正确地获取eventCount
    feature_value_t undefined = FEATURE_UNDEFINED;
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, undefined);
    instance_qjs->addEventCallback(&state_changed_member_event, undefined);
    EXPECT_EQ(FeatureGetEventCallbackCountByName(instance_handle, "data_changed"), 1);
    EXPECT_EQ(FeatureGetEventCallbackCountByName(instance_handle, "state_changed"), 1);
}

TEST_F(FeatureExportTestQjs, FeatureGetEventCallbackCountByName_HandleNullptr)
{
    //异常情况：handle 为 nullptr
    feature_value_t undefined = FEATURE_UNDEFINED;
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, undefined);
    instance_qjs->addEventCallback(&state_changed_member_event, undefined);
    EXPECT_EQ(FeatureGetEventCallbackCountByName(nullptr, "data_changed"), 0);
    EXPECT_EQ(FeatureGetEventCallbackCountByName(nullptr, "state_changed"), 0);
}

TEST_F(FeatureExportTestQjs, FeatureGetEventCallbackCountByName_IllegalEventId)
{
    // 异常情况：eid 为非法值
    EXPECT_EQ(FeatureGetEventCallbackCountByName(instance_handle, ""), 0);
}

// =============================================================================
// FeatureEmitEventByName Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureEmitEventByName1)
{
    auto data_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            const char* var = feature_to_cstring(ctx, argv[0]);
            EXPECT_STREQ(var, "hello");
            FEATURE_LOG_INFO("data_changed_callback!");
            feature_free_cstring(ctx, var);
            return JS_UNDEFINED;
        },
        "data_changed_test", 1);
    auto state_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            EXPECT_EQ(i_var, 77);
            return JS_UNDEFINED;
        },
        "state_changed_test", 1);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, data_changed_callback);
    instance_qjs->addEventCallback(&state_changed_member_event, state_changed_callback);
    FeatureEmitEventByName(instance_handle, "data_changed", "hello");
    FeatureEmitEventByName(instance_handle, "state_changed", 77);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}

TEST_F(FeatureExportTestQjs, FeatureEmitEventByName_HandleNullptr)
{
    //异常情况：handle 为 nullptr
    auto data_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            const char* var = feature_to_cstring(ctx, argv[0]);
            EXPECT_STREQ(var, "hello");
            FEATURE_LOG_INFO("data_changed_callback!");
            feature_free_cstring(ctx, var);
            return JS_UNDEFINED;
        },
        "data_changed_test", 1);
    auto state_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            EXPECT_EQ(i_var, 77);
            return JS_UNDEFINED;
        },
        "state_changed_test", 1);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, data_changed_callback);
    instance_qjs->addEventCallback(&state_changed_member_event, state_changed_callback);
    EXPECT_EQ(FeatureEmitEventByName(nullptr, "data_changed", "hello"), false);
    EXPECT_EQ(FeatureEmitEventByName(nullptr, "state_changed", 77), false);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}

TEST_F(FeatureExportTestQjs, FeatureEmitEventByName_NameEmpty)
{
    //边界情况：name 为 ""
    auto data_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            const char* var = feature_to_cstring(ctx, argv[0]);
            EXPECT_STREQ(var, "hello");
            FEATURE_LOG_INFO("data_changed_callback!");
            feature_free_cstring(ctx, var);
            return JS_UNDEFINED;
        },
        "data_changed_test", 1);
    auto state_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            EXPECT_EQ(i_var, 77);
            return JS_UNDEFINED;
        },
        "state_changed_test", 1);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, data_changed_callback);
    instance_qjs->addEventCallback(&state_changed_member_event, state_changed_callback);
    EXPECT_EQ(FeatureEmitEventByName(instance_handle, "", "hello"), false);
    EXPECT_EQ(FeatureEmitEventByName(instance_handle, "", 77), false);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}

TEST_F(FeatureExportTestQjs, FeatureEmitEventByName_NameNullptr)
{
    //边界情况：name 为 nullptr
    auto data_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            const char* var = feature_to_cstring(ctx, argv[0]);
            EXPECT_STREQ(var, "hello");
            FEATURE_LOG_INFO("data_changed_callback!");
            feature_free_cstring(ctx, var);
            return JS_UNDEFINED;
        },
        "data_changed_test", 1);
    auto state_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            EXPECT_EQ(i_var, 77);
            return JS_UNDEFINED;
        },
        "state_changed_test", 1);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, data_changed_callback);
    instance_qjs->addEventCallback(&state_changed_member_event, state_changed_callback);
    EXPECT_EQ(FeatureEmitEventByName(instance_handle, nullptr, "hello"), false);
    EXPECT_EQ(FeatureEmitEventByName(instance_handle, nullptr, 77), false);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}
// =============================================================================
// FeatureEmitEvent Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureEmitEvent1)
{
    auto data_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            const char* var = feature_to_cstring(ctx, argv[0]);
            EXPECT_STREQ(var, "hello");
            FEATURE_LOG_INFO("data_changed_callback!");
            feature_free_cstring(ctx, var);
            return JS_UNDEFINED;
        },
        "data_changed_test", 1);
    auto state_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            EXPECT_EQ(i_var, 77);
            return JS_UNDEFINED;
        },
        "state_changed_test", 1);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, data_changed_callback);
    instance_qjs->addEventCallback(&state_changed_member_event, state_changed_callback);
    EXPECT_EQ(FeatureEmitEvent(instance_handle, 1, "hello"), true);
    EXPECT_EQ(FeatureEmitEvent(instance_handle, 2, 77), true);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}

TEST_F(FeatureExportTestQjs, FeatureEmitEvent_HandleNullptr)
{
    auto data_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            const char* var = feature_to_cstring(ctx, argv[0]);
            EXPECT_STREQ(var, "hello");
            FEATURE_LOG_INFO("data_changed_callback!");
            feature_free_cstring(ctx, var);
            return JS_UNDEFINED;
        },
        "data_changed_test", 1);
    auto state_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            EXPECT_EQ(i_var, 77);
            return JS_UNDEFINED;
        },
        "state_changed_test", 1);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, data_changed_callback);
    instance_qjs->addEventCallback(&state_changed_member_event, state_changed_callback);
    EXPECT_EQ(FeatureEmitEvent(nullptr, 1, "hello"), false);
    EXPECT_EQ(FeatureEmitEvent(nullptr, 2, 77), false);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}

TEST_F(FeatureExportTestQjs, FeatureEmitEvent_IllegalEventId)
{
    auto data_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            const char* var = feature_to_cstring(ctx, argv[0]);
            EXPECT_STREQ(var, "hello");
            FEATURE_LOG_INFO("data_changed_callback!");
            feature_free_cstring(ctx, var);
            return JS_UNDEFINED;
        },
        "data_changed_test", 1);
    auto state_changed_callback = JS_NewCFunction(
        js_env.ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
            int32_t i_var = 0;
            JS_ToInt32(ctx, &i_var, argv[0]);
            EXPECT_EQ(i_var, 77);
            return JS_UNDEFINED;
        },
        "state_changed_test", 1);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    instance_qjs->addEventCallback(&data_changed_member_event, data_changed_callback);
    instance_qjs->addEventCallback(&state_changed_member_event, state_changed_callback);
    EXPECT_EQ(FeatureEmitEvent(instance_handle, 7788, "hello"), false);
    EXPECT_EQ(FeatureEmitEvent(instance_handle, 0, 77), false);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}
// Worker相关

// JsonObject相关
// =============================================================================
// FeatureAllocJsonObject Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureAllocJsonObject1)
{
    FtJsonObject json_str_data = FeatureAllocJsonObject(10);
    EXPECT_NE(json_str_data, nullptr);
    FeatureFreeValue(json_str_data);
}

// =============================================================================
// FeatureGetJsonString Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetJsonString1)
{
    const char* str = "{\"a\":1,\"b\":2}";
    FtJsonObject json_str_data = FeatureAllocJsonObject(14);
    EXPECT_NE(json_str_data, nullptr);
    memcpy(json_str_data->str, str, 14);
    const char* out_str = FeatureGetJsonString(json_str_data);
    EXPECT_STREQ(out_str, str);
    FeatureFreeValue(json_str_data);
}

TEST_F(FeatureExportTestQjs, FeatureGetJsonString_dataNullptr)
{
    const char* str = FeatureGetJsonString(nullptr);
    EXPECT_EQ(str, nullptr);
}

// =============================================================================
// FeatureNewJsonObject Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureNewJsonObject1)
{
    const char* str = "{\"a\":1,\"b\":2}";
    FtJsonObject json_str_data = FeatureNewJsonObject(str);
    EXPECT_NE(json_str_data, nullptr);
    EXPECT_STREQ(json_str_data->str, str);
    FeatureFreeValue(json_str_data);
}

TEST_F(FeatureExportTestQjs, FeatureNewJsonObject_strNullptr)
{
    FtJsonObject json_str_data = FeatureNewJsonObject(nullptr);
    EXPECT_EQ(json_str_data, nullptr);
}

// =============================================================================
// FeatureManagerHasUserData Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureManagerHasUserData1)
{
    char* str = (char*)malloc(6);
    FeatureSetManagerUserData(manager_handle_qjs, "test", str);
    EXPECT_TRUE(FeatureManagerHasUserData(manager_handle_qjs, "test"));
    free(str);
}

// =============================================================================
// FeatureSetManagerUserDataWithFreeCallback Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureSetManagerUserDataWithFree1)
{
    char* str = (char*)malloc(6);
    ManagerUserdataFreeCallback cb = test_userdata_free;
    FeatureSetManagerUserDataWithFreeCallback(manager_handle_qjs, "test", str, cb);
    EXPECT_TRUE(FeatureManagerHasUserData(manager_handle_qjs, "test"));
}

// =============================================================================
// FeatureThrowError Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureThrowError1)
{
    const char* msg = "test error";
    FeatureThrowError(instance_handle, msg);
    FeatureInstanceQjs* instance_qjs = static_cast<FeatureInstanceQjs*>(instance_handle);
    EXPECT_TRUE(instance_qjs->hasException());
}

TEST_F(FeatureExportTestQjs, FeatureThrowError_HandleNullptr)
{
    const char* msg = "test error";
    // 异常情况： handle 为 nullptr
    FeatureThrowError(nullptr, msg);
    // 期望不会崩溃或抛出异常
}

TEST_F(FeatureExportTestQjs, FeatureThrowError_MsgNullptr)
{
    // 异常情况： msg 为 nullptr
    FeatureThrowError(instance_handle, nullptr);
    // 期望不会崩溃或抛出异常
}

// =============================================================================
// FeatureGetPathFromUri Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetPathFromUri1)
{
    const char* path = "/data/local/tmp/test.txt";
    char* out_path = FeatureGetPathFromUri(instance_handle, path);
    EXPECT_NE(out_path, nullptr);
    EXPECT_STREQ(out_path, "/data/local/tmp/test.txt");
    free(out_path);
}

TEST_F(FeatureExportTestQjs, FeatureGetPathFromUri_HandleNullptr)
{
    const char* path = "/data/local/tmp/test.txt";
    // 异常情况： handle 为 nullptr
    EXPECT_EQ(FeatureGetPathFromUri(nullptr, path), nullptr);
}

TEST_F(FeatureExportTestQjs, FeatureGetPathFromUri_PathNullptr)
{
    // 异常情况： path 为 nullptr
    EXPECT_EQ(FeatureGetPathFromUri(instance_handle, nullptr), nullptr);
}

} // namespace feature_framework_test
