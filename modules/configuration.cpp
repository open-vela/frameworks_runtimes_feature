// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "configuration.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL struct 'Configuration' ******/
  static ObjectMember configuration_Configuration_struct_members[] = {
    { "language", FT_STRING, offsetof(configuration_Configuration, _language), sizeof(FtString) },
    { "countryOrRegion", FT_STRING, offsetof(configuration_Configuration, _countryOrRegion), sizeof(FtString) },
    { nullptr },
  };

  // complex defination
  static const ObjectMapType configuration_Configuration_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(configuration_Configuration) },
    .members = configuration_Configuration_struct_members
  };

  configuration_Configuration* configurationMallocConfiguration () {
    return (configuration_Configuration*)FeatureMalloc(
      sizeof(configuration_Configuration), FT_MK_COMPLEX(&configuration_Configuration_struct_type));
  }


  /****** for JIDL function 'getLocale' ******/
  static const FeatureType configuration_getLocale_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod configuration_getLocale_member_method = {
    .func = { .callback = FFI_FN(configuration_wrap_getLocale) },
    .parameters = configuration_getLocale_parameters,
    .return_type = FT_MK_COMPLEX_REF(&configuration_Configuration_struct_type),
  };


  // members
  static const Member configuration_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "getLocale",
      .method = configuration_getLocale_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks configuration_callbacks {
    configuration_onRegister,
    configuration_onCreate,
    configuration_onRequired,
    configuration_onDetached,
    configuration_onDestroy,
    configuration_onUnregister
  };

  static const FeatureDescription configuration_desc = {
    .version = 1,
    .name = "configuration",
    .description = "configuration",
    { .dynamic = false },
    .native_callbacks = &configuration_callbacks,
    .member_count = countof(configuration_members),
    .members = configuration_members,
  };

QAPPFEATURE_INIT(configuration)
{
    return mgr->registerFeature(features, &configuration_desc);
}
