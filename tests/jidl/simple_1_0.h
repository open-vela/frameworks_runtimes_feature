// Copyright 2023 Xiaomi, Inc. All rights reserved.



#ifndef JSON_AST_GEN_MODULE_SIMPLE_1_0_H_
#define JSON_AST_GEN_MODULE_SIMPLE_1_0_H_

#include "feature_exports.h"
#include "feature_log.h"
#include "feature_framework.h"
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
  void Simple_1_0_onRegister(FeatureRuntimeContext ctx);
  void Simple_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Simple_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Simple_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Simple_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Simple_1_0_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines

  // Function wrappers to be implemented
  void Simple_1_0_wrap_printStr(FeatureInstanceHandle feature, AppendData data, FtString c);
  void Simple_1_0_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters variadicParameters);
  FtInt Simple_1_0_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtString c, FtDouble b);
  void Simple_1_0_wrap_bar(FeatureInstanceHandle feature, AppendData data);
  void Simple_1_0_wrap_bar5(FeatureInstanceHandle feature, AppendData data, FtInt a, FtVariadicParameters variadicParameters);
  FtString Simple_1_0_wrap_bar6(FeatureInstanceHandle feature, AppendData data, FtInt a, FtFloat b, FtBool c);
  void Simple_1_0_wrap_goo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtInt b, FeatureCallbackId cb);
  void Simple_1_0_wrap_goo2(FeatureInstanceHandle feature, AppendData data, FeatureCallbackId cb, FeatureCallbackId cb3, FeatureCallbackId cb4);
  void Simple_1_0_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FeatureCallbackId cb, FeatureCallbackId cb2);
  void Simple_1_0_wrap_foo3(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FeatureCallbackId cb);
  void Simple_1_0_wrap_justTestNeverCall1(FeatureInstanceHandle feature, AppendData data);
  void Simple_1_0_wrap_justTestNeverCall2(FeatureInstanceHandle feature, AppendData data);
  FtInt Simple_1_0_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FTArray& values);
  FTArray Simple_1_0_wrap_bar3(FeatureInstanceHandle feature, AppendData data);

  // Property getters and setters to be implemented
  FtString Simple_1_0_get_name(void* feature, AppendData data);
  void Simple_1_0_set_name(void* feature, AppendData data, FtString name);
  void Simple_1_0_set_version(void* feature, AppendData data, FtString version);
  void Simple_1_0_set_args(void* feature, AppendData data, FTArray& args);

#endif // JSON_AST_GEN_MODULE_SIMPLE_1_0_H_
