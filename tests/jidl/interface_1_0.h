// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_INTERFACE_H_
#define JSON_AST_GEN_MODULE_INTERFACE_H_

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
  void Interface_onRegister(FeatureRuntimeContext ctx);
  void Interface_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Interface_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Interface_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Interface_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Interface_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines

  // Function wrappers to be implemented
  void Interface_wrap_flyFar(FeatureInstanceHandle feature, AppendData data, FtInt distance);
  FeatureInstanceHandle Interface_wrap_createCat(FeatureInstanceHandle feature, AppendData data);
  FeatureInstanceHandle Interface_wrap_createDog(FeatureInstanceHandle feature, AppendData data, FtInt type);
  void Interface_wrap_setAnimal(FeatureInstanceHandle feature, AppendData data, FeatureInstanceHandle animal);
  void Interface_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters vari_params);

  // Property getters and setters to be implemented

  // Array malloc functions

#endif // JSON_AST_GEN_MODULE_INTERFACE_H_
