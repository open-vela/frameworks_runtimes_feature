// Copyright 2023 Xiaomi, Inc. All rights reserved.


#include "interface_1_0.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** JIDL interface 'Animal' glue code begin ******/
  extern const InterfaceType Interface_Animal_interface_type;
  // for member property 'name'
  static const MemberAccessor Interface_Animal_interface_name_member_accessor = {
    .getter = { .vtable_idx = 1 },
    .setter = { .vtable_idx = 2 },
    .type = FT_STRING,
  };

  // for member property 'legCount'
  static const MemberAccessor Interface_Animal_interface_legCount_member_accessor = {
    .getter = { .vtable_idx = 3 },
    .type = FT_INT,
  };

  // for member method 'eatFood'
  static const ArrayType Interface_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_STRING
  };

  FtArray* Interface_malloc_string_array() {
    return (FtArray*)FeatureMalloc(
      sizeof(FtArray), FT_MK_COMPLEX(&Interface_string_array));
  }

  static const FeatureType Interface_Animal_interface_eatFood_parameters[] = {
    FT_MK_COMPLEX_REF(&Interface_string_array),
    FT_PARAM_END
  };

  static const MemberMethod Interface_Animal_interface_eatFood_member_method = {
    .func = { .vtable_idx = 4 },
    .parameters = Interface_Animal_interface_eatFood_parameters,
    .return_type = FT_INT,
  };

  // for member method 'run'
  static const FeatureType Interface_Animal_interface_run_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_PARAM_END
  };

  static const MemberMethod Interface_Animal_interface_run_member_method = {
    .func = { .vtable_idx = 5 },
    .parameters = Interface_Animal_interface_run_parameters,
    .return_type = FT_STRING,
  };

  // Interface members
  static const Member Interface_Animal_interface_members[] = {
    // Animal interface members
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
    .header = { .type = COMPLEX_INTERFACE, .size = 0 },
    .desc = &Interface_Animal_interface_desc
  };
  /****** JIDL interface 'Animal' glue code end ******/

  /****** for JIDL Interface constructor function 'createDog' ******/
  FeatureInterfaceHandle Interface_createDog_instance(FeatureInstanceHandle feature, AppendData data, FtInt type) {
    static NativeFunc dog_vtable[] = {
        NativeFunc(Interface_Animal_interface_dog_finalize),
        NativeFunc(Interface_Animal_interface_dog_get_name),
        NativeFunc(Interface_Animal_interface_dog_set_name),
        NativeFunc(Interface_Animal_interface_dog_get_legCount),
        NativeFunc(Interface_Animal_interface_dog_eatFood),
        NativeFunc(Interface_Animal_interface_dog_run),
    };
    return FeatureCreateInterface(feature, dog_vtable, countof(dog_vtable));
  }

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


  /****** JIDL interface 'Bird' glue code begin ******/
  extern const InterfaceType Interface_Bird_interface_type;
  // for member method 'fly'
  static const FeatureType Interface_Bird_interface_fly_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_Bird_interface_fly_member_method = {
    .func = { .vtable_idx = 1 },
    .parameters = Interface_Bird_interface_fly_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_string_array),
  };

  // for member property 'breed'
  static const MemberAccessor Interface_Bird_interface_breed_member_accessor = {
    .getter = { .vtable_idx = 2 },
    .setter = { .vtable_idx = 3 },
    .type = FT_STRING,
  };

  // Interface members
  static const Member Interface_Bird_interface_members[] = {
    // Bird interface members
    {
      .type = MEMBER_METHOD,
      .name = "fly",
      .method = Interface_Bird_interface_fly_member_method,
    },
    {
      .type = MEMBER_ACCESSOR,
      .name = "breed",
      .accessor = Interface_Bird_interface_breed_member_accessor,
    },
  };

  // Interface description
  static const FeatureDescription Interface_Bird_interface_desc = {
    .version = 1,
    .name = "Bird",
    .description = "Bird description",
    { .dynamic = true },
    nullptr,
    countof(Interface_Bird_interface_members),
    Interface_Bird_interface_members,
  };

  // InterfaceType
  const InterfaceType Interface_Bird_interface_type {
    .header = { .type = COMPLEX_INTERFACE, .size = 0 },
    .desc = &Interface_Bird_interface_desc
  };
  /****** JIDL interface 'Bird' glue code end ******/

  /****** for JIDL Interface constructor function 'createPigeon' ******/
  FeatureInterfaceHandle Interface_createPigeon_instance(FeatureInstanceHandle feature, AppendData data) {
    static NativeFunc pigeon_vtable[] = {
        NativeFunc(Interface_Bird_interface_pigeon_finalize),
        NativeFunc(Interface_Bird_interface_pigeon_fly),
        NativeFunc(Interface_Bird_interface_pigeon_get_breed),
        NativeFunc(Interface_Bird_interface_pigeon_set_breed),
    };
    return FeatureCreateInterface(feature, pigeon_vtable, countof(pigeon_vtable));
  }

  /****** for JIDL function 'createPigeon' ******/
  static const FeatureType Interface_createPigeon_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_createPigeon_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_createPigeon) },
    .parameters = Interface_createPigeon_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_Bird_interface_type),
  };


  /****** JIDL interface 'Chicken' glue code begin ******/
  extern const InterfaceType Interface_Chicken_interface_type;
  // Overrided parent member defines
  static const MemberAccessor Interface_Chicken_Animal_interface_name_member_accessor = {
    .getter = { .vtable_idx = 1 },
    .setter = { .vtable_idx = 2 },
    .type = FT_STRING,
  };

  static const MemberAccessor Interface_Chicken_Animal_interface_legCount_member_accessor = {
    .getter = { .vtable_idx = 3 },
    .type = FT_INT,
  };

  static const MemberMethod Interface_Chicken_Animal_interface_eatFood_member_method = {
    .func = { .vtable_idx = 4 },
    .parameters = Interface_Animal_interface_eatFood_parameters,
    .return_type = FT_INT,
  };

  static const MemberMethod Interface_Chicken_Animal_interface_run_member_method = {
    .func = { .vtable_idx = 5 },
    .parameters = Interface_Animal_interface_run_parameters,
    .return_type = FT_STRING,
  };

  static const MemberMethod Interface_Chicken_Bird_interface_fly_member_method = {
    .func = { .vtable_idx = 6 },
    .parameters = Interface_Bird_interface_fly_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_string_array),
  };

  static const MemberAccessor Interface_Chicken_Bird_interface_breed_member_accessor = {
    .getter = { .vtable_idx = 7 },
    .setter = { .vtable_idx = 8 },
    .type = FT_STRING,
  };

  // for member property 'weight'
  static const MemberAccessor Interface_Chicken_interface_weight_member_accessor = {
    .getter = { .vtable_idx = 9 },
    .setter = { .vtable_idx = 10 },
    .type = FT_INT,
  };

  // for member method 'walk'
  static const FeatureType Interface_Chicken_interface_walk_parameters[] = {
    FT_PARAM_END
  };

  static const PromiseType Interface_promise_string_array_FT_INT_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&Interface_string_array), FT_INT }
  };

  static const MemberMethod Interface_Chicken_interface_walk_member_method = {
    .func = { .vtable_idx = 11 },
    .parameters = Interface_Chicken_interface_walk_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_promise_string_array_FT_INT_type),
  };

  // Interface members
  static const Member Interface_Chicken_interface_members[] = {
    // overrided parent members
    {
      .type = MEMBER_ACCESSOR,
      .name = "name",
      .accessor = Interface_Chicken_Animal_interface_name_member_accessor,
    },
    {
      .type = MEMBER_ACCESSOR,
      .name = "legCount",
      .accessor = Interface_Chicken_Animal_interface_legCount_member_accessor,
    },
    {
      .type = MEMBER_METHOD,
      .name = "eatFood",
      .method = Interface_Chicken_Animal_interface_eatFood_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "run",
      .method = Interface_Chicken_Animal_interface_run_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "fly",
      .method = Interface_Chicken_Bird_interface_fly_member_method,
    },
    {
      .type = MEMBER_ACCESSOR,
      .name = "breed",
      .accessor = Interface_Chicken_Bird_interface_breed_member_accessor,
    },
    // Chicken interface members
    {
      .type = MEMBER_ACCESSOR,
      .name = "weight",
      .accessor = Interface_Chicken_interface_weight_member_accessor,
    },
    {
      .type = MEMBER_METHOD,
      .name = "walk",
      .method = Interface_Chicken_interface_walk_member_method,
    },
  };

  // Interface description
  static const FeatureDescription Interface_Chicken_interface_desc = {
    .version = 1,
    .name = "Chicken",
    .description = "Chicken description",
    { .dynamic = true },
    nullptr,
    countof(Interface_Chicken_interface_members),
    Interface_Chicken_interface_members,
  };

  // InterfaceType
  const InterfaceType Interface_Chicken_interface_type {
    .header = { .type = COMPLEX_INTERFACE, .size = 0 },
    .desc = &Interface_Chicken_interface_desc
  };
  /****** JIDL interface 'Chicken' glue code end ******/

  /****** for JIDL Interface constructor function 'createCock' ******/
  FeatureInterfaceHandle Interface_createCock_instance(FeatureInstanceHandle feature, AppendData data) {
    static NativeFunc cock_vtable[] = {
        NativeFunc(Interface_Chicken_interface_cock_finalize),
        NativeFunc(Interface_Chicken_interface_cock_get_name),
        NativeFunc(Interface_Chicken_interface_cock_set_name),
        NativeFunc(Interface_Chicken_interface_cock_get_legCount),
        NativeFunc(Interface_Chicken_interface_cock_eatFood),
        NativeFunc(Interface_Chicken_interface_cock_run),
        NativeFunc(Interface_Chicken_interface_cock_fly),
        NativeFunc(Interface_Chicken_interface_cock_get_breed),
        NativeFunc(Interface_Chicken_interface_cock_set_breed),
        NativeFunc(Interface_Chicken_interface_cock_get_weight),
        NativeFunc(Interface_Chicken_interface_cock_set_weight),
        NativeFunc(Interface_Chicken_interface_cock_walk),
    };
    return FeatureCreateInterface(feature, cock_vtable, countof(cock_vtable));
  }

  /****** for JIDL function 'createCock' ******/
  static const FeatureType Interface_createCock_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_createCock_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_createCock) },
    .parameters = Interface_createCock_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_Chicken_interface_type),
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


  /****** for JIDL function 'flyFar' ******/
  static const FeatureType Interface_flyFar_parameters[] = {
    FT_INT,
    FT_PARAM_END
  };

  static const MemberMethod Interface_flyFar_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_flyFar) },
    .parameters = Interface_flyFar_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_promise_string_array_FT_INT_type),
  };


  /****** for JIDL use 'flyAway' ******/
  static void Interface_wrap_flyAway (FeatureInstanceHandle feature, AppendData data, FtPromiseId pid) {
    Interface_wrap_flyFar (feature, data, pid, 100);
  }

  static const FeatureType Interface_flyAway_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Interface_flyAway_member_method = {
    .func = { .callback = FFI_FN(Interface_wrap_flyAway) },
    .parameters = Interface_flyAway_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Interface_promise_string_array_FT_INT_type),
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
      .name = "createDog",
      .method = Interface_createDog_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "createPigeon",
      .method = Interface_createPigeon_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "createCock",
      .method = Interface_createCock_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "createCat",
      .method = Interface_createCat_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "setAnimal",
      .method = Interface_setAnimal_member_method,
    },
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