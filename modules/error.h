// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_ERROR_H_
#define JSON_AST_GEN_MODULE_ERROR_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void Error_onRegister(const char* feature_name);
  void Error_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Error_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Error_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Error_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Error_onUnregister(const char* feature_name);

  // Struct defines

  // Function wrappers to be implemented
  FtString Error_wrap_strerror(FeatureInstanceHandle feature, AppendData data, FtInt errnum);

  // Property getters and setters to be implemented

  // interface vtable functions to be implemented

  // Array malloc functions

#endif // JSON_AST_GEN_MODULE_ERROR_H_
