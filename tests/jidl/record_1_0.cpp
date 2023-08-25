// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "record_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL callback 'success_cb' ******/
  static const FeatureType Record_1_0_success_cb_parameters[] = {
    FT_STRING,
    FT_PARAM_END
  };

  static const CallbackType Record_1_0_success_cb_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Record_1_0_success_cb_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL callback 'fail_cb' ******/
  static const FeatureType Record_1_0_fail_cb_parameters[] = {
    FT_STRING,
    FT_STRING,
    FT_PARAM_END
  };

  static const CallbackType Record_1_0_fail_cb_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Record_1_0_fail_cb_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL callback 'complete_cb' ******/
  static const FeatureType Record_1_0_complete_cb_parameters[] = {
    FT_PARAM_END
  };

  static const CallbackType Record_1_0_complete_cb_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Record_1_0_complete_cb_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL function 'start' ******/
  static OptionalType Record_1_0_start_param_duration_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT64,
    .lval = 1000
  };

  static OptionalType Record_1_0_start_param_sampleRate_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 8000
  };

  static OptionalType Record_1_0_start_param_numberOfChannels_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 1
  };

  static OptionalType Record_1_0_start_param_encodeBitRate_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 16000
  };

  static OptionalType Record_1_0_start_param_format_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = "aac"
  };

  static const FeatureType Record_1_0_start_parameters[] = {
    FT_MK_OPTIONAL(&Record_1_0_start_param_duration_opt_type),
    FT_MK_OPTIONAL(&Record_1_0_start_param_sampleRate_opt_type),
    FT_MK_OPTIONAL(&Record_1_0_start_param_numberOfChannels_opt_type),
    FT_MK_OPTIONAL(&Record_1_0_start_param_encodeBitRate_opt_type),
    FT_MK_OPTIONAL(&Record_1_0_start_param_format_opt_type),
    FT_MK_COMPLEX(&Record_1_0_success_cb_callback_type),
    FT_MK_COMPLEX(&Record_1_0_fail_cb_callback_type),
    FT_MK_COMPLEX(&Record_1_0_complete_cb_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Record_1_0_start_member_method = {
    .callback = FFI_FN(Record_1_0_wrap_start),
    .parameters = Record_1_0_start_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'stop' ******/
  static const FeatureType Record_1_0_stop_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Record_1_0_stop_member_method = {
    .callback = FFI_FN(Record_1_0_wrap_stop),
    .parameters = Record_1_0_stop_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Record_1_0_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "start",
      .method = Record_1_0_start_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "stop",
      .method = Record_1_0_stop_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Record_1_0_callbacks {
    Record_1_0_onRegister,
    Record_1_0_onCreate,
    Record_1_0_onRequired,
    Record_1_0_onDetached,
    Record_1_0_onDestroy,
    Record_1_0_onUnregister
  };

  static const FeatureDescription Record_1_0_desc = {
    1,
    "Record_1_0",
    "Record_1_0",
    1,
    &Record_1_0_callbacks,
    countof(Record_1_0_members),
    Record_1_0_members,
  };

QAPPFEATURE_INIT(Record_1_0)
{
    return mgr->registerFeature(features, &Record_1_0_desc);
}
