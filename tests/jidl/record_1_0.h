// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_RECORD_H_
#define JSON_AST_GEN_MODULE_RECORD_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void Record_onRegister(FeatureRuntimeContext ctx);
  void Record_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Record_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Record_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Record_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Record_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines

  // Function wrappers to be implemented
  void Record_wrap_start(FeatureInstanceHandle feature, AppendData data, long duration, FtInt sampleRate, FtInt numberOfChannels, FtInt encodeBitRate, FtString format, FtCallbackId s_cb, FtCallbackId f_cb, FtCallbackId c_cb);
  void Record_wrap_stop(FeatureInstanceHandle feature, AppendData data);

  // Property getters and setters to be implemented

  // interface vtable functions to be implemented

  // Array malloc functions

#endif // JSON_AST_GEN_MODULE_RECORD_H_
