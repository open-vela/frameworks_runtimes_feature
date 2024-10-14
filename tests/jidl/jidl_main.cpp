#include "builtin/builtin_console.h"
#include "builtin/console.h"
#include "feature_log.h"
#include "feature_manager_qjs.h"
#ifdef CONFIG_FEATURE_USE_WAMR
#include "feature_manager_wamr.h"
#endif
#include "feature_registry.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace feature_framework;
using namespace FEATURE;

static FeatureManagerQjs* g_manager_qjs;
#ifdef CONFIG_FEATURE_USE_WAMR
static FeatureManagerWamr* g_manager_wamr;
#endif

typedef struct feature_env_t {
    JSRuntime* rt;
    JSContext* ctx;
} feature_env_t;

#ifdef CONFIG_FEATURE_USE_WAMR
int events_poll(wasm_exec_env_t exec_env)
{
    /* TODO: not detect macro tasks yet */
    return -1;
}

void execute_micro_tasks(wasm_exec_env_t exec_env, dyn_ctx_t ctx)
{
    int err;

    for (;;) {
        /* execute the pending jobs */
        for (;;) {
            err = dyntype_execute_pending_jobs(ctx);
            if (err <= 0) {
                if (err < 0) {
                    dyntype_dump_error(ctx);
                }
                break;
            }
        }

        if (events_poll(exec_env))
            break;
    }
}
#endif

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
    if (file_name[len - 1] == 's') { // file is js file
        // initialize quickjs engine
        feature_env_t js_env;

        js_env.rt = JS_NewRuntime();
        js_env.ctx = JS_NewContext(js_env.rt);
        // JS_SetRuntimeOpaque(js_env.rt, js_env.ctx);
        auto registry = new FeatureRegistry();
        registry->init(pkg_name);

        g_manager_qjs = new FeatureManagerQjs(registry);

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
    } else { /* file is wasm file */
#ifdef CONFIG_FEATURE_USE_WAMR
        wasm_module_t module = NULL;
        wasm_module_inst_t module_inst = NULL;
        wasm_exec_env_t exec_env = NULL;
        uint stack_size = 64 * 1024, heap_size = 16 * 1024;
        char error_buf[128] = { 0 };
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_Allocator;
        init_args.mem_alloc_option.allocator.malloc_func = (void*)malloc;
        init_args.mem_alloc_option.allocator.realloc_func = (void*)realloc;
        init_args.mem_alloc_option.allocator.free_func = (void*)free;
        init_args.gc_heap_size = 16 * 1024;

        if (!wasm_runtime_full_init(&init_args)) {
            FEATURE_LOG_ERROR("Init runtime environment failed.");
            return -1;
        }

        /* initialize dyntype context and set callback dispatcher */
        dyn_ctx_t dyn_ctx = dyntype_context_init();
        dyntype_set_callback_dispatcher(dyntype_callback_wasm_dispatcher);

        /* init feature about wasm */
        auto registry = new FeatureRegistry();
        registry->init(pkg_name);
        g_manager_wamr = new FeatureManagerWamr(registry);
        if (!g_manager_wamr->init()) {
            printf(" wamr init error!\n");
            return 0;
        }

        module = wasm_runtime_load((uint8_t*)file_str, file_len, error_buf, sizeof(error_buf));
        if (!module) {
            printf("%s\n", error_buf);
            return 0;
        }
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
        if (!module_inst) {
            printf("%s\n", error_buf);
            return 0;
        }

        exec_env = wasm_runtime_get_exec_env_singleton(module_inst);
        if (exec_env == NULL) {
            printf("%s\n", wasm_runtime_get_exception(module_inst));
        }

        wasm_application_execute_main(module_inst, 0, NULL);
        const char* exception = wasm_runtime_get_exception(module_inst);
        if (exception) {
            printf("%s\n", exception);
        }

        /* run micro tasks */
        execute_micro_tasks(exec_env, dyn_ctx);

        wasm_runtime_deinstantiate(module_inst);
        wasm_runtime_unload(module);

        g_manager_wamr->release();

        /* destroy dynamic ctx */
        dyntype_context_destroy(dyn_ctx);

        /* destroy runtime environment */
        wasm_runtime_destroy();
        delete g_manager_wamr;
#endif
    }

    free(file_str);
    return 0;
}
