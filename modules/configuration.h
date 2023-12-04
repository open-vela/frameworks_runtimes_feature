// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_CONFIGURATION_H_
#define JSON_AST_GEN_MODULE_CONFIGURATION_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void configuration_onRegister(const char* feature_name);
  void configuration_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void configuration_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void configuration_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void configuration_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void configuration_onUnregister(const char* feature_name);

  // Struct defines
  typedef struct _Configuration {
    FtString _language;
    FtString _countryOrRegion;
  } configuration_Configuration;

  configuration_Configuration* configurationMallocConfiguration();


  // Function wrappers to be implemented
  configuration_Configuration * configuration_wrap_getLocale(FeatureInstanceHandle feature, AppendData data);

  // Interface constructors

  // interface vtable functions to be implemented

  // Property getters and setters to be implemented

  // Array malloc functions

#endif // JSON_AST_GEN_MODULE_CONFIGURATION_H_
