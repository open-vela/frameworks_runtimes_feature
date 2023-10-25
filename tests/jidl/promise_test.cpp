// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "promise_test.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL function 'foo' ******/
  static const FeatureType promise_test_foo_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_PARAM_END
  };

  static const PromiseType promise_test_promise_FT_INT_FT_INT_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_INT, FT_INT }
  };

  static const MemberMethod promise_test_foo_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_foo) },
    .parameters = promise_test_foo_parameters,
    .return_type = FT_MK_COMPLEX_REF(&promise_test_promise_FT_INT_FT_INT_type),
  };


  /****** for JIDL use 'use_foo' ******/
  static void promise_test_wrap_use_foo (FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a) {
    promise_test_wrap_foo (feature, data, pid, a, "hello");
  }

  static const FeatureType promise_test_use_foo_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod promise_test_use_foo_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_use_foo) },
    .parameters = promise_test_use_foo_parameters,
    .return_type = FT_MK_COMPLEX_REF(&promise_test_promise_FT_INT_FT_INT_type),
  };


  /****** for JIDL function 'foo1' ******/
  static const FeatureType promise_test_foo1_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const PromiseType promise_test_promise_FT_INT_FT_STRING_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_INT, FT_STRING }
  };

  static const MemberMethod promise_test_foo1_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_foo1) },
    .parameters = promise_test_foo1_parameters,
    .return_type = FT_MK_COMPLEX_REF(&promise_test_promise_FT_INT_FT_STRING_type),
  };


  /****** for JIDL function 'foo2' ******/
  static const FeatureType promise_test_foo2_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod promise_test_foo2_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_foo2) },
    .parameters = promise_test_foo2_parameters,
    .return_type = FT_MK_COMPLEX_REF(&promise_test_promise_FT_INT_FT_STRING_type),
  };


  /****** for JIDL function 'bar' ******/
  static const FeatureType promise_test_bar_parameters[] = {
    FT_PARAM_END
  };

  static const ArrayType promise_test_int_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_INT
  };

  FtArray* promise_test_malloc_int_array() {
    return (FtArray*)FeatureMalloc(
      sizeof(FtArray), FT_MK_COMPLEX(&promise_test_int_array));
  }

  static const ArrayType promise_test_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_STRING
  };

  FtArray* promise_test_malloc_string_array() {
    return (FtArray*)FeatureMalloc(
      sizeof(FtArray), FT_MK_COMPLEX(&promise_test_string_array));
  }

  static const PromiseType promise_test_promise_int_array_string_array_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&promise_test_int_array), FT_MK_COMPLEX_REF(&promise_test_string_array) }
  };

  static const MemberMethod promise_test_bar_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_bar) },
    .parameters = promise_test_bar_parameters,
    .return_type = FT_MK_COMPLEX_REF(&promise_test_promise_int_array_string_array_type),
  };


  /****** for JIDL function 'bar1' ******/
  static const FeatureType promise_test_bar1_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod promise_test_bar1_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_bar1) },
    .parameters = promise_test_bar1_parameters,
    .return_type = FT_MK_COMPLEX_REF(&promise_test_promise_int_array_string_array_type),
  };


  /****** for JIDL function 'bar2' ******/
  static const FeatureType promise_test_bar2_parameters[] = {
    FT_PARAM_END
  };

  static const PromiseType promise_test_promise_int_array_FT_STRING_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&promise_test_int_array), FT_STRING }
  };

  static const MemberMethod promise_test_bar2_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_bar2) },
    .parameters = promise_test_bar2_parameters,
    .return_type = FT_MK_COMPLEX_REF(&promise_test_promise_int_array_FT_STRING_type),
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType promise_test_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod promise_test_print_member_method = {
    .func = { .callback = FFI_FN(promise_test_wrap_print) },
    .parameters = promise_test_print_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member promise_test_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "foo",
      .method = promise_test_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "use_foo",
      .method = promise_test_use_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo1",
      .method = promise_test_foo1_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo2",
      .method = promise_test_foo2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar",
      .method = promise_test_bar_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar1",
      .method = promise_test_bar1_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar2",
      .method = promise_test_bar2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = promise_test_print_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks promise_test_callbacks {
    promise_test_onRegister,
    promise_test_onCreate,
    promise_test_onRequired,
    promise_test_onDetached,
    promise_test_onDestroy,
    promise_test_onUnregister
  };

  static const FeatureDescription promise_test_desc = {
    .version = 1,
    .name = "promise_test",
    .description = "promise_test",
    { .dynamic = false },
    .native_callbacks = &promise_test_callbacks,
    .member_count = countof(promise_test_members),
    .members = promise_test_members,
  };

QAPPFEATURE_INIT(promise_test)
{
    return mgr->registerFeature(features, &promise_test_desc);
}
