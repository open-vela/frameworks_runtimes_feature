/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_SYSTEM_ACTIVITY_SERVICE
#include <binder/IPCThreadState.h>
#endif
#include "feature.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_permission.h"
#include "feature_qjs_exports.h"
#include "feature_registry.h"
#include "feature_utils.h"

#ifdef CONFIG_FEATURE_RUST_MODULES
#include "internal/feature_rust.h"
#endif

#define CLI_TIME_LIMIT 2 // 异步限时 2000ms
using namespace feature_framework;

static FeatureManagerHandle g_manager;
extern FeatureRegistryTableHandle g_ajs_features_registry;

struct cli_timeout_host;

typedef struct {
    cli_timeout_host* host;
    uv_timer_t* timer;
    JSContext* ctx;
    JSValue callback;
    bool triggered;
} cli_time_callback;

struct cli_timeout_host {
    uv_loop_t* loop;
    std::set<cli_time_callback*> timers;
};

typedef struct {
    cli_timeout_host time_host;
    uv_timer_t* async_limiter;
    uint32_t time_limit;
    JSRuntime* rt;
    JSContext* ctx;
} feature_env_t;

/** setTimeOut 回调 */
void cli_time_cb(uv_timer_t* handle)
{
    cli_time_callback* tc = static_cast<cli_time_callback*>(handle->data);
    JS_Call(tc->ctx, tc->callback, JS_UNDEFINED, 0, NULL);
    JS_FreeValue(tc->ctx, tc->callback);
    tc->triggered = true;
    handle->data = 0;
    uv_timer_stop(handle);
    uv_close((uv_handle_t*)handle, NULL);
}

static void setScriptArgs(JSContext* ctx, JSValue global_obj, int argc, char* argv[], int scriptArgs_beg)
{
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0, j = scriptArgs_beg; j < argc; i++, j++) {
        JSValue js_string = JS_NewString(ctx, argv[j]);
        JS_SetPropertyUint32(ctx, arr, i, js_string);
    }
    JS_SetPropertyStr(ctx, global_obj, "scriptArgs", arr);
}

// __require
JSValue __require(JSContext* ctx, JSValue this_val, int argc, JSValue* argv)
{
    if (argc < 1) {
        JS_ThrowInternalError(ctx, "require need module name!");
        return JS_UNDEFINED;
    }

    const char* module_name = JS_ToCString(ctx, argv[0]);
    ft_context_ref ft_ctx = FeatureManagerGetContext(g_manager);
    ft_value_t ft_vm_obj = ft_from_jsvalue(ft_ctx, JS_UNDEFINED);
    ft_value_t ft_obj = FeatureRequire(g_manager, ft_vm_obj, module_name);
    auto js_obj = ft_to_jsvalue(ft_ctx, ft_obj);
    JS_FreeCString(ctx, module_name);
    return js_obj;
}

// console_log
JSValue __log(JSContext* ctx, JSValue this_val, int argc, JSValue* argv)
{
    int i;
    const char* str;
    std::string buff;
    for (i = 0; i < argc; i++) {
        if (i != 0)
            buff += ' ';
        str = JS_ToCString(ctx, argv[i]);
        if (str) {
            buff += str;
        } else { // exception
            buff += "[custom object]";
        }

        JS_FreeCString(ctx, str);
    }
    FEATURE_LOG_INFO("%s", buff.c_str());
    return JS_UNDEFINED;
}

JSValue __setCliTimeout(JSContext* ctx, JSValue this_val, int argc, JSValue* argv,
    int magic, JSValue* func_data)
{
    if (argc < 2) {
        JS_ThrowInternalError(ctx, "setTimeout need a callback and time!");
        return JS_UNDEFINED;
    }
    cli_timeout_host* time_host = (cli_timeout_host*)JS_GetOpaque(func_data[0], 1);
    int t = JS_VALUE_GET_INT(argv[1]);
    cli_time_callback* tc = (cli_time_callback*)malloc(sizeof(cli_time_callback));
    tc->callback = JS_DupValue(ctx, argv[0]);
    tc->triggered = false;
    tc->host = time_host;
    tc->ctx = ctx;
    tc->host->timers.insert(tc);
    uv_timer_t* timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    tc->timer = timer;
    uv_timer_init(tc->host->loop, timer);
    timer->data = tc;
    uv_timer_start(timer, cli_time_cb, t, 0);
    return JS_UNDEFINED;
}

// exit
JSValue __cliExit(JSContext* ctx, JSValue this_val, int argc, JSValue* argv)
{
    uv_loop_t* ploop = FeatureGetUVLoop(g_manager);
    uv_stop(ploop);
    FEATURE_LOG_INFO("feature_cli_test exit!");
    return JS_UNDEFINED;
}

bool cli_load_file(const char* file_name, char** file_content)
{
    if (file_name == NULL || file_content == NULL) {
        FEATURE_LOG_ERROR("file_name or file_content is NULL!\n");
        return false;
    }

    FILE* fp = fopen(file_name, "r");
    if (fp == NULL) {
        FEATURE_LOG_ERROR("open file_name is %s failed!\n", file_name);
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

static JSModuleDef* js_module_loader(JSContext* ctx, const char* name, void* opaque)
{
    char* js_str = NULL;
    cli_load_file(name, &js_str);
    if (!js_str) {
        JS_ThrowReferenceError(ctx, "read module file failed, name: '%s'", name);
        return NULL;
    }

    JSValue val = JS_Eval(ctx, js_str, strlen(js_str),
        name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    free(js_str);
    if (JS_IsException(val)) {
        JS_ThrowReferenceError(ctx, "Eval module failed,  name: '%s'", name);
        return NULL;
    }
    return (JSModuleDef*)JS_VALUE_GET_PTR(val);
}

static void cli_execute_job_cb(uv_prepare_t* handle)
{
    feature_env_t* env = static_cast<feature_env_t*>(handle->data);
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

static void cli_async_limit_cb(uv_timer_t* handle)
{
    uv_loop_t* ploop = FeatureGetUVLoop(g_manager);
    uv_stop(ploop);
}

#ifdef CONFIG_SYSTEM_ACTIVITY_SERVICE
static void __cli_uv_poll_cb(uv_poll_t* handle, int status, int events)
{
    android::IPCThreadState::self()->handlePolledCommands();
}
#endif

static bool on_feature_args_error(void* data, ArgsErrorInfo* error_info)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s: runtime context is null!", __func__);
        return false;
    }
    feature_env_t* env = static_cast<feature_env_t*>(data);
    if (!error_info) {
        FEATURE_LOG_ERROR("%s: error_info is null!", __func__);
        return false;
    }
    for (int i = 0; i < error_info->argc; ++i) {
        JSValue arg = *((JSValue*)(error_info->argv) + i);
        if (JS_IsUndefined(arg)) {
            FEATURE_LOG_ERROR("%s: arg %d is undefined!", __func__, i);
            return false;
        }
        if (JS_IsObject(arg)) {
            JSValue fail_cb = JS_GetPropertyStr(env->ctx, arg, "fail");
            if (JS_IsUndefined(fail_cb))
                continue;

            FEATURE_LOG_INFO("%s: found fail callback from arg %d!", __func__, i);
            JSValue argv[2];
            argv[0] = JS_NewString(env->ctx, error_info->error_msg);
            argv[1] = JS_NewInt32(env->ctx, error_info->error_code);
            JSValue ret = JS_Call(env->ctx, fail_cb, JS_UNDEFINED, 2, argv);
            JS_FreeValue(env->ctx, ret);
            JS_FreeValue(env->ctx, fail_cb);
            JS_FreeValue(env->ctx, argv[0]);

            JSValue complete_cb = JS_GetPropertyStr(env->ctx, arg, "complete");
            if (JS_IsUndefined(complete_cb)) {
                FEATURE_LOG_WARN("%s: no complete callback from arg %d!", __func__, i);
                return true;
            }
            ret = JS_Call(env->ctx, complete_cb, JS_UNDEFINED, 0, NULL);
            JS_FreeValue(env->ctx, ret);
            JS_FreeValue(env->ctx, complete_cb);
            return true;
        }
    }
    return false;
}

static char* on_uri_convert_cb(const char* package_name, const char* uri)
{
    if (!uri || !package_name) {
        FEATURE_LOG_ERROR("%s: uri or package_name is null!", __func__);
        return nullptr;
    }
    return strdup(uri);
}

/** FeaturePermissionsCb ptr */
static void permissions_cb(FeaturePermissionsHandle handle, const FeaturePermissionsInfo* info, void* data)
{
    FEATURE_LOG_INFO("wjf: permissions: %p, api_name: %s", handle, info->api_name);
    static bool granted = false;
    static FeaturePermsRejectReason reason = FEATURE_PERMS_DENIED;
    for (int i = 0; i <= HAPJS_PERMISSION_READ_HEALTH_DATA; ++i) {
        if (!HAS_PERMISSION(*(info->permissions), i))
            continue;
        FEATURE_LOG_INFO("wjf: got permission: %s", FeatureGetPermissionName((FeaturePermissionId)i));
    }

    if (granted) {
        FeatureGrantPermissions(g_manager, handle);
    } else {
        FeatureRejectPermissions(g_manager, handle, reason);
        reason = (FeaturePermsRejectReason)(reason + 1);
        if (reason > FEATURE_PERMS_NO_BG) {
            reason = FEATURE_PERMS_DENIED;
        }
    }
    granted = !granted;
}

// 当test.js使用异步任务, 命令为:./feature_test_cli -m 5 ./test.js
extern "C" int main(int argc, char** argv)
{
    if (argc < 2) {
        FEATURE_LOG_INFO("help: feature_test_cli js_file.js [manifest] [--scriptArgs ....]");
        return 0;
    }
    int time_limit = CLI_TIME_LIMIT;
    char* js_file = NULL;
    char* js_str = NULL;
    char* manifast_str = NULL;
    char* manifest_file = NULL;
    bool use_uvloop_async = false;
    int scriptArgs_beg = argc;
    int i = 1;
    for (; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            use_uvloop_async = true;
            i++;
            if (i >= argc)
                break;
            time_limit = atoi(argv[i]);
        } else if (strcmp(argv[i], "--scriptArgs") == 0) { // script after thie args will give js
            scriptArgs_beg = i;
            argv[i] = js_file;
            break;
        } else if (js_file) { // all args before --scriptArgs and after js_file will overwrite manifest_file
            manifest_file = argv[i];
        } else {
            js_file = argv[i];
        }
    }

    // manifest file is not required
    if (manifest_file) {
        cli_load_file(manifest_file, &manifast_str);
    }

    // 打开manifest.json文件,读取内容到一个字符串中
    // 打开js文件
    if (!js_file) {
        printf("feature_test_cli: js_file.js is required\n");
        return 1;
    }

    cli_load_file(js_file, &js_str);
    if (js_str == NULL) {
        printf("malloc js file failed!\n");
        if (manifast_str != NULL) {
            free(manifast_str);
            manifast_str = NULL;
        }
        return 0;
    }

    feature_env_t js_env;

    js_env.rt = JS_NewRuntime();
    js_env.ctx = JS_NewContext(js_env.rt);
    JS_SetRuntimeOpaque(js_env.rt, js_env.ctx);
    JS_SetModuleLoaderFunc(js_env.rt, NULL, js_module_loader, NULL);

    FeatureManagerCreateInfo ft_info;
    ft_info.raw_ctx = (FeatureRawContextHandle)(js_env.ctx);
    ft_info.release_cb = nullptr;
    ft_info.manager_type = FEATURE_MANAGER_JS;
    ft_info.package_name = "feature_test_cli";
    g_manager = FeatureCreateManager(&ft_info);
    FEATURE_CHECK_NE(g_manager, nullptr);
    FEATURE_LOG_INFO("created FeatureManagerHandle: %p", g_manager);
    // 初始化
    uv_loop_t* main_loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    uv_loop_init(main_loop);

#ifdef CONFIG_FEATURE_RUST_MODULES
    auto vdk_runtime = init_vdk_async_runtime(main_loop);
#endif

    uv_timer_t timer;
    uv_timer_init(main_loop, &timer);

    uv_prepare_t prepare;
    uv_prepare_init(main_loop, &prepare);

    prepare.data = &js_env;
    uv_prepare_start(&prepare, cli_execute_job_cb);

    js_env.async_limiter = &timer;
    js_env.time_limit = time_limit * 1000;
    timer.data = &js_env;

    js_env.time_host.loop = main_loop;
#ifdef CONFIG_SYSTEM_ACTIVITY_SERVICE
    // init binder
    int binderFd = -1;
    uv_poll_t binder_poll;
    android::IPCThreadState::self()->setupPolling(&binderFd);
    if (binderFd < 0) {
        printf("failed to open binder device:%d", errno);
    } else {
        uv_poll_init(main_loop, &binder_poll, binderFd);
        uv_poll_start(&binder_poll, UV_READABLE, __cli_uv_poll_cb);
    }
#endif

    // reigstry features
    FeatureRegistryHandle hRegistry = FeatureGetRegistryFromManager(g_manager);
    FeatureRegisterFeatures(hRegistry, g_ajs_features_registry);
    FeatureSetArgsErrorCb(g_manager, on_feature_args_error, &js_env);
    FeatureSetUriConvertCb(g_manager, on_uri_convert_cb);
    // FeatureSetManagerUserData(g_manager, "app", app);
    FeatureSetUVLoop(g_manager, main_loop);
    FeatureSetPermissionsCallback(g_manager, permissions_cb, NULL);

    JSValue global_obj = feature_global_object(js_env.ctx);
    JSValue console = feature_object(js_env.ctx);
    feature_set_object_property(js_env.ctx, global_obj, "console", console);

    JSValue require = feature_cfunction(js_env.ctx, __require, "require", 0);
    feature_set_object_property(js_env.ctx, global_obj, "require", require);

    JSValue log = feature_cfunction(js_env.ctx, __log, "console_log", 0);
    feature_set_object_property(js_env.ctx, console, "log", log);

    JSValue exit = feature_cfunction(js_env.ctx, __cliExit, "exit", 0);
    feature_set_object_property(js_env.ctx, global_obj, "exit", exit);

    JSValue time_func_data = feature_object(js_env.ctx);
    feature_set_opaque(time_func_data, &(js_env.time_host));
    JSValue setTimeout = feature_cfunctiondata(js_env.ctx, __setCliTimeout, 1, 0, 1, &time_func_data);
    feature_free_value(js_env.ctx, time_func_data);
    feature_set_object_property(js_env.ctx, global_obj, "setTimeout", setTimeout);

    setScriptArgs(js_env.ctx, global_obj, argc, argv, scriptArgs_beg);

    feature_free_value(js_env.ctx, global_obj);
    auto result = feature_eval(js_env.ctx, js_str, strlen(js_str), "<eval>", JS_EVAL_TYPE_MODULE);
    if (use_uvloop_async) {
        uv_timer_t* async_timer = static_cast<uv_timer_t*>(js_env.async_limiter);
        uv_timer_start(async_timer, cli_async_limit_cb, js_env.time_limit, 0);
        uv_run(main_loop, UV_RUN_DEFAULT);
        if (uv_is_active((uv_handle_t*)async_timer)) {
            uv_timer_stop(async_timer); // 异步测试正常结束
        }
    }
    if (feature_is_exception(result)) {
        feature_dump_error(js_env.ctx);
    }

    int err;
    JSContext* ctx1;
    while (!!JS_IsJobPending(js_env.rt)) {
        err = JS_ExecutePendingJob(js_env.rt, &ctx1);
        if (err <= 0) {
            if (err < 0)
                feature_dump_error(ctx1);
            break;
        }
    }
    feature_free_value(js_env.ctx, result);
    // release manager first
    FeatureUnsetUVLoop(g_manager);
    FeatureUninit(g_manager);

    for (cli_time_callback* tc : js_env.time_host.timers) {
        if (!tc->triggered) {
            JS_FreeValue(tc->ctx, tc->callback);
            uv_close((uv_handle_t*)tc->timer, NULL);
        }
        free(tc->timer);
        tc->timer = NULL;

        free(tc);
        tc = NULL;
    }
    js_env.time_host.timers.clear();
    uv_close((uv_handle_t*)&prepare, NULL);
    uv_close((uv_handle_t*)&timer, NULL);
#if defined(CONFIG_SYSTEM_ACTIVITY_SERVICE)
    if (binderFd >= 0) {
        uv_close((uv_handle_t*)&binder_poll, NULL);
    }
#endif

    // free g_manager
    FeatureFreeManager(g_manager);
    JS_FreeContext(js_env.ctx);
    JS_FreeRuntime(js_env.rt);
#ifdef CONFIG_FEATURE_RUST_MODULES
    close_vdk_async_runtime(vdk_runtime);
#endif

    int closed = 0;
    for (int j = 0; j < 200; j++) {
        if (uv_loop_close(main_loop) == 0) {
            closed = 1;
            break;
        }
        usleep(20000);
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
    // 释放manifast_str
    if (manifast_str != NULL) {
        free(manifast_str);
        manifast_str = NULL;
    }
    // 释放js_str
    if (js_str != NULL) {
        free(js_str);
        js_str = NULL;
    }
    return 0;
}
