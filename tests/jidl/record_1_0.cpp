// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "record_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL callback 'success_cb' ******/
  static const FeatureType Record_success_cb_parameters[] = {
    FT_STRING,
    FT_PARAM_END
  };

  static const CallbackType Record_success_cb_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Record_success_cb_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL callback 'fail_cb' ******/
  static const FeatureType Record_fail_cb_parameters[] = {
    FT_STRING,
    FT_STRING,
    FT_PARAM_END
  };

  static const CallbackType Record_fail_cb_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Record_fail_cb_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL callback 'complete_cb' ******/
  static const FeatureType Record_complete_cb_parameters[] = {
    FT_PARAM_END
  };

  static const CallbackType Record_complete_cb_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Record_complete_cb_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL function 'start' ******/
  static OptionalType Record_start_param_duration_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT64,
    .lval = 1000
  };

  static OptionalType Record_start_param_sampleRate_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 8000
  };

  static OptionalType Record_start_param_numberOfChannels_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 1
  };

  static OptionalType Record_start_param_encodeBitRate_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_INT,
    .ival = 16000
  };

  static OptionalType Record_start_param_format_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = "aac"
  };

  static const FeatureType Record_start_parameters[] = {
    FT_MK_OPTIONAL(&Record_start_param_duration_opt_type),
    FT_MK_OPTIONAL(&Record_start_param_sampleRate_opt_type),
    FT_MK_OPTIONAL(&Record_start_param_numberOfChannels_opt_type),
    FT_MK_OPTIONAL(&Record_start_param_encodeBitRate_opt_type),
    FT_MK_OPTIONAL(&Record_start_param_format_opt_type),
    FT_MK_COMPLEX(&Record_success_cb_callback_type),
    FT_MK_COMPLEX(&Record_fail_cb_callback_type),
    FT_MK_COMPLEX(&Record_complete_cb_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Record_start_member_method = {
    .func = { .callback = FFI_FN(Record_wrap_start) },
    .parameters = Record_start_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'stop' ******/
  static const FeatureType Record_stop_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Record_stop_member_method = {
    .func = { .callback = FFI_FN(Record_wrap_stop) },
    .parameters = Record_stop_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Record_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "start",
      .method = Record_start_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "stop",
      .method = Record_stop_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Record_callbacks {
    Record_onRegister,
    Record_onCreate,
    Record_onRequired,
    Record_onDetached,
    Record_onDestroy,
    Record_onUnregister
  };

  static const FeatureDescription Record_desc = {
    1,
    "Record",
    "Record",
    0,
    &Record_callbacks,
    countof(Record_members),
    Record_members,
  };

QAPPFEATURE_INIT(Record)
{
    return mgr->registerFeature(features, &Record_desc);
}