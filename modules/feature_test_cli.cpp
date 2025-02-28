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
#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_permission.h"
#include "feature_qjs_exports.h"
#include "feature_registry.h"

#define CLI_TIME_LIMIT 2 // 异步限时 2000ms
using namespace feature_framework;
using namespace FEATURE;

static FeatureManagerQjs* g_manager_qjs;

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
feature_value_t __require(feature_context_ref ctx, feature_value_t this_val, int argc, feature_value_t* argv)
{
    if (argc < 1) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "require need module name!");
        return FEATURE_UNDEFINED;
    }

    const char* module_name = feature_to_cstring(ctx, argv[0]);
    ft_context_ref ft_ctx = g_manager_qjs->getFeatureContext();
    ft_value_t ft_vm_obj = ft_from_jsvalue(ft_ctx, JS_UNDEFINED);
    ft_value_t ft_obj = g_manager_qjs->featureRequire(ft_vm_obj, module_name);
    auto js_obj = ft_to_jsvalue(ft_ctx, ft_obj);
    feature_free_cstring(ctx, module_name);
    return js_obj;
}

// console_log
feature_value_t __log(feature_context_ref ctx, feature_value_t this_val, int argc, feature_value_t* argv)
{
    int i;
    const char* str;
    std::string buff;
    for (i = 0; i < argc; i++) {
        if (i != 0)
            buff += ' ';
        str = feature_to_cstring(ctx, argv[i]);
        if (str) {
            buff += str;
        } else { // exception
            buff += "[custom object]";
        }

        feature_free_cstring(ctx, str);
    }
    FEATURE_LOG_INFO("%s", buff.c_str());
    return FEATURE_UNDEFINED;
}

feature_value_t __setCliTimeout(feature_context_ref ctx, feature_value_t this_val, int argc, feature_value_t* argv,
    int magic, feature_value_t* func_data)
{
    if (argc < 2) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "setTimeout need a callback and time!");
        return FEATURE_UNDEFINED;
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
    return FEATURE_UNDEFINED;
}

// exit
feature_value_t __cliExit(feature_context_ref ctx, feature_value_t this_val, int argc, feature_value_t* argv)
{
    uv_loop_t* ploop = g_manager_qjs->getUVLoop();
    uv_stop(ploop);
    FEATURE_LOG_INFO("feature_cli_test exit!");
    return FEATURE_UNDEFINED;
}

bool cli_load_file(char* file_name, char** file_content)
{
    if (file_name == NULL || file_content == NULL) {
        printf("file_name or file_content is NULL!\n");
        return false;
    }

    FILE* fp = fopen(file_name, "r");
    if (fp == NULL) {
        printf("open file_name is %s failed!\n", file_name);
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
    uv_loop_t* ploop = g_manager_qjs->getUVLoop();
    uv_stop(ploop);
}

#ifdef CONFIG_SYSTEM_ACTIVITY_SERVICE
static void __cli_uv_poll_cb(uv_poll_t* handle, int status, int events)
{
    android::IPCThreadState::self()->handlePolledCommands();
}
#endif

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
        FeatureGrantPermissions(g_manager_qjs, handle);
    } else {
        FeatureRejectPermissions(g_manager_qjs, handle, reason);
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
    cli_load_file(manifest_file, &manifast_str);

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
    auto registry = new FeatureRegistry();
    registry->init(manifast_str);
    g_manager_qjs = new FeatureManagerQjs(registry, (feature_context_ref)(js_env.ctx));

    // 初始化
    uv_loop_t* main_loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    uv_loop_init(main_loop);

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
    android::IPCThreadState::self()->setupPolling(&binderFd);
    if (binderFd < 0) {
        printf("failed to open binder device:%d", errno);
    } else {
        uv_poll_t binder_poll;
        uv_poll_init(main_loop, &binder_poll, binderFd);
        uv_poll_start(&binder_poll, UV_READABLE, __cli_uv_poll_cb);
    }
#endif

    FeatureSetUVLoop(g_manager_qjs, main_loop);
    FeatureSetPermissionsCallback(g_manager_qjs, permissions_cb, NULL);

    feature_value_t global_obj = feature_global_object(js_env.ctx);
    feature_value_t console = feature_object(js_env.ctx);
    feature_set_object_property(js_env.ctx, global_obj, "console", console);

    feature_value_t require = feature_cfunction(js_env.ctx, __require, "require", 0);
    feature_set_object_property(js_env.ctx, global_obj, "require", require);

    feature_value_t log = feature_cfunction(js_env.ctx, __log, "console_log", 0);
    feature_set_object_property(js_env.ctx, console, "log", log);

    feature_value_t exit = feature_cfunction(js_env.ctx, __cliExit, "exit", 0);
    feature_set_object_property(js_env.ctx, global_obj, "exit", exit);

    feature_value_t time_func_data = feature_object(js_env.ctx);
    feature_set_opaque(time_func_data, &(js_env.time_host));
    feature_value_t setTimeout = feature_cfunctiondata(js_env.ctx, __setCliTimeout, 1, 0, 1, &time_func_data);
    feature_free_value(js_env.ctx, time_func_data);
    feature_set_object_property(js_env.ctx, global_obj, "setTimeout", setTimeout);

    setScriptArgs(js_env.ctx, global_obj, argc, argv, scriptArgs_beg);

    feature_free_value(js_env.ctx, global_obj);
    auto result = feature_eval(js_env.ctx, js_str, strlen(js_str), "<eval>", JS_EVAL_TYPE_GLOBAL);
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
    feature_context_ref ctx1;
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
    FeatureUnsetUVLoop(g_manager_qjs);
    g_manager_qjs->uninit();

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
    if (uv_loop_alive(main_loop)) {
        uv_stop(main_loop);
        uv_loop_close(main_loop);
        free(main_loop);
        main_loop = NULL;
    }
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
    // free g_manager_qjs
    delete g_manager_qjs;
    JS_FreeContext(js_env.ctx);
    JS_FreeRuntime(js_env.rt);
    return 0;
}
