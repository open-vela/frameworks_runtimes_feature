// Copyright 2023 Xiaomi, Inc. All rights reserved.




#include "interface_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** JIDL interface 'Animal' glue code begin ******/
extern const InterfaceType Interface_Animal_interface_type;
  /****** for JIDL property 'Animal_interface_name' ******/
  static const MemberAccessor Interface_Animal_interface_name_member_accessor = {
    .getter = { .vtable_idx = 1 },
    .setter = { .vtable_idx = 2 },
    .type = FT_STRING,
  };

  /****** for JIDL property 'Animal_interface_legCount' ******/
  static const MemberAccessor Interface_Animal_interface_legCount_member_accessor = {
    .getter = { .vtable_idx = 3 },
    .type = FT_INT,
  };

  /****** for JIDL function 'Animal_interface_eatFood' ******/
  static const FeatureType Interface_Animal_interface_eatFood_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_Animal_interface_eatFood_member_method = {
    .func = { .vtable_idx = 4 },
    .parameters = Interface_Animal_interface_eatFood_parameters,
    .return_type = FT_VOID,
  };

  /****** for JIDL function 'Animal_interface_run' ******/
  static const FeatureType Interface_Animal_interface_run_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_Animal_interface_run_member_method = {
    .func = { .vtable_idx = 5 },
    .parameters = Interface_Animal_interface_run_parameters,
    .return_type = FT_VOID,
  };

  /****** for JIDL function 'Animal_interface_fly' ******/
  static const FeatureType Interface_Animal_interface_fly_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_Animal_interface_fly_member_method = {
    .func = { .vtable_idx = 6 },
    .parameters = Interface_Animal_interface_fly_parameters,
    .return_type = FT_VOID,
  };

  // Interface members
  static const Member Interface_Animal_interface_members[] = {
    {
      .type = MEMBER_ACCESSOR,
      .name = "name",
      .accessor = Interface_Animal_interface_name_member_accessor,
    },
    {
      .type = MEMBER_ACCESSOR,
      .name = "legCount",
      .accessor = Interface_Animal_interface_legCount_member_accessor,
    },
    {
      .type = MEMBER_METHOD,
      .name = "eatFood",
      .method = Interface_Animal_interface_eatFood_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "run",
      .method = Interface_Animal_interface_run_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "fly",
      .method = Interface_Animal_interface_fly_member_method,
    },
  };

  // Interface description
  static const FeatureDescription Interface_Animal_interface_desc = {
    .version = 1,
    .name = "Animal",
    .description = "Animal description",
    { .dynamic = true },
    nullptr,
    countof(Interface_Animal_interface_members),
    Interface_Animal_interface_members,
  };

  // InterfaceType
  const InterfaceType Interface_Animal_interface_type {
    .header = { .type = ferry::COMPLEX_INTERFACE, .size = 0 },
    .desc = &Interface_Animal_interface_desc
  };
  /****** JIDL interface 'Animal' glue code end ******/

  /****** for JIDL function 'flyFar' ******/
  static const FeatureType Interface_flyFar_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Interface_flyFar_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_flyFar) },
    .parameters = Interface_flyFar_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL use 'flyAway' ******/
  static void Interface_wrap_flyAway (FeatureInstanceHandle feature, AppendData data) {
    Interface_wrap_flyFar (feature, data, 100);
  }

  static const FeatureType Interface_flyAway_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_flyAway_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_flyAway) },
    .parameters = Interface_flyAway_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'createCat' ******/
  static const FeatureType Interface_createCat_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_createCat_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_createCat) },
    .parameters = Interface_createCat_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_Animal_interface_type),
  };


  /****** for JIDL function 'createDog' ******/
  static const FeatureType Interface_createDog_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Interface_createDog_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_createDog) },
    .parameters = Interface_createDog_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_Animal_interface_type),
  };


  /****** for JIDL function 'setAnimal' ******/
  static const FeatureType Interface_setAnimal_parameters[] = {
    FT_MK_COMPLEX_REF(&Interface_Animal_interface_type),
    FT_PARAM_END
  };

  static const MemberMethod Interface_setAnimal_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_setAnimal) },
    .parameters = Interface_setAnimal_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType Interface_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod Interface_print_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_print) },
    .parameters = Interface_print_parameters,
    .return_type = FT_VOID,
  };


  // members
  static const Member Interface_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "flyFar",
      .method = Interface_flyFar_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "flyAway",
      .method = Interface_flyAway_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "createCat",
      .method = Interface_createCat_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "createDog",
      .method = Interface_createDog_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "setAnimal",
      .method = Interface_setAnimal_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = Interface_print_member_method,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Interface_callbacks {
    Interface_onRegister,
    Interface_onCreate,
    Interface_onRequired,
    Interface_onDetached,
    Interface_onDestroy,
    Interface_onUnregister
  };

  static const FeatureDescription Interface_desc = {
    .version = 1,
    .name = "Interface",
    .description = "Interface",
    { .dynamic = false },
    .native_callbacks = &Interface_callbacks,
    .member_count = countof(Interface_members),
    .members = Interface_members,
  };

QAPPFEATURE_INIT(Interface)
{
    return mgr->registerFeature(features, &Interface_desc);
}