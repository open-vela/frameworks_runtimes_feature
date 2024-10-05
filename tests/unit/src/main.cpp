#include "backend/qjs/feature_context_qjs.h"
#include "builtin/builtin_console.h"
#include "builtin/console.h"
#include "feature.h"
#include "feature_context.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_main_exports.h"
#include "feature_types.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FeatureManagerHandle g_manager_qjs;

extern FeatureRegistryTableHandle g_ajs_features_registry;

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
    auto feature_obj = JS_UNDEFINED;
    ft_value_t feature_proto = FeatureFindFeature(g_manager_qjs, str_module_name);
    if (!JS_IsUndefined(FT_VAL_GET_JS_VAL(feature_proto))) {
        FEATURE_LOG_INFO("Find feautre in new manager: %s", str_module_name);
        ft_value_t param;
        *FT_VAL_GET_JS_VAL_PTR(param) = JS_UNDEFINED;
        auto res = FeatureCreateFeature(g_manager_qjs, feature_proto, param /* vm */);

        JS_FreeValue(ctx, FT_VAL_GET_JS_VAL(feature_proto));
        if (JS_IsUndefined(FT_VAL_GET_JS_VAL(res))) {
            FEATURE_LOG_ERROR("Failed to find feature:%s", str_module_name);
        }
        feature_obj = FT_VAL_GET_JS_VAL(res);
    }

    feature_free_cstring(ctx, str_module_name);
    return feature_obj;
}

int load_file(char* file_name, char** file_content)
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

    return len;
}

static bool on_feature_args_error(void* data, ArgsErrorInfo* error_info)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s: runtime context is null!", __func__);
        return false;
    }
    feature_env_t* js_env = static_cast<feature_env_t*>(data);
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
            JSValue fail_cb = JS_GetPropertyStr(js_env->ctx, arg, "fail");
            if (JS_IsUndefined(fail_cb))
                continue;

            FEATURE_LOG_INFO("%s: found fail callback from arg %d!", __func__, i);
            JSValue argv[2];
            argv[0] = JS_NewString(js_env->ctx, error_info->error_msg);
            argv[1] = JS_NewInt32(js_env->ctx, error_info->error_code);
            JSValue ret = JS_Call(js_env->ctx, fail_cb, JS_UNDEFINED, 2, argv);
            JS_FreeValue(js_env->ctx, ret);
            JS_FreeValue(js_env->ctx, fail_cb);
            JS_FreeValue(js_env->ctx, argv[0]);

            JSValue complete_cb = JS_GetPropertyStr(js_env->ctx, arg, "complete");
            if (JS_IsUndefined(complete_cb)) {
                FEATURE_LOG_WARN("%s: no complete callback from arg %d!", __func__, i);
                return true;
            }
            ret = JS_Call(js_env->ctx, complete_cb, JS_UNDEFINED, 0, NULL);
            JS_FreeValue(js_env->ctx, ret);
            JS_FreeValue(js_env->ctx, complete_cb);
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("please input js file and package_name, like ./feature_jidl_test ./test.js, or ./feature_jidl_test ./test.js package_name!\n");
        return 0;
    }

    char* file_name = argv[1];
    char* file_str = NULL;
    char* pkg_name = NULL;
    if (argc == 3) {
        pkg_name = argv[2];
    }

    // 打开manifest.json文件,读取内容到一个字符串中
    // 打开js文件
    int file_len = load_file(file_name, &file_str);
    if (file_str == NULL) {
        printf("load file failed!\n");
        return 0;
    }

    uint32_t len = strlen(file_name);
    // initialize quickjs engine
    feature_env_t js_env;

    js_env.rt = JS_NewRuntime();
    js_env.ctx = JS_NewContext(js_env.rt);

    FeatureManagerCreateInfo ft_info;
    ft_info.raw_ctx = (FeatureRawContextHandle)(js_env.ctx);
    ft_info.release_cb = nullptr;
    ft_info.manager_type = FEATURE_MANAGER_JS;
    ft_info.package_name = "com.feature.test";
    g_manager_qjs = FeatureCreateManager(&ft_info);
    FeatureRegistryHandle hRegistry = FeatureGetRegistryFromManager(g_manager_qjs);
    // reigstry features
    FeatureRegisterFeatures(hRegistry, g_ajs_features_registry);
    FeatureSetArgsErrorCb(g_manager_qjs, on_feature_args_error, &js_env);

    // register global require
    feature_value_t global_obj = feature_global_object(js_env.ctx);
    feature_value_t require = feature_cfunction(js_env.ctx, __require, "require", 0);
    feature_set_object_property(js_env.ctx, global_obj, "require", require);
    feature_free_value(js_env.ctx, global_obj);

    // add console
    builtin::addConsoleModule(js_env.ctx, "console.js", builtin::CONSOLE_JS);
    auto result = feature_eval(js_env.ctx, file_str, strlen(file_str), "<eval>", JS_EVAL_TYPE_GLOBAL);

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
    FeatureUninit(g_manager_qjs);
    FeatureFreeManager(g_manager_qjs);
    // free js runtime
    JS_FreeContext(js_env.ctx);
    JS_FreeRuntime(js_env.rt);

    free(file_str);
    return 0;
}
