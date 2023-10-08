// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_LOCALE_H_
#define JSON_AST_GEN_MODULE_LOCALE_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void locale_onRegister(const char* feature_name);
  void locale_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void locale_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void locale_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void locale_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void locale_onUnregister(const char* feature_name);

  // Struct defines
  typedef struct _Locale {
    FtString _language;
    FtString _countryOrRegion;
  } locale_Locale;

  locale_Locale* mallocLocale();


  // Function wrappers to be implemented
  locale_Locale * locale_wrap_get(FeatureInstanceHandle feature, AppendData data);

  // Property getters and setters to be implemented

  // interface vtable functions to be implemented

  // Array malloc functions

#endif // JSON_AST_GEN_MODULE_LOCALE_H_
