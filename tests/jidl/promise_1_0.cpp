// Copyright 2023 Xiaomi, Inc. All rights reserved.




#include "promise_1_0.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL function 'foo' ******/
  static const FeatureType Promise_foo_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_PARAM_END
  };

  static const PromiseType Promise_promise_FT_INT_FT_INT_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_INT, FT_INT }
  };

  static const MemberMethod Promise_foo_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_foo) },
    .parameters = Promise_foo_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_promise_FT_INT_FT_INT_type),
  };


  /****** for JIDL use 'use_foo' ******/
  static void Promise_wrap_use_foo (FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a) {
    Promise_wrap_foo (feature, data, pid, a, "hello");
  }

  static const FeatureType Promise_use_foo_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Promise_use_foo_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_use_foo) },
    .parameters = Promise_use_foo_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_promise_FT_INT_FT_INT_type),
  };


  /****** for JIDL function 'foo1' ******/
  static const FeatureType Promise_foo1_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const PromiseType Promise_promise_FT_INT_FT_STRING_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_INT, FT_STRING }
  };

  static const MemberMethod Promise_foo1_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_foo1) },
    .parameters = Promise_foo1_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_promise_FT_INT_FT_STRING_type),
  };


  /****** for JIDL function 'foo2' ******/
  static const FeatureType Promise_foo2_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Promise_foo2_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_foo2) },
    .parameters = Promise_foo2_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_promise_FT_INT_FT_STRING_type),
  };


  /****** for JIDL function 'bar' ******/
  static const FeatureType Promise_bar_parameters[] = {
    FT_PARAM_END
  };

  static const ArrayType Promise_int_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_INT
  };

  FtArray* Promise_malloc_int_array() {
    return (FtArray*)FeatureMalloc(
      sizeof(FtArray), FT_MK_COMPLEX(&Promise_int_array));
  }

  static const ArrayType Promise_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_STRING
  };

  FtArray* Promise_malloc_string_array() {
    return (FtArray*)FeatureMalloc(
      sizeof(FtArray), FT_MK_COMPLEX(&Promise_string_array));
  }

  static const PromiseType Promise_promise_int_array_string_array_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&Promise_int_array), FT_MK_COMPLEX_REF(&Promise_string_array) }
  };

  static const MemberMethod Promise_bar_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_bar) },
    .parameters = Promise_bar_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_promise_int_array_string_array_type),
  };


  /****** for JIDL function 'bar1' ******/
  static const FeatureType Promise_bar1_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Promise_bar1_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_bar1) },
    .parameters = Promise_bar1_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_promise_int_array_string_array_type),
  };


  /****** for JIDL function 'bar2' ******/
  static const FeatureType Promise_bar2_parameters[] = {
    FT_PARAM_END
  };

  static const PromiseType Promise_promise_int_array_FT_STRING_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&Promise_int_array), FT_STRING }
  };

  static const MemberMethod Promise_bar2_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_bar2) },
    .parameters = Promise_bar2_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_promise_int_array_FT_STRING_type),
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType Promise_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod Promise_print_member_method = {
    .func = { .callback = FFI_FN(Promise_wrap_print) },
    .parameters = Promise_print_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Promise_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "foo",
      .method = Promise_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "use_foo",
      .method = Promise_use_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo1",
      .method = Promise_foo1_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo2",
      .method = Promise_foo2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar",
      .method = Promise_bar_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar1",
      .method = Promise_bar1_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar2",
      .method = Promise_bar2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = Promise_print_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Promise_callbacks {
    Promise_onRegister,
    Promise_onCreate,
    Promise_onRequired,
    Promise_onDetached,
    Promise_onDestroy,
    Promise_onUnregister
  };

  static const FeatureDescription Promise_desc = {
    .version = 1,
    .name = "Promise",
    .description = "Promise",
    { .dynamic = false },
    .native_callbacks = &Promise_callbacks,
    .member_count = countof(Promise_members),
    .members = Promise_members,
  };

QAPPFEATURE_INIT(Promise)
{
    return mgr->registerFeature(features, &Promise_desc);
}