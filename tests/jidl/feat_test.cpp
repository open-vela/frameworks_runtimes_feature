// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "feat_test.h"
#include "ajs_features_init.h"
#include "feature_description.h"
#include "feature_main_exports.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL callback 'test_body' ******/
  static const FeatureType feat_test_test_body_parameters[] = {
    FT_PARAM_END
  };

  static const CallbackType feat_test_test_body_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = feat_test_test_body_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL function 'testsuite' ******/
  static OptionalType feat_test_testsuite_param_is_async_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_BOOLEAN,
    .ival = false
  };

  static const FeatureType feat_test_testsuite_parameters[] = {
    FT_STRING,
    FT_STRING,
    FT_MK_COMPLEX_REF(&feat_test_test_body_callback_type),
    FT_MK_OPTIONAL(&feat_test_testsuite_param_is_async_opt_type),
    FT_PARAM_END
  };

  static const MemberMethod feat_test_testsuite_member_method = {
    .func = { .callback = FFI_FN(feat_test_wrap_testsuite) },
    .parameters = feat_test_testsuite_parameters,
    .return_type = FT_INT,
  };


  /****** for JIDL function 'done' ******/
  static OptionalType feat_test_done_param_err_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 0
  };

  static OptionalType feat_test_done_param_err_message_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = "success"
  };

  static const FeatureType feat_test_done_parameters[] = {
    FT_INT,
    FT_MK_OPTIONAL(&feat_test_done_param_err_opt_type),
    FT_MK_OPTIONAL(&feat_test_done_param_err_message_opt_type),
    FT_PARAM_END
  };

  static const MemberMethod feat_test_done_member_method = {
    .func = { .callback = FFI_FN(feat_test_wrap_done) },
    .parameters = feat_test_done_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'expect_true' ******/
  static OptionalType feat_test_expect_true_param_message_info_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = ""
  };

  static const FeatureType feat_test_expect_true_parameters[] = {
    FT_BOOLEAN,
    FT_MK_OPTIONAL(&feat_test_expect_true_param_message_info_opt_type),
    FT_PARAM_END
  };

  static const MemberMethod feat_test_expect_true_member_method = {
    .func = { .callback = FFI_FN(feat_test_wrap_expect_true) },
    .parameters = feat_test_expect_true_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'run_all_tests' ******/
  static const FeatureType feat_test_run_all_tests_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod feat_test_run_all_tests_member_method = {
    .func = { .callback = FFI_FN(feat_test_wrap_run_all_tests) },
    .parameters = feat_test_run_all_tests_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType feat_test_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod feat_test_print_member_method = {
    .func = { .callback = FFI_FN(feat_test_wrap_print) },
    .parameters = feat_test_print_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member feat_test_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "testsuite",
      .method = &feat_test_testsuite_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "done",
      .method = &feat_test_done_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "expect_true",
      .method = &feat_test_expect_true_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "run_all_tests",
      .method = &feat_test_run_all_tests_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = &feat_test_print_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks feat_test_callbacks {
    feat_test_onRegister,
    feat_test_onCreate,
    feat_test_onRequired,
    feat_test_onDetached,
    feat_test_onDestroy,
    feat_test_onUnregister
  };

  static const FeatureDescription feat_test_desc = {
    .version = 1,
    .name = "feat_test",
    .description = "feat_test",
    { .dynamic = false },
    .native_callbacks = &feat_test_callbacks,
    .member_count = countof(feat_test_members),
    .members = feat_test_members,
  };

QAPPFEATURE_INIT(feat_test)
{
    return FeatureRegisterFeature(handle, &feat_test_desc);
}
