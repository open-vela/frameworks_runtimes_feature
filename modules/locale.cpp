// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "locale.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL struct 'Locale' ******/
  static ObjectMember locale_Locale_struct_members[] = {
    { "language", FT_STRING, offsetof(locale_Locale, _language), sizeof(FtString) },
    { "countryOrRegion", FT_STRING, offsetof(locale_Locale, _countryOrRegion), sizeof(FtString) },
    { nullptr },
  };

  // complex defination
  static const ObjectMapType locale_Locale_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(locale_Locale) },
    .members = locale_Locale_struct_members
  };

  locale_Locale* mallocLocale () {
    return (locale_Locale*)FeatureMalloc(
      sizeof(locale_Locale), FT_MK_COMPLEX(&locale_Locale_struct_type));
  }


  /****** for JIDL function 'get' ******/
  static const FeatureType locale_get_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod locale_get_member_method = {
    .func = { .callback = FFI_FN(locale_wrap_get) },
    .parameters = locale_get_parameters,
    .return_type = FT_MK_COMPLEX_REF(&locale_Locale_struct_type),
  };


  // members
  static const Member locale_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "get",
      .method = locale_get_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks locale_callbacks {
    locale_onRegister,
    locale_onCreate,
    locale_onRequired,
    locale_onDetached,
    locale_onDestroy,
    locale_onUnregister
  };

  static const FeatureDescription locale_desc = {
    .version = 1,
    .name = "locale",
    .description = "locale",
    { .dynamic = false },
    .native_callbacks = &locale_callbacks,
    .member_count = countof(locale_members),
    .members = locale_members,
  };

QAPPFEATURE_INIT(locale)
{
    return mgr->registerFeature(features, &locale_desc);
}