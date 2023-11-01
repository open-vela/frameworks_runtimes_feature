#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <set>

#include "feature_log.h"
#include "feature_manager_qjs.h"
#include "feature_registry.h"

using namespace ferry;
using namespace FEATURE;

bool load_file(const char *file_name, char **file_content);

#define TIME_LIMIT 2000  // 异步限时 2000ms

struct TimeoutHost;
struct TimeCallback {
  TimeoutHost *host;
  JSContext *ctx;
  JSValue callback;
};

struct TimeoutHost {
  std::set<TimeCallback *> timers;
};

typedef int (*LoopFunc)(void *);
typedef struct FeatTestEnv {
  LoopFunc run_loop;
  LoopFunc stop_loop;
  void *manager;
  TimeoutHost time_host;
  uv_timer_t *async_limiter;
  JSRuntime *rt;
  JSContext *ctx;
} FeatTestEnv;

// __require
JSValue __require(JSContext *ctx, JSValue this_val, int argc, JSValue *argv,
                  int magic, JSValue *func_data) {
  if (argc < 1) {
    FEATURE_THROW_INTERNAL_ERROR(ctx, "require need module name!");
    return FEATURE_UNDEFINED;
  }

  ferry::FeatureManagerQjs *manager =
      static_cast<ferry::FeatureManagerQjs *>(JS_GetOpaque(func_data[0], 1));

  const char *str_module_name = JS_ToCString(ctx, argv[0]);
  JSValue vm_object = JS_UNDEFINED;
  auto feature_obj = manager->featureRequire(ctx, vm_object, str_module_name);
  JS_FreeCString(ctx, str_module_name);
  return feature_obj;
}

/**
 * setTimeout 的回调函数
 */
void timeout_callback(uv_timer_t *handle) {
  TimeCallback *tc = static_cast<TimeCallback *>(handle->data);
  JS_Call(tc->ctx, tc->callback, JS_UNDEFINED, 0, NULL);
  JS_FreeValue(tc->ctx, tc->callback);
  tc->host->timers.erase(tc);
  tc->host = NULL;
  free(tc);
  handle->data = 0;
  uv_timer_stop(handle);
  uv_close((uv_handle_t *)handle, NULL);
  free(handle);
}

// setTimeout
JSValue __setTimeout(JSContext *ctx, JSValue this_val, int argc, JSValue *argv,
                     int magic, JSValue *func_data) {
  if (argc < 2) {
    FEATURE_THROW_INTERNAL_ERROR(ctx, "setTimeout need a callback and time!");
    return FEATURE_UNDEFINED;
  }
  TimeoutHost *time_host = (TimeoutHost *)JS_GetOpaque(func_data[0], 1);
  int t = JS_VALUE_GET_INT(argv[1]);
  TimeCallback *tc = (TimeCallback *)malloc(sizeof(TimeCallback));
  tc->callback = JS_DupValue(ctx, argv[0]);
  tc->host = time_host;
  tc->ctx = ctx;
  time_host->timers.insert(tc);
  uv_timer_t *timer = (uv_timer_t *)malloc(sizeof(uv_timer_t));
  uv_timer_init(uv_default_loop(), timer);
  timer->data = tc;
  uv_timer_start(timer, timeout_callback, t, 0);
  return JS_UNDEFINED;
}

/**
 * 在进入 io 之前执行（时机待定）：
 *  1. 如果有 pending job 就执行
 *  2. 如果没有 pending job 查看是否有运行时数据(io/timer等)未执行完。
 *    2.1 检查 timer
 *    2.3 todo: 检查 IO
 */
static void execute_job_cb(uv_prepare_t *handle) {
  FeatTestEnv *env = static_cast<FeatTestEnv *>(handle->data);
  JSContext *r_ctx;
  int err;
  for (;;) {
    err = JS_ExecutePendingJob(env->rt, &r_ctx);
    if (err <= 0) {
      break;
    }
  }
}

/**
 * 异步测试定时的超时回调
 */
static void async_limit_cb(uv_timer_t *handle) {
  FeatTestEnv *env = static_cast<FeatTestEnv *>(handle->data);
  uv_loop_t *ploop =
      static_cast<ferry::FeatureManager *>(env->manager)->getUVLoop();
  uv_stop(ploop);
}

/**
 * 如果超时退出，返回 1
 * 如果调用 done() 退出，返回 0
 */
static int run_loop(void *feat_test_env) {
  FeatTestEnv *env = static_cast<FeatTestEnv *>(feat_test_env);
  ferry::FeatureManager *manager =
      static_cast<ferry::FeatureManager *>(env->manager);
  uv_timer_t *async_timer = static_cast<uv_timer_t *>(env->async_limiter);
  uv_loop_t *ploop = manager->getUVLoop();
  uv_timer_start(async_timer, async_limit_cb, TIME_LIMIT, 0);
  uv_run(ploop, UV_RUN_DEFAULT);

  /**
   * 代码到这里，uvloop 已经结束。这里有两种情况：
   * 一种是 调用 done 函数，由 stop_loop 调用
   * 一种是 async_limit_timer 的回调
   * async_timer 执行过后会直接被关闭
   * 因此 当 async_timer 是激活状态的话，一定是由于 done 函数，返回 0
   * 其他情况属于超时，返回 1
   */
  if (uv_is_active((uv_handle_t *)async_timer)) {
    uv_timer_stop(async_timer);
    return 0;
  } else {
    return 1;
  }
}

static int stop_loop(void *feat_test_env) {
  FeatTestEnv *env = static_cast<FeatTestEnv *>(feat_test_env);
  ferry::FeatureManager *manager =
      static_cast<ferry::FeatureManager *>(env->manager);
  uv_loop_t *ploop = manager->getUVLoop();
  uv_stop(ploop);
  return 0;
}

static JSValue getRequireObject(FeatTestEnv *env) {
  JSValue func_data = JS_NewObject(env->ctx);
  JS_SetOpaque(func_data, env->manager);
  JSValue ret = JS_NewCFunctionData(env->ctx, __require, 1, 0, 1, &func_data);
  JS_FreeValue(env->ctx, func_data);
  return ret;
}

static JSValue getTimeoutObject(FeatTestEnv *env) {
  JSValue func_data = JS_NewObject(env->ctx);
  JS_SetOpaque(func_data, &env->time_host);
  JSValue ret =
      JS_NewCFunctionData(env->ctx, __setTimeout, 1, 0, 1, &func_data);
  JS_FreeValue(env->ctx, func_data);
  return ret;
}

static void walk(uv_handle_t *handle, void *arg) {
  if (uv_is_active(handle)) {
    printf("%p is active\n", handle);
  }
}

// 支持cli来读取 js 文件去执行，命令为：./feature_jidl_test
int main(int argc, char **argv) {
  if (argc < 2) {
    printf("please input js file, like ./test.js \n");
    return 0;
  }

  char *js_file = argv[1];
  char *js_str = NULL;

  // 打开js文件
  load_file(js_file, &js_str);
  if (js_str == NULL) {
    printf("malloc js file failed!\n");
    return 0;
  }
  FeatTestEnv env;
  // initialize quickjs engine
  env.rt = JS_NewRuntime();
  env.ctx = JS_NewContext(env.rt);

  // init uv_loop
  uv_loop_t *main_loop = uv_default_loop();
  uv_prepare_t prepare;
  uv_prepare_init(main_loop, &prepare);
  prepare.data = &env;
  uv_prepare_start(&prepare, execute_job_cb);

  uv_timer_t timer;
  uv_timer_init(main_loop, &timer);
  env.async_limiter = &timer;
  timer.data = &env;

  // init feature framework
  JS_SetRuntimeOpaque(env.rt, env.ctx);
  auto registry = new ferry::FeatureRegistry();
  registry->init(nullptr);

  // TODO: use factory pattern: manager = CreateFeatureManager(registry, "js");
  ferry::FeatureManagerQjs *manager = new ferry::FeatureManagerQjs(registry);
  env.manager = manager;
  env.run_loop = run_loop;
  env.stop_loop = stop_loop;

  manager->setUserData("run_loop", &env);
  manager->setUVLoop(main_loop);

  // register global require
  JSValue global_obj = JS_GetGlobalObject(env.ctx);
  JSValue require = getRequireObject(&env);
  JS_SetPropertyStr(env.ctx, global_obj, "require", require);

  JSValue setTimeout = getTimeoutObject(&env);

  JS_SetPropertyStr(env.ctx, global_obj, "setTimeout", setTimeout);

  JS_FreeValue(env.ctx, global_obj);

  // 加载 test frame work
  // TODO: ues qjs bytecode
  // original file ../test-internal.js
  const char *test_content =
      "let unittest = require('feat_test');\n\nfunction feat_test(name, desc, "
      "cb) {\n    unittest.testsuite(name, desc, cb);\n}\n\nfunction "
      "feat_async_test(suitname, desc, test_cb) {\n    var async_id;\n    "
      "async_id = unittest.testsuite(suitname, desc, () => {\n        "
      "test_cb(() => unittest.done(async_id));\n    }, true);\n}\n\nfunction "
      "feat_test_all() {\n    unittest.run_all_tests();\n}\n\nfunction "
      "feat_expect_true(r, d) {\n    return unittest.expect_true(r, "
      "d);\n}\n\nfunction print(a) {\n    unittest.print(a);\n}\n";
  auto res = JS_Eval(env.ctx, test_content, strlen(test_content), "<eval>",
                     JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(res)) {
    const char *str = JS_ToCString(env.ctx, res);
    printf("test internal file error: %s\n", str);
    JS_FreeValue(env.ctx, res);
    return -1;
  }
  JS_FreeValue(env.ctx, res);
  // 加载 测试文件
  auto result = JS_Eval(env.ctx, js_str, strlen(js_str), "a.js",
                        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_STRICT);

  if (JS_IsException(result)) {
    const char *str = JS_ToCString(env.ctx, result);
    printf("exec js file error: %s\n", str);
    JS_FreeValue(env.ctx, result);
    return -1;
  }
  JS_FreeValue(env.ctx, result);

  // uv_run(main_loop, UV_RUN_DEFAULT);
  // clear un-triggered timers
  for (TimeCallback *tc : env.time_host.timers) {
    JS_FreeValue(tc->ctx, tc->callback);
  }
  env.time_host.timers.clear();

  // release manager first
  manager->uninit();
  JS_FreeContext(env.ctx);
  JS_FreeRuntime(env.rt);

  uv_loop_close(main_loop);

  // free js_str
  if (js_str != NULL) {
    free(js_str);
    js_str = NULL;
  }
  // free manager
  delete manager;

  return 0;
}

bool load_file(const char *file_name, char **file_content) {
  if (file_name == NULL || file_content == NULL) {
    printf("file_name or file_content is NULL!\n");
    return false;
  }

  FILE *fp = fopen(file_name, "r");
  if (fp == NULL) {
    printf("open file_name is %s failed!\n", file_name);
    return false;
  }
  // 获取文件长度
  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  *file_content = (char *)malloc(len + 1);
  memset(*file_content, 0, len + 1);
  // 读取文件内容到file_content字符串中
  fread(*file_content, len, 1, fp);
  fclose(fp);

  return true;
}