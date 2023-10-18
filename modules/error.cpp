// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "error.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL function 'strerror' ******/
  static const FeatureType Error_strerror_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Error_strerror_member_method = {
    .func = { .callback = FFI_FN(Error_wrap_strerror) },
    .parameters = Error_strerror_parameters,
    .return_type = FT_STRING,
  };


  // members
  static const Member Error_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "strerror",
      .method = Error_strerror_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Error_callbacks {
    Error_onRegister,
    Error_onCreate,
    Error_onRequired,
    Error_onDetached,
    Error_onDestroy,
    Error_onUnregister
  };

  static const FeatureDescription Error_desc = {
    .version = 1,
    .name = "Error",
    .description = "Error",
    { .dynamic = false },
    .native_callbacks = &Error_callbacks,
    .member_count = countof(Error_members),
    .members = Error_members,
  };

QAPPFEATURE_INIT(Error)
{
    return mgr->registerFeature(features, &Error_desc);
}