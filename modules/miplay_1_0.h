// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_MIPLAY_H_
#define JSON_AST_GEN_MODULE_MIPLAY_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void Miplay_onRegister(const char* feature_name);
  void Miplay_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Miplay_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Miplay_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Miplay_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Miplay_onUnregister(const char* feature_name);

  // Struct defines

  // Function wrappers to be implemented
  void Miplay_wrap_init(FeatureInstanceHandle feature, AppendData data, FtCallbackId cb);
  void Miplay_wrap_uninit(FeatureInstanceHandle feature, AppendData data);

  // Property getters and setters to be implemented

  // interface vtable functions to be implemented

  // Array malloc functions

#endif // JSON_AST_GEN_MODULE_MIPLAY_H_
