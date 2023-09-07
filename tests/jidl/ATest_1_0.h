// Copyright 2023 Xiaomi, Inc. All rights reserved.



#ifndef JSON_AST_GEN_MODULE_ATEST_1_0_H_
#define JSON_AST_GEN_MODULE_ATEST_1_0_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  using namespace FEATURE;
  using namespace ferry;

  // FeatureCallbacks to be implemented
  void ATest_1_0_onRegister(FeatureRuntimeContext ctx);
  void ATest_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void ATest_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void ATest_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void ATest_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void ATest_1_0_onUnregister(FeatureRuntimeContext ctx);

  // Function wrappers to be implemented
  FtString ATest_1_0_wrap_test1(FeatureInstanceHandle feature, AppendData data, FtString a, FtInt b);

#endif // JSON_AST_GEN_MODULE_ATEST_1_0_H_