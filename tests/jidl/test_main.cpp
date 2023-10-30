#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_registry.h"
#include "feat_test_utils.h"


using namespace ferry;
using namespace FEATURE;

// need remove global variable later
ferry::FeatureManagerQjs* g_manager = nullptr;

typedef struct feature_env_t {
  JSRuntime* rt;
  JSContext* ctx;
} feature_env_t;

// __require
feature_value_t __require(feature_context_ref ctx, feature_value_t this_val,
                          int argc, feature_value_t* argv) {
  if (argc < 1) {
    FEATURE_THROW_INTERNAL_ERROR(ctx, "require need module name!");
    return FEATURE_UNDEFINED;
  }

  const char* str_module_name = feature_to_cstring(ctx, argv[0]);
  feature_value_t vm_object = JS_UNDEFINED;
  auto feature_obj = g_manager->featureRequire(ctx, vm_object, str_module_name);
  feature_free_cstring(ctx, str_module_name);
  return feature_obj;
}

bool load_file(const char* file_name, char** file_content) {
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

// static void idle_cb(uv_idle_t* handle) {
//   static int num = 0;
//   feature_env_t* env = static_cast<feature_env_t*>(handle->data);
//   if (!JS_IsJobPending(env->rt)) {
//     num++;
//     if (num > 5) {
//       uv_stop(handle->loop);
//     }
//     return;
//   }
//   int cnt = 8, err;
//   feature_context_ref r_ctx;
//   while (cnt-- && !!JS_IsJobPending(env->rt)) {
//     err = JS_ExecutePendingJob(env->rt, &r_ctx);
//     if (err <= 0) {
//       if (err < 0) feature_dump_error(r_ctx);
//       break;
//     }
//   }

//   printf("idle callback\n");
//   if (num >= 5) {
//     printf("idle stop, num = %d\n", num);
//     uv_stop();
//   }
// }

typedef void (*uv_prepare_callback)(uv_prepare_t*);
// typedef void (*uv_check_callback)(uv_check_t*);
static void execute_job_cb(uv_prepare_t* handle) {
  feature_env_t* env = static_cast<feature_env_t*>(handle->data);
  feature_context_ref r_ctx;
  int ret;
  ret = JS_ExecutePendingJob(env->rt, &r_ctx);
  if (!ret) {
    uv_stop(handle->loop);
  } else if (ret > 0) {
    // 执行成功
  } else {
    // TODO: err deal
  }
}

// TODO： 增加超时退出机制
static void run_loop(ferry::FeatureManager* manager) {
  uv_loop_t* ploop = manager->getUVLoop();
  uv_run(ploop, UV_RUN_DEFAULT);
}

static void stop_loop(ferry::FeatureManager* manager) {
    uv_loop_t* ploop = manager->getUVLoop();
    uv_stop(ploop);
}

// 支持cli来读取 js 文件去执行，命令为：./feature_jidl_test
int main(int argc, char** argv) {
//   ferry::FeatureManagerQjs* manager;
  if (argc < 2) {
    printf("please input js file, like ./test.js \n");
    return 0;
  }

  char* js_file = argv[1];
  char* js_str = NULL;
  char* test_str = NULL;
  char* manifast_str = NULL;

  // this path should change depends
  const char* test_file =
      "/home/gyl/work/vela/miwear-bes/frameworks/base/feature/tests/jidl/test-internal.js";
  // 打开 test 框架加载文件
  load_file(test_file, &test_str);
  if (test_str == NULL) {
    printf("load test framework failed. filename: %s\n", test_file);
    return -1;
  }
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
//   uv_loop_t loop;
//   uv_loop_init(&loop);
  uv_prepare_t prepare;

  uv_prepare_init(uv_default_loop(), &prepare);
  prepare.data = &js_env;

  uv_prepare_start(&prepare, execute_job_cb);

  js_env.rt = JS_NewRuntime();
  js_env.ctx = JS_NewContext(js_env.rt);
  JS_SetRuntimeOpaque(js_env.rt, js_env.ctx);
  auto registry = new ferry::FeatureRegistry();
  registry->init(nullptr);
  // g_manager = CreateFeatureManager(registry, "js");
  g_manager = new ferry::FeatureManagerQjs(registry);

  struct LoopPack pack = {g_manager, run_loop, stop_loop};

  g_manager->setUserData("run_loop", &pack);

  g_manager->setUVLoop(uv_default_loop());

  // register global require
  feature_value_t global_obj = feature_global_object(js_env.ctx);
  feature_value_t require =
      feature_cfunction(js_env.ctx, __require, "require", 0);
  feature_set_object_property(js_env.ctx, global_obj, "require", require);
  feature_free_value(js_env.ctx, global_obj);

  // 加载 test frame work
  auto res = feature_eval(js_env.ctx, test_str, strlen(test_str), "<eval>",
                          JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(res)) {
    const char* str = JS_ToCString(js_env.ctx, res);
        printf("test internal file error: %s\n", str);
    feature_free_value(js_env.ctx, res);
    return -1;
  }
  feature_free_value(js_env.ctx, res);
  // 加载 测试文件
  auto result = feature_eval(js_env.ctx, js_str, strlen(js_str), "a.js",
                             JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_STRICT);

  if (JS_IsException(result)) {
    const char* str = JS_ToCString(js_env.ctx, result); 
        printf("exec js file error: %s\n", str);
    feature_free_value(js_env.ctx, result);
    return -1;
  }

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

//   uv_prepare_stop(&prepare);
//   uv_check_stop(&checker);
//   uv_close((uv_handle_t*)&prepare, NULL);
//   uv_close((uv_handle_t*)&checker, NULL);

  feature_free_value(js_env.ctx, result);
  // release manager first
  dynamic_cast<ferry::FeatureManagerQjs*>(g_manager)->uninit();
  JS_FreeContext(js_env.ctx);
  JS_FreeRuntime(js_env.rt);
  uv_loop_close(uv_default_loop());
  // 释放 test_str
  if (test_str) {
    free(test_str);
    test_str = NULL;
  }
  // 释放 js_str
  if (js_str != NULL) {
    free(js_str);
    js_str = NULL;
  }
  // free g_manager
  delete g_manager;

  return 0;
}
