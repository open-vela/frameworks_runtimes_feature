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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#ifdef CONFIG_SYSTEM_ACTIVITY_SERVICE
#include <binder/IPCThreadState.h>
#endif
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_main_exports.h"
#include "feature_qjs_exports.h"
#include "feature_utils.h"

// static FeatureManagerQjs* g_manager_qjs;
static FeatureManagerHandle g_manager;
extern FeatureRegistryTableHandle g_ajs_features_registry;

typedef struct feature_env_t {
    JSRuntime* rt;
    JSContext* ctx;
} feature_env_t;



static inline void feature_dump_obj(JSContext* ctx, JSValue val)
{
    const char* str = JS_ToCString(ctx, val);
    if (str) {
        FEATURE_LOG_ERROR("%s", str);
        JS_FreeCString(ctx, str);
    } else {
        FEATURE_LOG_ERROR("[exception]");
    }
}

/**
 * 打印错误信息
 */
static inline void feature_dump_error1(JSContext* ctx, JSValue exception_val)
{
    bool is_error = JS_IsError(ctx, exception_val);
    feature_dump_obj(ctx, exception_val);
    if (is_error) {
        // 如果是Error，则打印栈信息
        JSValue val = JS_GetPropertyStr(ctx, exception_val, "stack");
        if (!JS_IsUndefined(val)) {
            feature_dump_obj(ctx, val);
        }
        JS_FreeValue(ctx, val);
    }
}

static inline void feature_dump_error(JSContext* ctx)
{
    JSValue exception_val = JS_GetException(ctx);

    feature_dump_error1(ctx, exception_val);
    JS_FreeValue(ctx, exception_val);
}

// __require
JSValue __require(JSContext* ctx, JSValue this_val, int argc, JSValue* argv)
{
    if (argc < 1) {
        JS_ThrowInternalError(ctx, "require need module name!");
        return JS_UNDEFINED;
    }

    const char* module_name = JS_ToCString(ctx, argv[0]);
    ft_context_ref ft_ctx = FeatureGetContext(g_manager);
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
    FEATURE_LOG_INFO("%s\n", buff.c_str());
    return JS_UNDEFINED;
}

bool load_file(char* file_name, char** file_content)
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

#ifdef CONFIG_SYSTEM_ACTIVITY_SERVICE
void execute_jobs(JSContext* ctx)
{
    JSContext* ctx1;
    int err;

    // 执行挂起任务
    for (;;) {
        // 返回0表示任务全部完成
        err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
        if (err <= 0) {
            if (err < 0)
                feature_dump_error(ctx1);
            break;
        }
    }
}

static void __uv_check_cb(uv_check_t* handle)
{
    feature_env_t* env = static_cast<feature_env_t*>(handle->data);
    execute_jobs(env->ctx);
}

static void __uv_poll_cb(uv_poll_t* handle, int status, int events)
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

static void setScriptArgs(JSContext* ctx, JSValue global_obj, int argc, char* argv[], int scriptArgs_beg)
{
    JSValue arr = JS_NewArray(ctx);
    for (int i = 0, j = scriptArgs_beg; j < argc; i++, j++) {
        JSValue js_string = JS_NewString(ctx, argv[j]);
        JS_SetPropertyUint32(ctx, arr, i, js_string);
    }
    JS_SetPropertyStr(ctx, global_obj, "scriptArgs", arr);
}

// 支持cli来读取manitest.json以及js文件去执行，命令为:./feature_test_cli ./test.js ../manifest.json
// 当test.js使用message channel, 命令为:./feature_test_cli -m ./test.js ../manifest.json
extern "C" int main(int argc, char** argv)
{
    if (argc < 2) {
        FEATURE_LOG_ERROR("help: feature_test_cli js_file.js [manifest] [--scriptArgs ....]\n");
        return 0;
    }

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
    load_file(manifest_file, &manifast_str);

    // 打开manifest.json文件,读取内容到一个字符串中
    // 打开js文件
    if (!js_file) {
        FEATURE_LOG_ERROR("feature_test_cli: js_file.js is required");
        return 1;
    }

    load_file(js_file, &js_str);
    if (js_str == NULL) {
        FEATURE_LOG_ERROR("malloc js file failed!");
        if (manifast_str != NULL) {
            free(manifast_str);
            manifast_str = NULL;
        }
        return 0;
    }

    // initialize quickjs engine
    feature_env_t js_env;

    js_env.rt = JS_NewRuntime();
    js_env.ctx = JS_NewContext(js_env.rt);
    JS_SetRuntimeOpaque(js_env.rt, js_env.ctx);
    uv_loop_t uv_loop;
    uv_loop_init(&uv_loop);

    FeatureManagerCreateInfo ft_info;
    ft_info.raw_ctx = (FeatureRawContextHandle)(js_env.ctx);
    ft_info.release_cb = nullptr;
    ft_info.manager_type = FEATURE_MANAGER_JS;
    ft_info.package_name = "feature_test_cli";
    g_manager = FeatureCreateManager(&ft_info);
    FeatureRegistryHandle hRegistry = FeatureGetRegistryFromManager(g_manager);
    // reigstry features
    FeatureRegisterFeatures(hRegistry, g_ajs_features_registry);
    FeatureSetArgsErrorCb(g_manager, on_feature_args_error, &js_env);
    // FeatureSetManagerUserData(g_manager, "app", app);
    FEATURE_CHECK_NE(g_manager, nullptr);
    FEATURE_LOG_INFO("created FeatureManagerHandle: %p", g_manager);
    FeatureSetUVLoop(g_manager, &uv_loop);

    // register global require
    JSValue global_obj = JS_GetGlobalObject(js_env.ctx);
    JSValue console = JS_NewObject(js_env.ctx);
    JS_SetPropertyStr(js_env.ctx, global_obj, "console", console);
    setScriptArgs(js_env.ctx, global_obj, argc, argv, scriptArgs_beg);

    JSValue require = JS_NewCFunction(js_env.ctx, __require, "require", 0);
    JS_SetPropertyStr(js_env.ctx, global_obj, "require", require);
    JSValue log = JS_NewCFunction(js_env.ctx, __log, "console_log", 0);
    JS_SetPropertyStr(js_env.ctx, console, "log", log);
    JS_FreeValue(js_env.ctx, global_obj);

    auto result = JS_Eval(js_env.ctx, js_str, strlen(js_str), "<eval>", JS_EVAL_TYPE_GLOBAL);

    if (!use_uvloop_async) {
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
    } else {
#ifdef CONFIG_SYSTEM_ACTIVITY_SERVICE
        int binderFd;
        android::IPCThreadState::self()->setupPolling(&binderFd);
        if (binderFd < 0) {
            FEATURE_LOG_ERROR("failed to open binder device:%d", errno);
            return -1;
        }

        // init uv_check_t & uv_poll_t
        uv_check_t uv_check;
        uv_poll_t uv_poll;
        uv_check_init(&uv_loop, &uv_check);
        uv_poll_init(&uv_loop, &uv_poll, binderFd);
        uv_check.data = &js_env;
        uv_check_start(&uv_check, __uv_check_cb);
        uv_poll_start(&uv_poll, UV_READABLE, __uv_poll_cb);
        uv_unref((uv_handle_t*)&uv_check);
        uv_run(&uv_loop, UV_RUN_DEFAULT);
#endif
    }

    JS_FreeValue(js_env.ctx, result);
    // release manager first
    FeatureUninit(g_manager);
    JS_FreeContext(js_env.ctx);
    JS_FreeRuntime(js_env.rt);

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
    FeatureFreeManager(g_manager);

    return 0;
}
