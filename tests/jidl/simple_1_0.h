// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_SIMPLE_H_
#define JSON_AST_GEN_MODULE_SIMPLE_H_

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
  void Simple_onRegister(FeatureRuntimeContext ctx);
  void Simple_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Simple_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Simple_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Simple_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Simple_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines

  // Function wrappers to be implemented
  void Simple_wrap_printStr(FeatureInstanceHandle feature, AppendData data, FtString c);
  void Simple_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters vari_params);
  FtInt Simple_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtString c, FtDouble b);
  void Simple_wrap_bar(FeatureInstanceHandle feature, AppendData data);
  void Simple_wrap_bar5(FeatureInstanceHandle feature, AppendData data, FtInt a, FtVariadicParameters vari_params);
  FtString Simple_wrap_bar6(FeatureInstanceHandle feature, AppendData data, FtInt a, FtFloat b, FtBool c);
  void Simple_wrap_goo(FeatureInstanceHandle feature, AppendData data, FtInt a, FtInt b, FeatureCallbackId cb);
  void Simple_wrap_goo2(FeatureInstanceHandle feature, AppendData data, FeatureCallbackId cb, FeatureCallbackId cb3, FeatureCallbackId cb4);
  void Simple_wrap_foo2(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FeatureCallbackId cb, FeatureCallbackId cb2);
  void Simple_wrap_foo3(FeatureInstanceHandle feature, AppendData data, FtInt x, FtDouble y, FeatureCallbackId cb);
  void Simple_wrap_justTestNeverCall1(FeatureInstanceHandle feature, AppendData data);
  void Simple_wrap_justTestNeverCall2(FeatureInstanceHandle feature, AppendData data);
  FtInt Simple_wrap_bar2(FeatureInstanceHandle feature, AppendData data, FTArray& values);
  FTArray* Simple_wrap_bar3(FeatureInstanceHandle feature, AppendData data);

  // Property getters and setters to be implemented
  FtString Simple_get_name(void* feature, AppendData data);
  void Simple_set_name(void* feature, AppendData data, FtString name);
  void Simple_set_version(void* feature, AppendData data, FtString version);
  void Simple_set_args(void* feature, AppendData data, FTArray& args);

  // Array malloc functions
  FTArray* Simple_malloc_string_array();
  FTArray* Simple_malloc_int_array();

#endif // JSON_AST_GEN_MODULE_SIMPLE_H_
