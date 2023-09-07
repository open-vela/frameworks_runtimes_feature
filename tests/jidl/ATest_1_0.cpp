// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "ATest_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL function 'test1' ******/
  static const FeatureType ATest_1_0_test1_parameters[] = {
    FT_STRING,
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod ATest_1_0_test1_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_test1),
    .parameters = ATest_1_0_test1_parameters,
    .return_type = FT_STRING,
  };

  // members
  static const Member ATest_1_0_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "test1",
      .method = ATest_1_0_test1_member_method,
    }
  };

  // callbacks
  static const struct FeatureCallbacks ATest_1_0_callbacks {
    ATest_1_0_onRegister,
    ATest_1_0_onCreate,
    ATest_1_0_onRequired,
    ATest_1_0_onDetached,
    ATest_1_0_onDestroy,
    ATest_1_0_onUnregister
  };

  static const FeatureDescription ATest_1_0_desc = {
    1,
    "ATest_1_0",
    "ATest_1_0",
    1,
    &ATest_1_0_callbacks,
    countof(ATest_1_0_members),
    ATest_1_0_members,
  };

QAPPFEATURE_INIT(ATest_1_0)
{
    return mgr->registerFeature(features, &ATest_1_0_desc);
}