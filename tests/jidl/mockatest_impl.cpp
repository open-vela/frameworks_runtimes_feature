#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __NuttX__
#include <nuttx/tls.h>
#endif

#include <map>

extern "C" {
#include <cmocka.h>
}

#include "mockatest.h"

#define DEFAULT_LEN 10
typedef struct {
    struct CMUnitTest* tests;
    FtCallbackId* callbackids;
    uint32_t len;
    uint32_t cap;
} UnitTests;

typedef struct {
    FeatureInstanceHandle handle;
    FtCallbackId id;
} CallbackInfo;

using UnitTestMap = std::map<FeatureInstanceHandle, UnitTests*>;

static UnitTests* CreateUnitTests()
{
    UnitTests* ret = (UnitTests*)malloc(sizeof(UnitTests));
    if (!ret)
        return NULL;
    ret->tests = (struct CMUnitTest*)malloc(DEFAULT_LEN * sizeof(struct CMUnitTest));
    ret->callbackids = (FtCallbackId*)malloc(DEFAULT_LEN * sizeof(FtCallbackId));
    ret->len = 0;
    ret->cap = DEFAULT_LEN;
    return ret;
}

static void DeleteUnitTests(UnitTests* u)
{
    if (!u)
        return;
    if (u->tests)
        free(u->tests);
    if (u->callbackids)
        free(u->callbackids);
    free(u);
}

static int test_teardown(void** state)
{
    free(*state);
    return 0;
}
static void test_func(void** state)
{
    CallbackInfo* ci = (CallbackInfo*)(*state);
    FeatureInvokeCallback(ci->handle, ci->id);
    FeatureRemoveCallback(ci->handle, ci->id);
}

static int addTestSuite(UnitTests* u, FeatureInstanceHandle feature,
    FtCallbackId body)
{
    if (u->len >= u->cap) {
        // (todo : expend list)
        return -2;
    }
    CallbackInfo* ci = (CallbackInfo*)malloc(sizeof(CallbackInfo));
    if (!ci)
        return -2;
    ci->handle = feature;
    ci->id = body;

    u->tests[u->len++] = cmocka_unit_test_prestate_setup_teardown(
        test_func, NULL, test_teardown, ci);
    u->tests[u->len] = { NULL };
    return 0;
}

#ifdef __NuttX__
void FreeInstance(void* instance)
{
    if (instance) {
        UnitTestMap* p = static_cast<UnitTestMap*>(instance);
        delete p;
    }
}
#endif

static UnitTestMap* getUnitTestMap()
{
#ifdef __NuttX__
    static int index = -1;
    UnitTestMap* instance;
    if (index < 0) {
        index = task_tls_alloc(FreeInstance);
    }
    if (index >= 0) {
        instance = (UnitTestMap*)task_tls_get_value(index);
        if (instance == NULL) {
            instance = new UnitTestMap;
            if (instance) {
                task_tls_set_value(index, reinterpret_cast<uintptr_t>(instance));
            }
        }
        return instance;
    }
    assert(false);
    return nullptr;
#else
    static UnitTestMap instance;
    return &instance;
#endif
}

static inline UnitTests* getUnitTest(FeatureInstanceHandle feature)
{
    UnitTests* u = nullptr;
    if (getUnitTestMap()->find(feature) != getUnitTestMap()->end()) {
        u = (*getUnitTestMap())[feature];
    }
    return u;
}

void mockatest_onRegister(const char* module_name)
{
    printf("register module %s\n", module_name);
}

void mockatest_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("create module mockatest\n");
}

void mockatest_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    printf("required module cmocka %p\n", handle);
    assert(getUnitTestMap()->find(handle) == getUnitTestMap()->end());
    UnitTests* u = CreateUnitTests();
    printf("create unitest: %p\n", u);
    (*getUnitTestMap())[handle] = u;
}

void mockatest_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    printf("detached mockatest\n");

    UnitTests* u = getUnitTest(handle);
    if (u) {
        DeleteUnitTests(u);
        getUnitTestMap()->erase(handle);
    }
}

void mockatest_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("destroy mockatest\n");
    assert(getUnitTestMap()->size() == 0);
}

void mockatest_onUnregister(const char* module_name)
{
    printf("unregister %s\n", module_name);
}

void mockatest_wrap_test(FeatureInstanceHandle feature, AppendData data,
    FtString test_suit_name, FtString test_case_name,
    FtCallbackId body)
{
    printf("mockatest: register test case handle %p, callbackid: %d\n", feature,
        body);
    UnitTests* u = getUnitTest(feature);

    assert(u != nullptr);
    if (!u)
        return;
    addTestSuite(u, feature, body);
}

void mockatest_wrap_expect_true(FeatureInstanceHandle feature, AppendData data,
    FtBool result, FtString message_info)
{
    // file name and line num need to be settle
    printf("mockatest_wrap_expect_true: %d %s\n", result, message_info);
    // assert_true(result);
    mock_assert(result, message_info, "", 0);
}

void mockatest_wrap_runAllOnce(FeatureInstanceHandle feature, AppendData data)
{
    printf("mockatests: run all tests, handle %p\n", feature);
    UnitTests* u = getUnitTest(feature);
    _cmocka_run_group_tests("test test", u->tests, u->len, NULL, NULL);
}
