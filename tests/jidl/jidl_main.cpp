#include "feature_log.h"
#include "feature_manager.h"
#include "feature_registry.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace ferry;
using namespace FEATURE;

static ferry::FeatureRegistry* g_registry;
static ferry::FeatureManager* g_manager;

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
    auto feature_obj = g_manager->featureRequire(ctx, str_module_name);
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
    //获取文件长度
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *file_content = (char*)malloc(len + 1);
    memset(*file_content, 0, len + 1);
    //读取文件内容到file_content字符串中
    fread(*file_content, len, 1, fp);
    fclose(fp);

    return true;
}

//支持cli来读取manitest.json以及js文件去执行，命令为：./feature_jidl_test ./test.js ../manifest.json
int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("please input manifest.json file and js file, like ./jidl_main ./test.js!\n");
        return 0;
    }

    char* js_file = argv[1];
    char* js_str = NULL;
    char* manifast_str = NULL;

    if (argc == 3) {
        char* manifest_file = argv[2];
        load_file(manifest_file, &manifast_str);

        if (manifast_str == NULL) {
            printf("malloc manifest.json failed!\n");
            return 0;
        }
    }

    //打开manifest.json文件,读取内容到一个字符串中
    //打开js文件
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
    g_registry = new ferry::FeatureRegistry(nullptr);
    g_registry->init(manifast_str);
    g_manager = new ferry::FeatureManager(g_registry);

    // register global require
    feature_value_t global_obj = feature_global_object(js_env.ctx);
    feature_value_t require = feature_cfunction(js_env.ctx, __require, "require", 0);
    feature_set_object_property(js_env.ctx, global_obj, "require", require);
    feature_free_value(js_env.ctx, global_obj);

    auto result = feature_eval(js_env.ctx, js_str, strlen(js_str), "<eval>", JS_EVAL_TYPE_GLOBAL);
    
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
    JS_FreeContext(js_env.ctx);
    JS_FreeRuntime(js_env.rt);
    g_manager->featureRelease();
    g_registry->uninit();

    //释放manifast_str
    if (manifast_str != NULL) {
        free(manifast_str);
        manifast_str = NULL;
    }
    //释放js_str
    if (js_str != NULL) {
        free(js_str);
        js_str = NULL;
    }

    return 0;
}
