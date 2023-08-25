// Copyright 2023 Xiaomi, Inc. All rights reserved.



#ifndef JSON_AST_GEN_MODULE_STRUCT_1_0_H_
#define JSON_AST_GEN_MODULE_STRUCT_1_0_H_

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
  void Struct_1_0_onRegister(FeatureRuntimeContext ctx);
  void Struct_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Struct_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Struct_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Struct_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Struct_1_0_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines
  typedef struct _Chapter {
    FtInt _page_count;
    FtString _title;
    FtBool _is_end;
  } Struct_1_0_Chapter;

  Struct_1_0_Chapter* mallocChapter();

  typedef struct _Book {
    FtInt _page_count;
    FtString _title;
    FTArray* _chap_titles;
    Struct_1_0_Chapter * _first_chap;
    FeatureCallbackId _chap_changed;
  } Struct_1_0_Book;

  Struct_1_0_Book* mallocBook();


  // Function wrappers to be implemented
  void Struct_1_0_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, Struct_1_0_Chapter * b);
  Struct_1_0_Chapter * Struct_1_0_wrap_bar(FeatureInstanceHandle feature, AppendData data, FtInt a);
  void Struct_1_0_wrap_bar2(FeatureInstanceHandle feature, AppendData data, Struct_1_0_Book * a);
  void Struct_1_0_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariadicParameters variadicParameters);

  // Property getters and setters to be implemented

#endif // JSON_AST_GEN_MODULE_STRUCT_1_0_H_
