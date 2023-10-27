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

#include <binder/IPCThreadState.h>

#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_registry.h"

using namespace ferry;
using namespace FEATURE;

static ferry::FeatureManagerQjs* g_manager_qjs;

typedef struct feature_env_t {
    JSRuntime* rt;
    JSContext* ctx;
} feature_env_t;

// __require
feature_value_t __require(feature_context_ref ctx, feature_value_t this_val, int argc, feature_value_t* argv)
{
    if (argc < 1) {
        FEATURE_THROW_INTERNAL_ERROR(ctx, "require need module name!");
        return FEATURE_UNDEFINED;
    }

    const char* str_module_name = feature_to_cstring(ctx, argv[0]);
    feature_value_t vm_object = JS_UNDEFINED;
    auto feature_obj = g_manager_qjs->featureRequire(ctx, vm_object, str_module_name);
    feature_free_cstring(ctx, str_module_name);
    return feature_obj;
}

bool load_file(char* file_name, char** file_content)
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

// 支持cli来读取manitest.json以及js文件去执行，命令为:./feature_test_cli ./test.js ../manifest.json
// 当test.js使用message channel, 命令为:./feature_test_cli -m ./test.js ../manifest.json
extern "C" int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("please input manifest.json file and js file, like ./feature_test_cli ./test.js ./manifest.json or ./feature_test_cli -m ./test.js ./manifest.json when use message channel!\n");
        return 0;
    }

    char* js_file = NULL;
    char* js_str = NULL;
    char* manifast_str = NULL;
    char* manifest_file = NULL;
    bool use_uvloop_async = false;

    if (argc == 3) {
        js_file = argv[1];
        manifest_file = argv[2];
        load_file(manifest_file, &manifast_str);

        if (manifast_str == NULL) {
            printf("malloc manifest.json failed!\n");
            return 0;
        }
    } else if (argc == 4 && strcmp(argv[1], "-m") == 0) {
        use_uvloop_async = true;
        js_file = argv[2];
        manifest_file = argv[3];
        load_file(manifest_file, &manifast_str);
        if (manifast_str == NULL) {
            printf("malloc manifest.json failed!\n");
            return 0;
        }
    }

    // 打开manifest.json文件,读取内容到一个字符串中
    // 打开js文件
    load_file(js_file, &js_str);
    if (js_str == NULL) {
        printf("malloc js file failed!\n");
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
    auto registry = new ferry::FeatureRegistry();
    registry->init(manifast_str);
    g_manager_qjs = new ferry::FeatureManagerQjs(registry);

    // register global require
    feature_value_t global_obj = feature_global_object(js_env.ctx);
    feature_value_t require = feature_cfunction(js_env.ctx, __require, "require", 0);
    feature_set_object_property(js_env.ctx, global_obj, "require", require);
    feature_free_value(js_env.ctx, global_obj);

    auto result = feature_eval(js_env.ctx, js_str, strlen(js_str), "<eval>", JS_EVAL_TYPE_GLOBAL);

    if (!use_uvloop_async) {
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
    } else {
        int binderFd;
        android::IPCThreadState::self()->setupPolling(&binderFd);
        if (binderFd < 0) {
            printf("failed to open binder device:%d", errno);
            return -1;
        }

        // init uv_check_t & uv_poll_t
        uv_loop_t loop_t;
        uv_check_t check_t;
        uv_poll_t poll_t;
        uv_loop_init(&loop_t);
        uv_check_init(&loop_t, &check_t);
        uv_poll_init(&loop_t, &poll_t, binderFd);
        check_t.data = &js_env;
        uv_check_start(&check_t, __uv_check_cb);
        uv_poll_start(&poll_t, UV_READABLE, __uv_poll_cb);
        uv_unref((uv_handle_t*)&check_t);
        uv_run(&loop_t, UV_RUN_DEFAULT);
    }

    feature_free_value(js_env.ctx, result);
    // release manager first
    g_manager_qjs->uninit();
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
    delete g_manager_qjs;

    return 0;
}
