// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "struct_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  static OptionalType Struct_1_0_Chapter_member_page_count_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 10
  };

  static OptionalType Struct_1_0_Chapter_member_title_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = "hello world"
  };

  static OptionalType Struct_1_0_Chapter_member_is_end_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_BOOLEAN,
    .ival = false
  };

  /****** for JIDL struct 'Chapter' ******/
  static ObjectMember Struct_1_0_Chapter_struct_members[] = {
    { "page_count", FT_MK_OPTIONAL(&Struct_1_0_Chapter_member_page_count_opt_type), offsetof(Struct_1_0_Chapter, _page_count), sizeof(FtInt) },
    { "title", FT_MK_OPTIONAL(&Struct_1_0_Chapter_member_title_opt_type), offsetof(Struct_1_0_Chapter, _title), sizeof(FtString) },
    { "is_end", FT_MK_OPTIONAL(&Struct_1_0_Chapter_member_is_end_opt_type), offsetof(Struct_1_0_Chapter, _is_end), sizeof(FtBool) },
    { nullptr },
  };

  // complex defination
  static const ObjectMapType Struct_1_0_Chapter_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(Struct_1_0_Chapter) },
    .members = Struct_1_0_Chapter_struct_members
  };

  Struct_1_0_Chapter* mallocChapter () {
    return (Struct_1_0_Chapter*)FTMalloc(
      sizeof(Struct_1_0_Chapter), FT_MK_COMPLEX(&Struct_1_0_Chapter_struct_type));
  }


  /****** for JIDL callback 'ChapChanged' ******/
  static const FeatureType Struct_1_0_ChapChanged_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_PARAM_END
  };

  static const CallbackType Struct_1_0_ChapChanged_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Struct_1_0_ChapChanged_parameters,
    .return_type = FT_VOID
  };


  static const ArrayType Struct_1_0_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FTArray) },
    .element_type = FT_STRING
  };

  /****** for JIDL struct 'Book' ******/
  static ObjectMember Struct_1_0_Book_struct_members[] = {
    { "page_count", FT_INT, offsetof(Struct_1_0_Book, _page_count), sizeof(FtInt) },
    { "title", FT_STRING, offsetof(Struct_1_0_Book, _title), sizeof(FtString) },
    { "chap_titles", FT_MK_COMPLEX_REF(&Struct_1_0_string_array), offsetof(Struct_1_0_Book, _chap_titles), sizeof(FTArray*) },
    { "first_chap", FT_MK_COMPLEX_REF(&Struct_1_0_Chapter_struct_type), offsetof(Struct_1_0_Book, _first_chap), sizeof(Struct_1_0_Chapter *) },
    { "chap_changed", FT_MK_COMPLEX(&Struct_1_0_ChapChanged_callback_type), offsetof(Struct_1_0_Book, _chap_changed), sizeof(FeatureCallbackId) },
    { nullptr },
  };

  // complex defination
  static const ObjectMapType Struct_1_0_Book_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(Struct_1_0_Book) },
    .members = Struct_1_0_Book_struct_members
  };

  Struct_1_0_Book* mallocBook () {
    return (Struct_1_0_Book*)FTMalloc(
      sizeof(Struct_1_0_Book), FT_MK_COMPLEX(&Struct_1_0_Book_struct_type));
  }


  /****** for JIDL function 'foo' ******/
  static const FeatureType Struct_1_0_foo_parameters[] = {
    FT_INT,
    FT_MK_COMPLEX_REF(&Struct_1_0_Chapter_struct_type),
    FT_PARAM_END
  };

  static const MemberMethod Struct_1_0_foo_member_method = {
    .callback = FFI_FN(Struct_1_0_wrap_foo),
    .parameters = Struct_1_0_foo_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'bar' ******/
  static const FeatureType Struct_1_0_bar_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Struct_1_0_bar_member_method = {
    .callback = FFI_FN(Struct_1_0_wrap_bar),
    .parameters = Struct_1_0_bar_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Struct_1_0_Chapter_struct_type),
  };


  /****** for JIDL function 'bar2' ******/
  static const FeatureType Struct_1_0_bar2_parameters[] = {
    FT_MK_COMPLEX_REF(&Struct_1_0_Book_struct_type),
    FT_PARAM_END
  };

  static const MemberMethod Struct_1_0_bar2_member_method = {
    .callback = FFI_FN(Struct_1_0_wrap_bar2),
    .parameters = Struct_1_0_bar2_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType Struct_1_0_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod Struct_1_0_print_member_method = {
    .callback = FFI_FN(Struct_1_0_wrap_print),
    .parameters = Struct_1_0_print_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Struct_1_0_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "foo",
      .method = Struct_1_0_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar",
      .method = Struct_1_0_bar_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar2",
      .method = Struct_1_0_bar2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = Struct_1_0_print_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Struct_1_0_callbacks {
    Struct_1_0_onRegister,
    Struct_1_0_onCreate,
    Struct_1_0_onRequired,
    Struct_1_0_onDetached,
    Struct_1_0_onDestroy,
    Struct_1_0_onUnregister
  };

  static const FeatureDescription Struct_1_0_desc = {
    1,
    "Struct_1_0",
    "Struct_1_0",
    1,
    &Struct_1_0_callbacks,
    countof(Struct_1_0_members),
    Struct_1_0_members,
  };

QAPPFEATURE_INIT(Struct_1_0)
{
    return mgr->registerFeature(features, &Struct_1_0_desc);
}