#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <set>

#include "application.h"
#include "builtin/builtin_console.h"
#include "builtin/console.h"
#include "feature.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_main_exports.h"
#include "feature_qjs_exports.h"
#include "feature_registry.h"
#if defined(CONFIG_ANDROID_BINDER) && defined(CONFIG_ANDROID_SERVICEMANAGER)
#include <binder/IPCThreadState.h>
#endif

using namespace feature_framework;

extern FeatureRegistryTableHandle g_ajs_features_registry;

namespace {
class TestNavigator : public Navigator {
public:
    TestNavigator() {};
    ~TestNavigator() {};
    int replace(const RouteInfo& route) { return 0; }
    int push(const RouteInfo& route) { return 0; }
    int back(const std::string& path = "") { return 0; }
    void clear() { }
    int getState(NavigatorInfo* info) { return 0; }
    int getStateByIndex(NavigatorInfo* info, int index) { return 0; }
    int getLength() { return 0; }
    void show() { }
    void hide() { }
    void clearAll() { }
    int getRouteInfoFromUri(RouteInfo* info, const char* uri) { return 0; }
};

class TestApplication {
public:
    TestApplication()
    {
        navigator_ = std::make_unique<TestNavigator>();
    };
    ~TestApplication() {};
    bool init(AIOTJS::CLIParsedArgument* args, uv_loop_t* loop) { return true; }
    void initOnUI(lv_obj_t* root_widget, uv_loop_t* main_loop) { }
    void destroyOnUI() { }
    void thread_memory_status() { }
    void run() { }
    void stop() { }
    void show() { }
    void hide() { }
    void destroy() { }
    AIOTJS::RuntimeContext* runtime() { return nullptr; }
    void setRuntime(AIOTJS::RuntimeContext* runtime) { }
    std::shared_ptr<Page> page() { return nullptr; }
    std::shared_ptr<AppManifest> getAppManifest() { return nullptr; }
    void evalScript(const char* code) { }
    Navigator* navigator() const { return static_cast<Navigator*>(navigator_.get()); }
    void callHook(const char* hook) { }
    const char* packageName() { return ""; }
    const char* packagePath() { return ""; }
    uv_thread_t& threadId() { return thread_; }
    int state() { return 0; }
    void setState(int state) { }
    // TODO
    //AIOTJS::WidgetContextHandle widgetContext() { return nullptr; }
    void clearRuntime() { }
    void postAppNotify(ApplicationNotifyType type) { }
    bool isExitRequest() { return true; }
    const char* appName() { return "testApp"; }
    void setExitRequested() { }
    bool isAsyncMode() { return true; }
    void notifyEvent(int evt) { }
    AIOTJS::CLIParsedArgument* getCLIArgument() { return nullptr; }
    void setCLIArgument(AIOTJS::CLIParsedArgument* args) { }
    void* getXmsContext() const { return nullptr; }
    void setXmsContext(void* xms_context) { }
    std::chrono::steady_clock::time_point getHideTime() { return std::chrono::steady_clock::time_point(); }
    // TODO
    //ProcessStatus getProcessStatus() { return UNKNOWN; };
    void setThreadMemory(int mem) { }
    int getThreadMemory() { return 0; }
    void initMemoryStatusTimer() { }
    void stopMemoryStatusTimer() { }
    std::string getAppPriority() { return ""; }
    bool route(const char* uri) { return true; }
    void* getDebugHandler() { return nullptr; }
    void onError(void* param) {};

private:
    std::unique_ptr<TestNavigator> navigator_;
    uv_thread_t thread_ { 0 };
};
}

bool load_file(const char* file_name, char** file_content);

#define TIME_LIMIT 2000 // 异步限时 2000ms

struct TimeoutHost;
struct TimeCallback {
    TimeoutHost* host;
    uv_timer_t* timer;
    JSContext* ctx;
    JSValue callback;
    bool triggered;
};

struct TimeoutHost {
    uv_loop_t* loop;
    std::set<TimeCallback*> timers;
};

typedef int (*LoopFunc)(void*);
typedef struct FeatTestEnv {
    const char* filename;
    LoopFunc run_loop;
    LoopFunc stop_loop;
    FeatureManagerHandle manager;
    TimeoutHost time_host;
    uv_timer_t* async_limiter;
    uint32_t time_limit;
    JSRuntime* rt;
    JSContext* ctx;
} FeatTestEnv;

static bool feat_test_args_error_cb(void* data, ArgsErrorInfo* error_info)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s: runtime context is null!", __func__);
        return false;
    }
    void* qrt_ctx = data;
    if (!error_info) {
        FEATURE_LOG_ERROR("%s: error_info is null!", __func__);
        return false;
    }
    for (int i = 0; i < error_info->argc; ++i) {
        feature_value_t arg = *((feature_value_t*)(error_info->argv) + i);
        if (feature_is_undefined(arg)) {
            AIOTJS_LOG_ERROR("%s: arg %d is undefined!", __func__, i);
            return false;
        }
        if (feature_is_object(arg)) {
            feature_value_t fail_cb = feature_get_object_property(qrt_ctx, arg, "fail");
            if (feature_is_undefined(fail_cb))
                continue;

            AIOTJS_LOG_INFO("%s: found fail callback from arg %d!", __func__, i);
            feature_value_t argv[2];
            argv[0] = feature_string(qrt_ctx, error_info->error_msg);
            argv[1] = feature_int(qrt_ctx, error_info->error_code);
            feature_value_t ret = feature_call(qrt_ctx, fail_cb, FEATURE_UNDEFINED, 2, argv);
            feature_free_value(qrt_ctx, ret);
            feature_free_value(qrt_ctx, fail_cb);
            feature_free_value(qrt_ctx, argv[0]);

            feature_value_t complete_cb = feature_get_object_property(qrt_ctx, arg, "complete");
            if (feature_is_undefined(complete_cb)) {
                AIOTJS_LOG_WARN("%s: no complete callback from arg %d!", __func__, i);
                return true;
            }
            ret = feature_call(qrt_ctx, complete_cb, FEATURE_UNDEFINED, 0, NULL);
            feature_free_value(qrt_ctx, ret);
            feature_free_value(qrt_ctx, complete_cb);
            return true;
        }
    }
    return false;
}

// __require
JSValue __require(JSContext* ctx, JSValue this_val, int argc, JSValue* argv,
    int magic, JSValue* func_data)
{
    if (argc < 1) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "require need module name!");
        return FEATURE_UNDEFINED;
    }

    FeatureManagerHandle manager = static_cast<FeatureManagerHandle>(JS_GetOpaque(func_data[0], 1));
    ft_context_ref ft_ctx = FeatureManagerGetContext(manager);
    ft_value_t ft_vm_obj = ft_from_jsvalue(ft_ctx, JS_UNDEFINED);
    const char* module_name = JS_ToCString(ctx, argv[0]);
    ft_value_t ft_obj = FeatureRequire(manager, ft_vm_obj, module_name);
    auto js_obj = ft_to_jsvalue(ft_ctx, ft_obj);
    JS_FreeCString(ctx, module_name);
    return js_obj;
}

/**
 * setTimeout 的回调函数
 */
void timeout_callback(uv_timer_t* handle)
{
    TimeCallback* tc = static_cast<TimeCallback*>(handle->data);
    JS_Call(tc->ctx, tc->callback, JS_UNDEFINED, 0, NULL);
    JS_FreeValue(tc->ctx, tc->callback);
    tc->triggered = true;
    handle->data = 0;
    uv_timer_stop(handle);
    uv_close((uv_handle_t*)handle, NULL);
}

// setTimeout
JSValue __setTimeout(JSContext* ctx, JSValue this_val, int argc, JSValue* argv,
    int magic, JSValue* func_data)
{
    if (argc < 2) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "setTimeout need a callback and time!");
        return FEATURE_UNDEFINED;
    }
    TimeoutHost* time_host = (TimeoutHost*)JS_GetOpaque(func_data[0], 1);
    int t = JS_VALUE_GET_INT(argv[1]);
    TimeCallback* tc = (TimeCallback*)malloc(sizeof(TimeCallback));
    tc->callback = JS_DupValue(ctx, argv[0]);
    tc->triggered = false;
    tc->host = time_host;
    tc->ctx = ctx;
    time_host->timers.insert(tc);
    uv_timer_t* timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    tc->timer = timer;
    uv_timer_init(time_host->loop, timer);
    timer->data = tc;
    uv_timer_start(timer, timeout_callback, t, 0);
    return JS_UNDEFINED;
}

static void execute_job_cb(uv_prepare_t* handle)
{
    FeatTestEnv* env = static_cast<FeatTestEnv*>(handle->data);
    JSContext* r_ctx;
    int err;
    for (;;) {
        err = JS_ExecutePendingJob(env->rt, &r_ctx);
        if (err <= 0) {
            if (err < 0)
                feature_dump_error(r_ctx);
            break;
        }
    }
}

/**
 * 异步测试定时的超时回调
 */
static void async_limit_cb(uv_timer_t* handle)
{
    FeatTestEnv* env = static_cast<FeatTestEnv*>(handle->data);
    uv_loop_t* ploop = FeatureGetUVLoop(env->manager);
    uv_stop(ploop);
}

/**
 * 如果超时退出，返回 1
 * 如果调用 done() 退出，返回 0
 */
static int run_loop(void* feat_test_env)
{
    FeatTestEnv* env = static_cast<FeatTestEnv*>(feat_test_env);
    uv_timer_t* async_timer = static_cast<uv_timer_t*>(env->async_limiter);
    uv_loop_t* ploop = FeatureGetUVLoop(env->manager);
    uv_timer_start(async_timer, async_limit_cb, env->time_limit, 0);
    uv_run(ploop, UV_RUN_DEFAULT);

    /**
     * 代码到这里，uvloop 已经结束。这里有两种情况：
     * 一种是 调用 done 函数，由 stop_loop 调用
     * 一种是 async_limit_timer 的回调
     * async_timer 执行过后会直接被关闭
     * 因此 当 async_timer 是激活状态的话，一定是由于 done 函数，返回 0
     * 其他情况属于超时，返回 1
     */
    if (uv_is_active((uv_handle_t*)async_timer)) {
        uv_timer_stop(async_timer); // 异步测试正常结束
        return 0;
    } else {
        return 1; // 异步测试执行超时
    }
}

static int stop_loop(void* feat_test_env)
{
    FeatTestEnv* env = static_cast<FeatTestEnv*>(feat_test_env);
    uv_loop_t* ploop = FeatureGetUVLoop(env->manager);
    uv_stop(ploop);
    return 0;
}

static JSValue getRequireObject(FeatTestEnv* env)
{
    JSValue func_data = JS_NewObject(env->ctx);
    JS_SetOpaque(func_data, env->manager);
    JSValue ret = JS_NewCFunctionData(env->ctx, __require, 1, 0, 1, &func_data);
    JS_FreeValue(env->ctx, func_data);
    return ret;
}

static JSValue getTimeoutObject(FeatTestEnv* env)
{
    JSValue func_data = JS_NewObject(env->ctx);
    JS_SetOpaque(func_data, &env->time_host);
    JSValue ret = JS_NewCFunctionData(env->ctx, __setTimeout, 1, 0, 1, &func_data);
    JS_FreeValue(env->ctx, func_data);
    return ret;
}

#if defined(CONFIG_ANDROID_BINDER) && defined(CONFIG_ANDROID_SERVICEMANAGER)
static void __uv_poll_cb(uv_poll_t* handle, int status, int events)
{
    android::IPCThreadState::self()->handlePolledCommands();
}
#endif

void feat_test_once(char* js_file, char* js_str, const char* test_all, char* package_name, int time_limit)
{
    FeatTestEnv env;
    env.filename = js_file;
    // initialize quickjs engine
    env.rt = JS_NewRuntime();
    env.ctx = JS_NewContext(env.rt);

    // 初始化
    uv_loop_t* main_loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    uv_loop_init(main_loop);
    uv_prepare_t prepare;
    uv_prepare_init(main_loop, &prepare);
    uv_timer_t timer;
    uv_timer_init(main_loop, &timer);

    prepare.data = &env;
    uv_prepare_start(&prepare, execute_job_cb);

    env.async_limiter = &timer;
    env.time_limit = time_limit;
    timer.data = &env;

    env.time_host.loop = main_loop;
#if defined(CONFIG_ANDROID_BINDER) && defined(CONFIG_ANDROID_SERVICEMANAGER)
    // init binder
    int binderFd = -1;
    uv_poll_t binder_poll;
    android::IPCThreadState::self()->setupPolling(&binderFd);
    int dupFd = dup(binderFd);
    if (binderFd < 0) {
        FEATURE_LOG_ERROR("failed to open binder device:%d", errno);
    } else {
        uv_poll_init(main_loop, &binder_poll, dupFd);
        binder_poll.data = &dupFd;
        uv_poll_start(&binder_poll, UV_READABLE, __uv_poll_cb);
    }
#endif

    // init feature framework
    // TODO: use factory pattern: manager = CreateFeatureManager(registry, "js");
    FeatureManagerCreateInfo ft_info;
    ft_info.raw_ctx = (FeatureRawContextHandle)(env.ctx);
    ft_info.release_cb = nullptr;
    ft_info.manager_type = FEATURE_MANAGER_JS;
    ft_info.package_name = package_name;
    FeatureManagerHandle manager = FeatureCreateManager(&ft_info);
    FeatureRegistryHandle hRegistry = FeatureGetRegistryFromManager(manager);
    FeatureRegisterFeatures(hRegistry, g_ajs_features_registry);

    env.manager = manager;
    env.run_loop = run_loop;
    env.stop_loop = stop_loop;

    TestApplication* app = new TestApplication();
    FeatureSetManagerUserData(manager, "app", app);

    FeatureSetManagerUserData(manager, "run_loop", &env);
    FeatureSetUVLoop(manager, main_loop);
    FeatureSetArgsErrorCb(manager, feat_test_args_error_cb, env.ctx);
    // register global require
    JSValue global_obj = JS_GetGlobalObject(env.ctx);
    JSValue require = getRequireObject(&env);
    JS_SetPropertyStr(env.ctx, global_obj, "require", require);

    JSValue setTimeout = getTimeoutObject(&env);
    JS_SetPropertyStr(env.ctx, global_obj, "setTimeout", setTimeout);

    JS_FreeValue(env.ctx, global_obj);

    // add console
    builtin::addConsoleModule(env.ctx, "console.js", builtin::CONSOLE_JS);
    // 加载 test frame work

    // TODO: ues qjs bytecode
    // original file ../test-internal.js
    const char* test_content = "let unittest = require('feat_test');\n\nfunction feat_test(name, desc, cb) "
                               "{\n    unittest.testsuite(name, desc, cb);\n}\n\nfunction "
                               "feat_async_test(suitname, desc, test_cb) {\n    var async_id;\n    async_id = "
                               "unittest.testsuite(suitname, desc, () => {\n        test_cb(() => "
                               "unittest.done(async_id));\n    }, true);\n}\n\nfunction __feat_test_all() { "
                               "// hide to outside\n    unittest.run_all_tests();\n}\n\nfunction "
                               "feat_expect_true(r, d) {\n    return unittest.expect_true(r, d);\n}\n\n"
                               "function init_feat_filter(filter) {\n   unittest.init_suit_filter(filter);\n}\n\n"
                               "function almostEqualFloat(a, b, epsilon) {\n    if (Math.abs(a - b) <= epsilon) {\n"
                               "    return true;\n}\n   const absA = Math.abs(a);\n     const absB = Math.abs(b);\n"
                               "    const diff = Math.abs(a - b);\n    return diff <= (Math.max(absA, absB) * epsilon);\n}\n\n"
                               "function arraysEqual(arr1, arr2) {\n    if (arr1.length !== arr2.length) return false;\n"
                               "    for (let i = 0; i < arr1.length; i++) {\n        if (arr1[i] !== arr2[i]) return false;\n"
                               "    }\n    return true;\n}\n";

    auto res = JS_Eval(env.ctx, test_content, strlen(test_content), "test-internal.js",
        JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
    if (JS_IsException(res)) {
        const char* str = JS_ToCString(env.ctx, res);
        FEATURE_LOG_ERROR("[feat_test]: Exception in initializing test internal interface.: %s\n", str);
        feature_dump_error(env.ctx);
        JS_FreeValue(env.ctx, res);
        goto feat_test_done;
    }
    JS_FreeValue(env.ctx, res);

    // 加载 测试文件
    res = JS_Eval(env.ctx, js_str, strlen(js_str), js_file,
        JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);

    if (JS_IsException(res)) {
        const char* str = JS_ToCString(env.ctx, res);
        FEATURE_LOG_ERROR("[feat_test]: Exception thrown while executing test file \"%s\": %s\n", js_file, str);
        feature_dump_error(env.ctx);
        JS_FreeValue(env.ctx, res);
        goto feat_test_done;
    }
    JS_FreeValue(env.ctx, res);

    res = JS_Eval(env.ctx, test_all, strlen(test_all), "run_test.js",
        JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);

    if (JS_IsException(res)) {
        const char* str = JS_ToCString(env.ctx, res);
        FEATURE_LOG_ERROR("[feat_test]: Exception thrown while running all test, \"%s\"\n", str);
        feature_dump_error(env.ctx);
        JS_FreeValue(env.ctx, res);
        goto feat_test_done;
    }
    JS_FreeValue(env.ctx, res);

feat_test_done:
    // release manager first
    FeatureUnsetUVLoop(manager);
    FeatureUninit(manager);
    FeatureFreeManager(manager);

    for (TimeCallback* tc : env.time_host.timers) {
        if (!tc->triggered) {
            JS_FreeValue(tc->ctx, tc->callback);
            uv_close((uv_handle_t*)tc->timer, NULL);
        }
        free(tc->timer);
        tc->timer = NULL;

        free(tc);
        tc = NULL;
    }
    env.time_host.timers.clear();

    uv_close((uv_handle_t*)&prepare, NULL);
    uv_close((uv_handle_t*)&timer, NULL);

#if defined(CONFIG_ANDROID_BINDER) && defined(CONFIG_ANDROID_SERVICEMANAGER)
    uv_close((uv_handle_t*)&binder_poll, [](uv_handle_t* handle) {
        int* fdPtr = static_cast<int*>(handle->data);
        if (fdPtr && *fdPtr >= 0) {
            close(*fdPtr);
        }
    });
#endif

    int closed = 0;
    for (int i = 0; i < 200; i++) {
        if (uv_loop_close(main_loop) == 0) {
            closed = 1;
            break;
        }
        uv_run(main_loop, UV_RUN_NOWAIT);
    }

    if (!closed) {
        FILE* fp = fopen("/dev/log", "wb");
        fp = fp ? fp : stderr;
        uv_print_all_handles(main_loop, fp);
        if (fp != stderr) {
            fclose(fp);
        }
        // assert directly if we can't stop uv loop successfuly.
        assert(0);
    }
    if (main_loop)
        free(main_loop);
    JS_FreeContext(env.ctx);
    JS_FreeRuntime(env.rt);
    delete app;
    return;
}

// 支持cli来读取 js 文件去执行，命令为：./feature_jidl_test
extern "C" int main(int argc, char** argv)
{
    if (argc < 2) {
        FEATURE_LOG_DEBUG("please input js file, like ./test.js \n");
        return 0;
    }

    int time_limit = TIME_LIMIT;
    int times = 1;

    const char* test_all = "__feat_test_all();";
    char* js_file = NULL;
    char* js_str = NULL;
    char* package_name = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            i++;
            if (i >= argc)
                break;
            time_limit = atoi(argv[i]);
        } else if (strcmp(argv[i], "-times") == 0) {
            i++;
            if (i >= argc)
                break;
            times = atoi(argv[i]);
        } else if (strcmp(argv[i], "-package_name") == 0) {
            i++;
            if (i >= argc)
                break;
            package_name = argv[i];
        } else {
            js_file = argv[i];
        }
    }

    FEATURE_LOG_DEBUG("[feat_test]: Time limit of asynchronous test execution: %d\n", time_limit);

    // 打开js文件
    load_file(js_file, &js_str);
    if (js_str == NULL) {
        FEATURE_LOG_DEBUG("malloc js file failed!\n");
        return 0;
    }
    // 打开 manifest 文件

    for (int i = 0; i < times; i++) {
        FEATURE_LOG_DEBUG("[feat_test]:  the number of times you want to repeat the test is %d, Current number of tests is %d\n", times, i);
        feat_test_once(js_file, js_str, test_all, package_name, time_limit);
    }

    // free js_str
    free(js_str);

    return 0;
}

bool load_file(const char* file_name, char** file_content)
{
    if (file_name == NULL || file_content == NULL) {
        FEATURE_LOG_DEBUG("file_name or file_content is NULL!\n");
        return false;
    }

    FILE* fp = fopen(file_name, "r");
    if (fp == NULL) {
        FEATURE_LOG_DEBUG("open file_name is %s failed!\n", file_name);
        return false;
    }
    // 获取文件长度
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *file_content = (char*)malloc(len + 1);
    memset(*file_content, 0, len + 1);
    // 读取文件内容到file_content字符串中
    fread(*file_content, len, 1, fp);
    fclose(fp);

    return true;
}
