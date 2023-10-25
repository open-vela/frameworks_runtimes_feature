// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_PROMISE_TEST_H_
#define JSON_AST_GEN_MODULE_PROMISE_TEST_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void promise_test_onRegister(const char* feature_name);
  void promise_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void promise_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void promise_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void promise_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void promise_test_onUnregister(const char* feature_name);

  // Struct defines

  // Function wrappers to be implemented
  void promise_test_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a, FtString b);
  void promise_test_wrap_foo1(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a);
  void promise_test_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid);
  void promise_test_wrap_bar(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid);
  void promise_test_wrap_bar1(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid);
  void promise_test_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid);
  void promise_test_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params);

  // Interface constructors

  // interface vtable functions to be implemented

  // Property getters and setters to be implemented

  // Array malloc functions
  FtArray* promise_test_malloc_string_array();
  FtArray* promise_test_malloc_int_array();

#endif // JSON_AST_GEN_MODULE_PROMISE_TEST_H_
