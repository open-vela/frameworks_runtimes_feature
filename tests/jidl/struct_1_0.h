// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_STRUCT_H_
#define JSON_AST_GEN_MODULE_STRUCT_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void Struct_onRegister(const char* feature_name);
  void Struct_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Struct_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Struct_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Struct_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Struct_onUnregister(const char* feature_name);

  // Struct defines
  typedef struct _Chapter {
    FtInt _page_count;
    FtString _title;
    FtBool _is_end;
  } Struct_Chapter;

  Struct_Chapter* mallocChapter();

  typedef struct _Book {
    FtAny _anyparameter;
    FtInt _page_count;
    FtString _title;
    FtArray* _chap_titles;
    Struct_Chapter * _first_chap;
    FtCallbackId _chap_changed;
  } Struct_Book;

  Struct_Book* mallocBook();


  // Function wrappers to be implemented
  void Struct_wrap_foo(FeatureInstanceHandle feature, AppendData data, FtInt a, Struct_Chapter * b);
  Struct_Chapter * Struct_wrap_bar(FeatureInstanceHandle feature, AppendData data, FtInt a);
  void Struct_wrap_bar2(FeatureInstanceHandle feature, AppendData data, Struct_Book * a);
  void Struct_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params);

  // Property getters and setters to be implemented

  // interface vtable functions to be implemented

  // Array malloc functions
  FtArray* Struct_malloc_string_array();

#endif // JSON_AST_GEN_MODULE_STRUCT_H_
