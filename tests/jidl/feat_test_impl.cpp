#include <gtest/gtest.h>
#include <stdio.h>

#include "feat_test.h"

#define EXPECT_TRUE_FILE(condition, file, line)                                             \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                                           \
    if (const ::testing::AssertionResult gtest_ar_ = ::testing::AssertionResult(condition)) \
        ;                                                                                   \
    else                                                                                    \
        GTEST_MESSAGE_AT_(file, line,                                                       \
            ::testing::internal::GetBoolAssertionFailureMessage(                            \
                gtest_ar_, #condition, "false", "true")                                     \
                .c_str(),                                                                   \
            ::testing::TestPartResult::kNonFatalFailure)

class FeatureUnittest {
public:
    FeatureUnittest()
        : executed_(false)
    {
    }
    static int generateAsyncId() { return ++next_async_id; }
    static int currentAsyncId() { return current_async_id; }
    static void setCurrentAsyncId(int aid) { current_async_id = aid; }
    ~FeatureUnittest()
    {
        setCurrentAsyncId(0);
        next_async_id = 0;
    }

    int runAllTests()
    {
        if (executed_)
            return -1;
        int argc = 1;
        const char* argv[] = { "feature google test" };
        ::testing::InitGoogleTest(&argc, const_cast<char**>(argv));
        int r = RUN_ALL_TESTS();
        executed_ = true;
        if (r) {
            FEATURE_LOG_WARN("feat_test: Test failed");
        }
        return r;
    }

private:
    static int current_async_id;
    static int next_async_id;

    bool executed_;
};
int FeatureUnittest::current_async_id = 0;
int FeatureUnittest::next_async_id = 0;

typedef int (*LoopFunc)(void*);
typedef struct FeatTestEnv {
    const char* filename;
    LoopFunc run_loop;
    LoopFunc stop_loop;
} FeatTestEnv;

class FeatureTest : public ::testing::Test {
public:
    FeatureTest(FeatureInstanceHandle featureInstance, FtCallbackId cb,
        bool is_async, int async_id)
        : _featureInstance(featureInstance)
        , _cb(cb)
        , _is_async(is_async)
        , _async_id(async_id)
    {
    }
    void TestBody() override
    {
        FeatureInvokeCallback(_featureInstance, _cb);
        if (!_is_async)
            return;

        // deal with async test
        FeatureUnittest::setCurrentAsyncId(_async_id);

        FeatTestEnv* pack = (FeatTestEnv*)FeatureInstanceGetManagerUserData(_featureInstance, "run_loop");

        LoopFunc run_loop = pack->run_loop;

        if (run_loop) {
            int ret = run_loop(pack);
            if (ret) {
                EXPECT_TRUE_FILE(false, pack->filename, -1)
                    << "  This test case timed out. id: " << _async_id;
            }
        }
        FeatureUnittest::setCurrentAsyncId(0);
    }

private:
    FeatureInstanceHandle _featureInstance;
    FtCallbackId _cb;
    bool _is_async;
    int _async_id;
};

class FTTestFactory : public ::testing::internal::TestFactoryBase {
public:
    FTTestFactory(FeatureInstanceHandle handle, FtCallbackId cb, bool is_async,
        int async_id)
        : _featureInstance(handle)
        , _cb(cb)
        , _is_async(is_async)
        , _async_id(async_id)
    {
    }

    ::testing::Test* CreateTest() override
    {
        return new FeatureTest(_featureInstance, _cb, _is_async, _async_id);
    }

private:
    FeatureInstanceHandle _featureInstance;
    FtCallbackId _cb;
    bool _is_async;
    int _async_id;
};

void feat_test_onRegister(const char* module_name)
{
    FEATURE_LOG_DEBUG("register module %s", module_name);
}

void feat_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("create module feat_test");
}

void feat_test_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    static int inited = 0;
    FEATURE_LOG_DEBUG("required module feat_test %p", handle);
    if (inited) {
        FEATURE_LOG_DEBUG("You can't require feat_test twice %p", handle);
        return;
    }

    FeatureUnittest* p = new FeatureUnittest;
    FeatureSetObjectData(handle, p);
    return;
}

void feat_test_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_DEBUG("detached feat_test %p", handle);
    FeatureUnittest* p = static_cast<FeatureUnittest*>(FeatureGetObjectData(handle));
    FeatureSetObjectData(handle, 0);
    if (p)
        delete p;
}

void feat_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("destroy feat_test");
    testing::UnitTest::ClearTestSuitesAndIndices();
}

void feat_test_onUnregister(const char* module_name)
{
    FEATURE_LOG_DEBUG("unregister %s", module_name);
}

void feat_test_wrap_done(FeatureInstanceHandle feature, AppendData append_data,
    FtInt async_id, FtInt err, FtString err_message)
{
    if (FeatureUnittest::currentAsyncId() != async_id) {
        FEATURE_LOG_DEBUG("[feat_test] done(%d) in odd test context: Timeout occurred.",
            async_id);
        return;
    }

    // FEATURE_LOG_DEBUG("[feat_test] done stop a async test: id(%d)", async_id);
    FeatTestEnv* pack = (FeatTestEnv*)FeatureInstanceGetManagerUserData(feature, "run_loop");
    LoopFunc stop_loop = pack->stop_loop;
    stop_loop(pack);
}
FtInt feat_test_wrap_testsuite(FeatureInstanceHandle feature,
    AppendData append_data, FtString test_suit_name,
    FtString test_case_name, FtCallbackId body,
    FtBool is_async)
{
    FEATURE_LOG_DEBUG("[feat_test] add testsuite %p", feature);

    int async_id = 0;
    if (is_async)
        async_id = FeatureUnittest::generateAsyncId();

    ::testing::internal::MakeAndRegisterTestInfo(
        test_suit_name, test_case_name, NULL, NULL,
        ::testing::internal::CodeLocation("", 0), // Location need more accurancy
        ::testing::internal::GetTypeId<FeatureTest>(),
        ::testing::internal::SuiteApiResolver<FeatureTest>::GetSetUpCaseOrSuite(
            "", 0),
        ::testing::internal::SuiteApiResolver<
            FeatureTest>::GetTearDownCaseOrSuite("", 0),
        new FTTestFactory(feature, body, is_async, async_id));

    return async_id;
}

void feat_test_wrap_expect_true(FeatureInstanceHandle feature, AppendData data,
    FtBool result, FtString message_info)
{
    // TODO 判断是否要执行
    FeatTestEnv* pack = (FeatTestEnv*)FeatureInstanceGetManagerUserData(feature, "run_loop");
    EXPECT_TRUE_FILE(result, pack->filename, -1) << message_info;
}

void feat_test_wrap_run_all_tests(FeatureInstanceHandle feature,
    AppendData data)
{
    FEATURE_LOG_DEBUG("feat_test runAllTests %p", feature);
    FeatureUnittest* p = static_cast<FeatureUnittest*>(FeatureGetObjectData(feature));
    p->runAllTests();
}

void feat_test_wrap_init_suit_filter(FeatureInstanceHandle feature,
    AppendData data, FtString test_suit_filter)
{
    ::testing::GTEST_FLAG(filter) = test_suit_filter;
}