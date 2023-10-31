#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_registry.h"

using namespace ferry;
using namespace FEATURE;

typedef struct feature_env_t {
  JSRuntime* rt;
  JSContext* ctx;
} feature_env_t;

typedef void (*LoopFunc)(void*);
typedef struct LoopPack {
  void* manager;
  LoopFunc run_loop;
  LoopFunc stop_loop;
} LoopPack;

// __require
feature_value_t __require(feature_context_ref ctx, feature_value_t this_val,
                          int argc, feature_value_t* argv, int magic,
                          feature_value_t* func_data) {
  if (argc < 1) {
    FEATURE_THROW_INTERNAL_ERROR(ctx, "require need module name!");
    return FEATURE_UNDEFINED;
  }

  ferry::FeatureManagerQjs* manager = static_cast<ferry::FeatureManagerQjs*>(
      feature_get_opaque(func_data[0], 1));

  const char* str_module_name = feature_to_cstring(ctx, argv[0]);
  feature_value_t vm_object = JS_UNDEFINED;
  auto feature_obj = manager->featureRequire(ctx, vm_object, str_module_name);
  feature_free_cstring(ctx, str_module_name);
  return feature_obj;
}

typedef struct {
  JSContext *ctx;
  JSValue callback;
} TimeCallback;
void timeout_callback(uv_timer_t *handle) {
  static int cnt = 0;
  printf("callback %d\n", cnt++);

  TimeCallback *tc = static_cast<TimeCallback *>(handle->data);
  JS_Call(tc->ctx, tc->callback, JS_UNDEFINED, 0, NULL);
  JS_FreeValue(tc->ctx, tc->callback);
}

// setTimeout
feature_value_t __setTimeout(JSContext* ctx, feature_value_t this_val,
                             int argc, JSValue* argv) {
  if (argc < 2) {
    FEATURE_THROW_INTERNAL_ERROR(ctx, "setTimeout need a callback and time!");
    return FEATURE_UNDEFINED;
  }
  int t = JS_VALUE_GET_INT(argv[1]);
  TimeCallback* tc = (TimeCallback*)malloc(sizeof(TimeCallback));
  tc->ctx = ctx;
  tc->callback = JS_DupValue(ctx, argv[0]);
  printf("set time out!!!! %d\n", t);
  uv_timer_t *timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
  uv_timer_init(uv_default_loop(), timer);
  timer->data = tc;
  uv_timer_start(timer, timeout_callback, t, 0);
  return feature_int(ctx, 3);
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

// TODO: remove stop
static void execute_job_cb(uv_prepare_t* handle) {
  feature_env_t* env = static_cast<feature_env_t*>(handle->data);
  feature_context_ref r_ctx;
  int ret;
  ret = JS_ExecutePendingJob(env->rt, &r_ctx);
}

// TODO： 增加超时退出机制
static void run_loop(void* m) {
  ferry::FeatureManager* manager = static_cast<ferry::FeatureManager*>(m);
  uv_loop_t* ploop = manager->getUVLoop();
  uv_run(ploop, UV_RUN_DEFAULT);
}

static void stop_loop(void* m) {
  ferry::FeatureManager* manager = static_cast<ferry::FeatureManager*>(m);
  uv_loop_t* ploop = manager->getUVLoop();
  uv_stop(ploop);
}

static feature_value_t getRequireObject(feature_env_t* js_env,
                                        ferry::FeatureManagerQjs* m) {
  feature_value_t func_data = feature_object(js_env->ctx);
  feature_set_opaque(func_data, m);
  feature_value_t ret =
      feature_cfunctiondata(js_env->ctx, __require, 1, 0, 1, &func_data);
  feature_free_value(js_env->ctx, func_data);
  return ret;
}

// 支持cli来读取 js 文件去执行，命令为：./feature_jidl_test
int main(int argc, char** argv) {
  if (argc < 2) {
    printf("please input js file, like ./test.js \n");
    return 0;
  }

  char* js_file = argv[1];
  char* js_str = NULL;

  // 打开js文件
  load_file(js_file, &js_str);
  if (js_str == NULL) {
    printf("malloc js file failed!\n");
    return 0;
  }

  // initialize quickjs engine
  feature_env_t js_env;
  uv_prepare_t prepare;

  uv_prepare_init(uv_default_loop(), &prepare);
  prepare.data = &js_env;

  uv_prepare_start(&prepare, execute_job_cb);

  js_env.rt = JS_NewRuntime();
  js_env.ctx = JS_NewContext(js_env.rt);
  JS_SetRuntimeOpaque(js_env.rt, js_env.ctx);
  auto registry = new ferry::FeatureRegistry();
  registry->init(nullptr);

  // TODO: use factory pattern: manager = CreateFeatureManager(registry, "js");
  ferry::FeatureManagerQjs* manager = new ferry::FeatureManagerQjs(registry);

  LoopPack pack = {manager, run_loop, stop_loop};

  manager->setUserData("run_loop", &pack);

  manager->setUVLoop(uv_default_loop());

  // register global require
  feature_value_t global_obj = feature_global_object(js_env.ctx);
  feature_value_t require = getRequireObject(&js_env, manager);
  feature_set_object_property(js_env.ctx, global_obj, "require", require);

  feature_value_t setTimeout =
      feature_cfunction(js_env.ctx, __setTimeout, "setTimeout", 2);
  feature_set_object_property(js_env.ctx, global_obj, "setTimeout", setTimeout);

  feature_free_value(js_env.ctx, global_obj);

  // 加载 test frame work
  // TODO: ues qjs bytecode
  // original file ../test-internal.js
  const char* test_content =
      "let unittest = require('feat_test');\n\nfunction feat_test(name, desc, "
      "cb) {\n    unittest.testsuite(name, desc, cb);\n}\n\nfunction "
      "feat_async_test(suitname, desc, test_cb) {\n    var async_id;\n    "
      "async_id = unittest.testsuite(suitname, desc, () => {\n        "
      "test_cb(() => unittest.done(async_id));\n    }, true);\n}\n\nfunction "
      "feat_test_all() {\n    unittest.run_all_tests();\n}\n\nfunction "
      "feat_expect_true(r, d) {\n    return unittest.expect_true(r, "
      "d);\n}\n\nfunction print(a) {\n    unittest.print(a);\n}\n";
  auto res = feature_eval(js_env.ctx, test_content, strlen(test_content),
                          "<eval>", JS_EVAL_TYPE_GLOBAL);
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

  feature_free_value(js_env.ctx, result);
  // release manager first
  manager->uninit();
  JS_FreeContext(js_env.ctx);
  JS_FreeRuntime(js_env.rt);
  uv_loop_close(uv_default_loop());

  // free js_str
  if (js_str != NULL) {
    free(js_str);
    js_str = NULL;
  }
  // free manager
  delete manager;

  return 0;
}
