#include "builtin/builtin_console.h"
#include "builtin/console.h"
#include "feature_context.h"
#include "feature_context_qjs.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_registry.h"
#include "quickjs/quickjs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace feature_framework;

static FeatureManagerQjs* g_manager_qjs;

typedef bool (*FeatureRegistryFunc)(FeatureRegistryHandle);
extern FeatureRegistryFunc g_ajs_features_registry[];
extern size_t g_ajs_features_registry_count;

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
    FeatureManagerHandle new_manager = g_manager_qjs;
    ft_value_t feature_proto = FeatureFindFeature(new_manager, str_module_name);
    if (!JS_IsUndefined(FT_VAL_GET_JS_VAL(feature_proto))) {
        FEATURE_LOG_INFO("Find feautre in new manager: %s", str_module_name);
        ft_value_t param;
        *FT_VAL_GET_JS_VAL_PTR(param) = JS_UNDEFINED;
        auto res = FeatureCreateFeature(new_manager, feature_proto, param /* vm */);

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

// 支持cli来读取包名以及js文件去执行，命令为：./feature_jidl_test ./test.js pkg_name
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
    // JS_SetRuntimeOpaque(js_env.rt, js_env.ctx);
    auto registry = new FeatureRegistry();
    registry->init(pkg_name);

    g_manager_qjs = new FeatureManagerQjs(registry, js_env.ctx);
    for (size_t i = 0; i < g_ajs_features_registry_count; i++) {
        g_ajs_features_registry[i](registry);
    }

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
    g_manager_qjs->uninit();
    JS_FreeContext(js_env.ctx);
    JS_FreeRuntime(js_env.rt);
    // free g_manager_qjs
    delete g_manager_qjs;

    free(file_str);
    return 0;
}
