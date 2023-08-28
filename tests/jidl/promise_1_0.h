// Copyright 2023 Xiaomi, Inc. All rights reserved.



#ifndef JSON_AST_GEN_MODULE_PROMISE_1_0_H_
#define JSON_AST_GEN_MODULE_PROMISE_1_0_H_

#include "feature_exports.h"
#include "feature_log.h"
#include "feature_ffi.h"
#include "feature.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  using namespace FEATURE;
  using namespace ferry;

  // FeatureCallbacks to be implemented
  void Promise_1_0_onRegister(FeatureRuntimeContext ctx);
  void Promise_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Promise_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Promise_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Promise_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Promise_1_0_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines

  // Function wrappers to be implemented
  void Promise_1_0_wrap_foo(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle, FtInt a, FtString b);
  void Promise_1_0_wrap_foo1(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle, FtInt a);
  void Promise_1_0_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle);
  void Promise_1_0_wrap_bar(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle);
  void Promise_1_0_wrap_bar1(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle);
  void Promise_1_0_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle);
  void Promise_1_0_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters vari_params);

  // Property getters and setters to be implemented

#endif // JSON_AST_GEN_MODULE_PROMISE_1_0_H_
