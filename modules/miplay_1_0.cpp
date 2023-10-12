// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "miplay_1_0.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL callback 'setMediainfoCb' ******/
  static const FeatureType Miplay_setMediainfoCb_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_STRING,
    FT_INT,
    FT_INT,
    FT_PARAM_END
  };

  static const CallbackType Miplay_setMediainfoCb_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = Miplay_setMediainfoCb_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL function 'init' ******/
  static const FeatureType Miplay_init_parameters[] = {
    FT_MK_COMPLEX(&Miplay_setMediainfoCb_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Miplay_init_member_method = {
    .func = { .callback = FFI_FN(Miplay_wrap_init) },
    .parameters = Miplay_init_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'uninit' ******/
  static const FeatureType Miplay_uninit_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Miplay_uninit_member_method = {
    .func = { .callback = FFI_FN(Miplay_wrap_uninit) },
    .parameters = Miplay_uninit_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Miplay_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "init",
      .method = Miplay_init_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "uninit",
      .method = Miplay_uninit_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Miplay_callbacks {
    Miplay_onRegister,
    Miplay_onCreate,
    Miplay_onRequired,
    Miplay_onDetached,
    Miplay_onDestroy,
    Miplay_onUnregister
  };

  static const FeatureDescription Miplay_desc = {
    .version = 1,
    .name = "Miplay",
    .description = "Miplay",
    { .dynamic = false },
    .native_callbacks = &Miplay_callbacks,
    .member_count = countof(Miplay_members),
    .members = Miplay_members,
  };

QAPPFEATURE_INIT(Miplay)
{
    return mgr->registerFeature(features, &Miplay_desc);
}