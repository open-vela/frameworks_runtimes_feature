// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "promise_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL function 'foo' ******/
  static const FeatureType Promise_1_0_foo_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_PARAM_END
  };

  static const PromiseType Promise_1_0_promise_FT_INT_FT_INT_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FeaturePromiseHandle) },
    .resolveTypes = { FT_INT, FT_INT }
  };

  static const MemberMethod Promise_1_0_foo_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_foo),
    .parameters = Promise_1_0_foo_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_1_0_promise_FT_INT_FT_INT_type),
  };


  /****** for JIDL use 'use_foo' ******/
  static void Promise_1_0_wrap_use_foo (FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle, FtInt a) {
    Promise_1_0_wrap_foo (feature, data, promiseHandle, a, "hello");
  }

  static const FeatureType Promise_1_0_use_foo_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Promise_1_0_use_foo_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_use_foo),
    .parameters = Promise_1_0_use_foo_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_1_0_promise_FT_INT_FT_INT_type),
  };


  /****** for JIDL function 'foo1' ******/
  static const FeatureType Promise_1_0_foo1_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const PromiseType Promise_1_0_promise_FT_INT_FT_STRING_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FeaturePromiseHandle) },
    .resolveTypes = { FT_INT, FT_STRING }
  };

  static const MemberMethod Promise_1_0_foo1_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_foo1),
    .parameters = Promise_1_0_foo1_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_1_0_promise_FT_INT_FT_STRING_type),
  };


  /****** for JIDL function 'foo2' ******/
  static const FeatureType Promise_1_0_foo2_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Promise_1_0_foo2_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_foo2),
    .parameters = Promise_1_0_foo2_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_1_0_promise_FT_INT_FT_STRING_type),
  };


  /****** for JIDL function 'bar' ******/
  static const FeatureType Promise_1_0_bar_parameters[] = {
    FT_PARAM_END
  };

  static const ArrayType Promise_1_0_int_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FTArray) },
    .element_type = FT_INT
  };

  static const ArrayType Promise_1_0_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FTArray) },
    .element_type = FT_STRING
  };

  static const PromiseType Promise_1_0_promise_int_array_string_array_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FeaturePromiseHandle) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&Promise_1_0_int_array), FT_MK_COMPLEX_REF(&Promise_1_0_string_array) }
  };

  static const MemberMethod Promise_1_0_bar_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_bar),
    .parameters = Promise_1_0_bar_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_1_0_promise_int_array_string_array_type),
  };


  /****** for JIDL function 'bar1' ******/
  static const FeatureType Promise_1_0_bar1_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Promise_1_0_bar1_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_bar1),
    .parameters = Promise_1_0_bar1_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_1_0_promise_int_array_string_array_type),
  };


  /****** for JIDL function 'bar2' ******/
  static const FeatureType Promise_1_0_bar2_parameters[] = {
    FT_PARAM_END
  };

  static const PromiseType Promise_1_0_promise_int_array_FT_STRING_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FeaturePromiseHandle) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&Promise_1_0_int_array), FT_STRING }
  };

  static const MemberMethod Promise_1_0_bar2_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_bar2),
    .parameters = Promise_1_0_bar2_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Promise_1_0_promise_int_array_FT_STRING_type),
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType Promise_1_0_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod Promise_1_0_print_member_method = {
    .callback = FFI_FN(Promise_1_0_wrap_print),
    .parameters = Promise_1_0_print_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Promise_1_0_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "foo",
      .method = Promise_1_0_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "use_foo",
      .method = Promise_1_0_use_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo1",
      .method = Promise_1_0_foo1_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo2",
      .method = Promise_1_0_foo2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar",
      .method = Promise_1_0_bar_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar1",
      .method = Promise_1_0_bar1_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar2",
      .method = Promise_1_0_bar2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = Promise_1_0_print_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Promise_1_0_callbacks {
    Promise_1_0_onRegister,
    Promise_1_0_onCreate,
    Promise_1_0_onRequired,
    Promise_1_0_onDetached,
    Promise_1_0_onDestroy,
    Promise_1_0_onUnregister
  };

  static const FeatureDescription Promise_1_0_desc = {
    1,
    "Promise_1_0",
    "Promise_1_0",
    1,
    &Promise_1_0_callbacks,
    countof(Promise_1_0_members),
    Promise_1_0_members,
  };

QAPPFEATURE_INIT(Promise_1_0)
{
    return mgr->registerFeature(features, &Promise_1_0_desc);
}