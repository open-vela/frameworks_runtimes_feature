// Copyright 2023 Xiaomi, Inc. All rights reserved.



#ifndef JSON_AST_GEN_MODULE_RECORD_1_0_H_
#define JSON_AST_GEN_MODULE_RECORD_1_0_H_

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
  void Record_1_0_onRegister(FeatureRuntimeContext ctx);
  void Record_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Record_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Record_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Record_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Record_1_0_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines

  // Function wrappers to be implemented
  void Record_1_0_wrap_start(FeatureInstanceHandle feature, AppendData data, long duration, FtInt sampleRate, FtInt numberOfChannels, FtInt encodeBitRate, FtString format, FeatureCallbackId s_cb, FeatureCallbackId f_cb, FeatureCallbackId c_cb);
  void Record_1_0_wrap_stop(FeatureInstanceHandle feature, AppendData data);

  // Property getters and setters to be implemented

#endif // JSON_AST_GEN_MODULE_RECORD_1_0_H_
