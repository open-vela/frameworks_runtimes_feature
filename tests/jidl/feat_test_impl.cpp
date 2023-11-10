#include <gtest/gtest.h>
#include <stdio.h>

#include "feat_test.h"

class FeatureUnittest {
 public:
  FeatureUnittest(::testing::UnitTest* u) : unittest_(u) {}
  static int generateAsyncId() { return ++next_async_id; }
  static int currentAsyncId() { return current_async_id; }
  static void setCurrentAsyncId(int aid) { current_async_id = aid; }
  ~FeatureUnittest() {
    if (unittest_) ::testing::UnitTest::Delete(unittest_);
  }

  int runAllTests() {
    if (!unittest_) return -1;
    int r = unittest_->Run();
    if (r) {
      printf("feat_test: Test failed, handle %p\n", unittest_);
    }
    return r;
  }
  ::testing::UnitTest* getUnittest() { return unittest_; }

 private:
  static int current_async_id;
  static int next_async_id;
  ::testing::UnitTest* unittest_;
};
int FeatureUnittest::current_async_id = 0;
int FeatureUnittest::next_async_id = 0;

typedef int (*LoopFunc)(void*);
typedef struct FeatTestEnv {
  const char* filename;
  LoopFunc run_loop;
  LoopFunc stop_loop;
} FeatTestEnv;

class FeatureTest : public ::testing::Test {
 public:
  FeatureTest(FeatureInstanceHandle featureInstance, FtCallbackId cb,
              bool is_async, int async_id)
      : _featureInstance(featureInstance),
        _cb(cb),
        _is_async(is_async),
        _async_id(async_id) {}
  void TestBody() override {
    FeatureInvokeCallback(_featureInstance, _cb);
    if (!_is_async) return;

    // deal with async test
    FeatureUnittest::setCurrentAsyncId(_async_id);

    FeatTestEnv* pack =
        (FeatTestEnv*)FeatureInstanceGetUserData(_featureInstance, "run_loop");

    LoopFunc run_loop = pack->run_loop;

    if (run_loop) {
      int ret = run_loop(pack);
      if (ret) {
        EXPECT_TRUE_FILE(false, pack->filename, -1)
            << "  This test case timed out. id: " << _async_id;
      }
    }
    FeatureUnittest::setCurrentAsyncId(0);
  }

 private:
  FeatureInstanceHandle _featureInstance;
  FtCallbackId _cb;
  bool _is_async;
  int _async_id;
};

class FTTestFactory : public ::testing::internal::TestFactoryBase {
 public:
  FTTestFactory(FeatureInstanceHandle handle, FtCallbackId cb, bool is_async,
                int async_id)
      : _featureInstance(handle),
        _cb(cb),
        _is_async(is_async),
        _async_id(async_id) {}

  ::testing::Test* CreateTest() override {
    return new FeatureTest(_featureInstance, _cb, _is_async, _async_id);
  }

 private:
  FeatureInstanceHandle _featureInstance;
  FtCallbackId _cb;
  bool _is_async;
  int _async_id;
};

void feat_test_onRegister(const char* module_name) {
  printf("register module %s\n", module_name);
}

void feat_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  int argc = 1;
  const char* argv[] = {"feature google test"};
  ::testing::InitGoogleTest(&argc, const_cast<char**>(argv));
}

void feat_test_onRequired(FeatureRuntimeContext ctx,
                          FeatureInstanceHandle handle) {
  static int inited = 0;
  printf("required module feat_test %p\n", handle);
  if (inited) {
    printf("You can't require feat_test twice %p\n", handle);
    return;
  }

  ::testing::UnitTest* u = ::testing::UnitTest::Create();
  FeatureUnittest* p = new FeatureUnittest(u);
  FeatureSetObjectData(handle, p);
  return;
}

void feat_test_onDetached(FeatureRuntimeContext ctx,
                          FeatureInstanceHandle handle) {
  printf("detached feat_test %p\n", handle);
  FeatureUnittest* p =
      static_cast<FeatureUnittest*>(FeatureGetObjectData(handle));
  FeatureSetObjectData(handle, 0);
  if (p) delete p;
}

void feat_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  printf("destroy feat_test\n");
}

void feat_test_onUnregister(const char* module_name) {
  printf("unregister %s\n", module_name);
}

void feat_test_wrap_done(FeatureInstanceHandle feature, AppendData append_data,
                         FtInt async_id, FtInt err, FtString err_message) {
  if (FeatureUnittest::currentAsyncId() != async_id) {
    printf("[feat_test] done(%d) in odd test context: Timeout occurred.",
           async_id);
    return;
  }

  // printf("[feat_test] done stop a async test: id(%d)\n", async_id);
  FeatTestEnv* pack =
      (FeatTestEnv*)FeatureInstanceGetUserData(feature, "run_loop");
  LoopFunc stop_loop = pack->stop_loop;
  stop_loop(pack);
}
FtInt feat_test_wrap_testsuite(FeatureInstanceHandle feature,
                               AppendData append_data, FtString test_suit_name,
                               FtString test_case_name, FtCallbackId body,
                               FtBool is_async) {
  printf("[feat_test] add testsuite %p\n", feature);
  FeatureUnittest* p =
      static_cast<FeatureUnittest*>(FeatureGetObjectData(feature));
  ::testing::UnitTest* u = p->getUnittest();
  if (!u) return 0;

  int async_id = 0;
  if (is_async) async_id = FeatureUnittest::generateAsyncId();

  ::testing::internal::UnitTestMakeAndRegisterTestInfo(
      u, test_suit_name, test_case_name, NULL, NULL,
      ::testing::internal::CodeLocation("", 0),  // Location need more accurancy
      ::testing::internal::GetTypeId<FeatureTest>(),
      ::testing::internal::SuiteApiResolver<FeatureTest>::GetSetUpCaseOrSuite(
          "", 0),
      ::testing::internal::SuiteApiResolver<
          FeatureTest>::GetTearDownCaseOrSuite("", 0),
      new FTTestFactory(feature, body, is_async, async_id));

  return async_id;
}

void feat_test_wrap_expect_true(FeatureInstanceHandle feature, AppendData data,
                                FtBool result, FtString message_info) {
  // TODO 判断是否要执行
  FeatTestEnv* pack =
      (FeatTestEnv*)FeatureInstanceGetUserData(feature, "run_loop");
  EXPECT_TRUE_FILE(result, pack->filename, -1) << message_info;
}

void feat_test_wrap_run_all_tests(FeatureInstanceHandle feature,
                                  AppendData data) {
  printf("feat_test runAllTests %p\n", feature);
  FeatureUnittest* p =
      static_cast<FeatureUnittest*>(FeatureGetObjectData(feature));
  p->runAllTests();
}

void feat_test_wrap_print(FeatureInstanceHandle feature, AppendData append_data,
                          FtVariParams var_params) {
  printf("[feat_test print] ");
  ft_context_ref ft_ctx = FeatureGetContext(feature);
  for (int i = 0; i < var_params.vari_count; i++) {
    ft_value_t param = var_params.vari_args[i];
    ft_type param_type = ft_get_type(ft_ctx, param);
    if (param_type == FT_TYPE_OBJECT) {
      const char* param_obj = ft_to_string(ft_ctx, param);
      printf("%s ", param_obj);
      ft_free_string(ft_ctx, param_obj);
    } else if (param_type == FT_TYPE_ARRAY) {
      uint32_t array_size = ft_array_size(ft_ctx, param);
      printf("[");
      for (uint32_t j = 0; j < array_size; ++j) {
        ft_value_t elem = ft_array_at(ft_ctx, param, j);
        ft_type elem_type = ft_get_type(ft_ctx, elem);
        if (elem_type == FT_TYPE_NUMBER) {
          double param_num;
          if (ft_to_double(ft_ctx, elem, &param_num)) printf("%lf ", param_num);
        } else if (elem_type == FT_TYPE_STRING) {
          const char* param_str = ft_to_string(ft_ctx, elem);
          printf("%s ", param_str);
          ft_free_string(ft_ctx, param_str);
        } else if (elem_type == FT_TYPE_BOOL) {
          bool param_bool;
          ft_to_bool(ft_ctx, param, &param_bool);
          printf("%d ", param_bool);
        } else {
          printf("invalid array element type!");
          return;
        }
      }
      printf("] ");
    } else if (param_type == FT_TYPE_STRING) {
      const char* param_str = ft_to_string(ft_ctx, param);
      printf("%s ", param_str);
      ft_free_string(ft_ctx, param_str);
    } else if (param_type == FT_TYPE_NUMBER) {
      double param_num;
      ft_to_double(ft_ctx, param, &param_num);
      printf("%lf ", param_num);
    } else if (param_type == FT_TYPE_BOOL) {
      bool param_bool;
      ft_to_bool(ft_ctx, param, &param_bool);
      printf("%d ", param_bool);
    } else {
      printf("invalid param type!");
      return;
    }
  }
  printf("\n");
}
