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
    }

    void TearDown() override
    {
        FeatureUnsetUVLoop(manager_handle_qjs);
        FeatureSetEventChangeListener(instance_handle, NULL);
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
    EXPECT_EQ(header->ref_count, 1);
    // EXPECT_EQ(header->featureType, (uintptr_t)&simple_struct_type);
    FeatureFreeValue(simple_struct);
}
TEST_F(FeatureExportTestQjs, FeatureMalloc_arrayAlloc)
{
    FtArray* simple_array_test = (FtArray*)FeatureMalloc(sizeof(FtArray), FT_MK_COMPLEX(&simple_array));
    auto header = (FTObjHeader*)((char*)simple_array_test - FT_OBJ_HEADER_SIZE);
    // EXPECT_TRUE(FT_IS_COMPLEX(header->featureType));
    EXPECT_EQ(header->ref_count, 1);
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
// FeatureGetBindingObject Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeatureGetBindingObject1)
{
    //测试是否能正确获取绑定对象
    auto binding_obj = FeatureGetBindingObject(instance_handle);
    EXPECT_EQ(binding_obj, FT_VAL_GET_JS_VAL(param));
}
TEST_F(FeatureExportTestQjs, FeatureGetBindingObject_handleIsNull)
{
    auto binding_obj = FeatureGetBindingObject(nullptr);
    EXPECT_EQ(binding_obj, FEATURE_UNDEFINED);
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

// =============================================================================
// FeaturePromiseResolve Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeaturePromiseResolve1)
{
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    EXPECT_EQ(FeaturePromiseResolve(instance_handle, pid, 1), true);
    // FeaturePromiseResolve后确认promise已被释放
    EXPECT_EQ(((FeatureInstanceQjs*)(instance_handle))->getPromise(pid), FEATURE_VALUE_UNDEFINED);
}

// =============================================================================
// FeaturePromiseReject Tests
// =============================================================================
TEST_F(FeatureExportTestQjs, FeaturePromiseReject1)
{
    FtPromiseId pid = ((FeatureInstanceQjs*)(instance_handle))->addPromise(promise_type.resolveType);
    EXPECT_EQ(FeaturePromiseReject(instance_handle, pid, 400, "reject"), true);
    // FeaturePromiseReject
    EXPECT_EQ(((FeatureInstanceQjs*)(instance_handle))->getPromise(pid), FEATURE_VALUE_UNDEFINED);
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

TEST_F(FeatureExportTestQjs, FeatureInstanceIsDetached1)
{
    //测试能否正确地判断instance是否已经detached
    EXPECT_EQ(FeatureInstanceIsDetached(instance_handle), false);
    ((FeatureInstanceQjs*)(instance_handle))->onDetached();
    EXPECT_EQ(FeatureInstanceIsDetached(instance_handle), true);
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
    data->data_changed_added = false;
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
    FeatureEmitEvent(instance_handle, 1, "hello");
    FeatureEmitEvent(instance_handle, 2, 77);
    JS_FreeValue(js_env.ctx, data_changed_callback);
    JS_FreeValue(js_env.ctx, state_changed_callback);
}
// Worker相关

} // namespace feature_framework_test
