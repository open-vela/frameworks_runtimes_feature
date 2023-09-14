// Copyright 2023 Xiaomi, Inc. All rights reserved.




#include "struct_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  static OptionalType Struct_Chapter_member_page_count_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 10
  };

  static OptionalType Struct_Chapter_member_title_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = "hello world"
  };

  static OptionalType Struct_Chapter_member_is_end_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_BOOLEAN,
    .ival = false
  };

  /****** for JIDL struct 'Chapter' ******/
  static ObjectMember Struct_Chapter_struct_members[] = {
    { "page_count", FT_MK_OPTIONAL(&Struct_Chapter_member_page_count_opt_type), offsetof(Struct_Chapter, _page_count), sizeof(FtInt) },
    { "title", FT_MK_OPTIONAL(&Struct_Chapter_member_title_opt_type), offsetof(Struct_Chapter, _title), sizeof(FtString) },
    { "is_end", FT_MK_OPTIONAL(&Struct_Chapter_member_is_end_opt_type), offsetof(Struct_Chapter, _is_end), sizeof(FtBool) },
    { nullptr },
  };

  // complex defination
  static const ObjectMapType Struct_Chapter_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(Struct_Chapter) },
    .members = Struct_Chapter_struct_members
  };

  Struct_Chapter* mallocChapter () {
    return (Struct_Chapter*)FTMalloc(
      sizeof(Struct_Chapter), FT_MK_COMPLEX(&Struct_Chapter_struct_type));
  }


  /****** for JIDL callback 'ChapChanged' ******/
  static const FeatureType Struct_ChapChanged_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_PARAM_END
  };

  static const CallbackType Struct_ChapChanged_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Struct_ChapChanged_parameters,
    .return_type = FT_VOID
  };


  static const ArrayType Struct_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FTArray) },
    .element_type = FT_STRING
  };

  FTArray* Struct_malloc_string_array() {
    return (FTArray*)FTMalloc(
      sizeof(FTArray), FT_MK_COMPLEX(&Struct_string_array));
  }

  /****** for JIDL struct 'Book' ******/
  static ObjectMember Struct_Book_struct_members[] = {
    { "page_count", FT_INT, offsetof(Struct_Book, _page_count), sizeof(FtInt) },
    { "title", FT_STRING, offsetof(Struct_Book, _title), sizeof(FtString) },
    { "chap_titles", FT_MK_COMPLEX_REF(&Struct_string_array), offsetof(Struct_Book, _chap_titles), sizeof(FTArray*) },
    { "first_chap", FT_MK_COMPLEX_REF(&Struct_Chapter_struct_type), offsetof(Struct_Book, _first_chap), sizeof(Struct_Chapter *) },
    { "chap_changed", FT_MK_COMPLEX(&Struct_ChapChanged_callback_type), offsetof(Struct_Book, _chap_changed), sizeof(FeatureCallbackId) },
    { nullptr },
  };

  // complex defination
  static const ObjectMapType Struct_Book_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(Struct_Book) },
    .members = Struct_Book_struct_members
  };

  Struct_Book* mallocBook () {
    return (Struct_Book*)FTMalloc(
      sizeof(Struct_Book), FT_MK_COMPLEX(&Struct_Book_struct_type));
  }


  /****** for JIDL function 'foo' ******/
  static const FeatureType Struct_foo_parameters[] = {
    FT_INT,
    FT_MK_COMPLEX_REF(&Struct_Chapter_struct_type),
    FT_PARAM_END
  };

  static const MemberMethod Struct_foo_member_method = {
    .func = { .callback = FFI_FN(Struct_wrap_foo) },
    .parameters = Struct_foo_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'bar' ******/
  static const FeatureType Struct_bar_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Struct_bar_member_method = {
    .func = { .callback = FFI_FN(Struct_wrap_bar) },
    .parameters = Struct_bar_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Struct_Chapter_struct_type),
  };


  /****** for JIDL function 'bar2' ******/
  static const FeatureType Struct_bar2_parameters[] = {
    FT_MK_COMPLEX_REF(&Struct_Book_struct_type),
    FT_PARAM_END
  };

  static const MemberMethod Struct_bar2_member_method = {
    .func = { .callback = FFI_FN(Struct_wrap_bar2) },
    .parameters = Struct_bar2_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType Struct_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod Struct_print_member_method = {
    .func = { .callback = FFI_FN(Struct_wrap_print) },
    .parameters = Struct_print_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Struct_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "foo",
      .method = Struct_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar",
      .method = Struct_bar_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar2",
      .method = Struct_bar2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = Struct_print_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Struct_callbacks {
    Struct_onRegister,
    Struct_onCreate,
    Struct_onRequired,
    Struct_onDetached,
    Struct_onDestroy,
    Struct_onUnregister
  };

  static const FeatureDescription Struct_desc = {
    .version = 1,
    .name = "Struct",
    .description = "Struct",
    { .dynamic = false },
    .native_callbacks = &Struct_callbacks,
    .member_count = countof(Struct_members),
    .members = Struct_members,
  };

QAPPFEATURE_INIT(Struct)
{
    return mgr->registerFeature(features, &Struct_desc);
}