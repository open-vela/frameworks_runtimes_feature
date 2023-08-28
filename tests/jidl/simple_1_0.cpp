// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "simple_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

  /****** for JIDL function 'printStr' ******/
  static const FeatureType Simple_1_0_printStr_parameters[] = {
    FT_STRING,
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_printStr_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_printStr),
    .parameters = Simple_1_0_printStr_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'print' ******/
  static const FeatureType Simple_1_0_print_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const MemberMethod Simple_1_0_print_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_print),
    .parameters = Simple_1_0_print_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'foo' ******/
  static OptionalType Simple_1_0_foo_param_b_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_DOUBLE,
    .fval = 1.0
  };

  static const FeatureType Simple_1_0_foo_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_MK_OPTIONAL(&Simple_1_0_foo_param_b_opt_type),
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_foo_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_foo),
    .parameters = Simple_1_0_foo_parameters,
    .return_type = FT_INT,
  };


  /****** for JIDL function 'bar' ******/
  static const FeatureType Simple_1_0_bar_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_bar_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_bar),
    .parameters = Simple_1_0_bar_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'bar5' ******/
  static const FeatureType Simple_1_0_bar5_parameters[] = {
    FT_INT,
    FT_PARAM_REST_END,
  };

  static const MemberMethod Simple_1_0_bar5_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_bar5),
    .parameters = Simple_1_0_bar5_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'bar6' ******/
  static const FeatureType Simple_1_0_bar6_parameters[] = {
    FT_INT,
    FT_FLOAT,
    FT_BOOLEAN,
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_bar6_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_bar6),
    .parameters = Simple_1_0_bar6_parameters,
    .return_type = FT_STRING,
  };


  /****** for JIDL use 'ubar6' ******/
  static FtString Simple_1_0_wrap_ubar6 (FeatureInstanceHandle feature, AppendData data, FtFloat a) {
    return Simple_1_0_wrap_bar6 (feature, data, 1, a, false);
  }

  static const FeatureType Simple_1_0_ubar6_parameters[] = {
    FT_FLOAT,
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_ubar6_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_ubar6),
    .parameters = Simple_1_0_ubar6_parameters,
    .return_type = FT_STRING,
  };


  /****** for JIDL callback 'cb1' ******/
  static const FeatureType Simple_1_0_cb1_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_DOUBLE,
    FT_PARAM_END
  };

  static const CallbackType Simple_1_0_cb1_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Simple_1_0_cb1_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL callback 'cb2' ******/
  static const FeatureType Simple_1_0_cb2_parameters[] = {
    FT_INT,
    FT_STRING,
    FT_PARAM_REST_END,
  };

  static const CallbackType Simple_1_0_cb2_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Simple_1_0_cb2_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL callback 'cb3' ******/
  static const FeatureType Simple_1_0_cb3_parameters[] = {
    FT_PARAM_END
  };

  static const CallbackType Simple_1_0_cb3_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Simple_1_0_cb3_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL callback 'cb4' ******/
  static const FeatureType Simple_1_0_cb4_parameters[] = {
    FT_PARAM_REST_END,
  };

  static const CallbackType Simple_1_0_cb4_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId) },
    .parameters = Simple_1_0_cb4_parameters,
    .return_type = FT_VOID
  };


  /****** for JIDL function 'goo' ******/
  static const FeatureType Simple_1_0_goo_parameters[] = {
    FT_INT,
    FT_INT,
    FT_MK_COMPLEX(&Simple_1_0_cb1_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_goo_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_goo),
    .parameters = Simple_1_0_goo_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'goo2' ******/
  static const FeatureType Simple_1_0_goo2_parameters[] = {
    FT_MK_COMPLEX(&Simple_1_0_cb2_callback_type),
    FT_MK_COMPLEX(&Simple_1_0_cb3_callback_type),
    FT_MK_COMPLEX(&Simple_1_0_cb4_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_goo2_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_goo2),
    .parameters = Simple_1_0_goo2_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL use 'goo3' ******/
  static void Simple_1_0_wrap_goo3 (FeatureInstanceHandle feature, AppendData data, FeatureCallbackId cb) {
    Simple_1_0_wrap_goo (feature, data, 100, 200, cb);
  }

  static const FeatureType Simple_1_0_goo3_parameters[] = {
    FT_MK_COMPLEX(&Simple_1_0_cb1_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_goo3_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_goo3),
    .parameters = Simple_1_0_goo3_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'foo2' ******/
  static const FeatureType Simple_1_0_foo2_parameters[] = {
    FT_INT,
    FT_DOUBLE,
    FT_MK_COMPLEX(&Simple_1_0_cb1_callback_type),
    FT_MK_COMPLEX(&Simple_1_0_cb2_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_foo2_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_foo2),
    .parameters = Simple_1_0_foo2_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'foo3' ******/
  static const FeatureType Simple_1_0_foo3_parameters[] = {
    FT_INT,
    FT_DOUBLE,
    FT_MK_COMPLEX(&Simple_1_0_cb1_callback_type),
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_foo3_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_foo3),
    .parameters = Simple_1_0_foo3_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'justTestNeverCall1' ******/
  static const FeatureType Simple_1_0_justTestNeverCall1_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_justTestNeverCall1_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_justTestNeverCall1),
    .parameters = Simple_1_0_justTestNeverCall1_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'justTestNeverCall2' ******/
  static const FeatureType Simple_1_0_justTestNeverCall2_parameters[] = {
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_justTestNeverCall2_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_justTestNeverCall2),
    .parameters = Simple_1_0_justTestNeverCall2_parameters,
    .return_type = FT_VOID,
  };


  /****** for JIDL function 'bar2' ******/
  static const ArrayType Simple_1_0_int_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FTArray) },
    .element_type = FT_INT
  };

  static const FeatureType Simple_1_0_bar2_parameters[] = {
    FT_MK_COMPLEX_REF(&Simple_1_0_int_array),
    FT_PARAM_END
  };

  static const MemberMethod Simple_1_0_bar2_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_bar2),
    .parameters = Simple_1_0_bar2_parameters,
    .return_type = FT_INT,
  };


  /****** for JIDL function 'bar3' ******/
  static const FeatureType Simple_1_0_bar3_parameters[] = {
    FT_PARAM_END
  };

  static const ArrayType Simple_1_0_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FTArray) },
    .element_type = FT_STRING
  };

  static const MemberMethod Simple_1_0_bar3_member_method = {
    .callback = FFI_FN(Simple_1_0_wrap_bar3),
    .parameters = Simple_1_0_bar3_parameters,
    .return_type = FT_MK_COMPLEX_REF(&Simple_1_0_string_array),
  };


  /****** for JIDL const 'x' ******/
  const FtInt Simple_1_0_g_const_x = 1;
  const FtInt Simple_1_0_init_const_x(FeatureInstanceHandle feature, AppendData data) { return Simple_1_0_g_const_x; };

  static const MemberConst Simple_1_0_x_member_const = {
    .type = FT_INT,
    //.callback = FFI_FN(Simple_1_0_init_const_x),
    .callback = nullptr,
    .data = { .i32 = Simple_1_0_g_const_x }
  };

  /****** for JIDL const 'y' ******/
  FtString Simple_1_0_g_const_y = "hello world";
  FtString Simple_1_0_init_const_y(FeatureInstanceHandle feature, AppendData data) { return Simple_1_0_g_const_y; };

  static const MemberConst Simple_1_0_y_member_const = {
    .type = FT_STRING,
    //.callback = FFI_FN(Simple_1_0_init_const_y),
    .callback = nullptr,
    .data = { .str = Simple_1_0_g_const_y }
  };

  /****** for JIDL const 'z' ******/
  const FtDouble Simple_1_0_g_const_z = 9.8;
  const FtDouble Simple_1_0_init_const_z(FeatureInstanceHandle feature, AppendData data) { return Simple_1_0_g_const_z; };

  static const MemberConst Simple_1_0_z_member_const = {
    .type = FT_DOUBLE,
    //.callback = FFI_FN(Simple_1_0_init_const_z),
    .callback = nullptr,
    .data = { .f64 = Simple_1_0_g_const_z }
  };

  /****** for JIDL property 'name' ******/
  static const MemberAccessor Simple_1_0_name_member_accessor = {
    .getter = FFI_FN(Simple_1_0_get_name),
    .setter = FFI_FN(Simple_1_0_set_name),
    .type = FT_STRING,
  };

  /****** for JIDL property 'version' ******/
  static const MemberAccessor Simple_1_0_version_member_accessor = {
    .setter = FFI_FN(Simple_1_0_set_version),
    .type = FT_STRING,
  };

  /****** for JIDL property 'args' ******/
  static const MemberAccessor Simple_1_0_args_member_accessor = {
    .setter = FFI_FN(Simple_1_0_set_args),
    .type = FT_MK_COMPLEX_REF(&Simple_1_0_string_array),
  };

  // members
  static const Member Simple_1_0_members[] = {
    {
      .type = MEMBER_METHOD,
      .name = "printStr",
      .method = Simple_1_0_printStr_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "print",
      .method = Simple_1_0_print_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo",
      .method = Simple_1_0_foo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar",
      .method = Simple_1_0_bar_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar5",
      .method = Simple_1_0_bar5_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar6",
      .method = Simple_1_0_bar6_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "ubar6",
      .method = Simple_1_0_ubar6_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "goo",
      .method = Simple_1_0_goo_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "goo2",
      .method = Simple_1_0_goo2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "goo3",
      .method = Simple_1_0_goo3_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo2",
      .method = Simple_1_0_foo2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "foo3",
      .method = Simple_1_0_foo3_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "justTestNeverCall1",
      .method = Simple_1_0_justTestNeverCall1_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "justTestNeverCall2",
      .method = Simple_1_0_justTestNeverCall2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar2",
      .method = Simple_1_0_bar2_member_method,
    },
    {
      .type = MEMBER_METHOD,
      .name = "bar3",
      .method = Simple_1_0_bar3_member_method,
    },
    {
      .type = MEMBER_CONST,
      .name = "x",
      .value = Simple_1_0_x_member_const,
    },
    {
      .type = MEMBER_CONST,
      .name = "y",
      .value = Simple_1_0_y_member_const,
    },
    {
      .type = MEMBER_CONST,
      .name = "z",
      .value = Simple_1_0_z_member_const,
    },
    {
      .type = MEMBER_ACCESSOR,
      .name = "name",
      .accessor = Simple_1_0_name_member_accessor,
    },
    {
      .type = MEMBER_ACCESSOR,
      .name = "version",
      .accessor = Simple_1_0_version_member_accessor,
    },
    {
      .type = MEMBER_ACCESSOR,
      .name = "args",
      .accessor = Simple_1_0_args_member_accessor,
    },
  };

  // callbacks
  static const struct FeatureCallbacks Simple_1_0_callbacks {
    Simple_1_0_onRegister,
    Simple_1_0_onCreate,
    Simple_1_0_onRequired,
    Simple_1_0_onDetached,
    Simple_1_0_onDestroy,
    Simple_1_0_onUnregister
  };

  static const FeatureDescription Simple_1_0_desc = {
    1,
    "Simple_1_0",
    "Simple_1_0",
    1,
    &Simple_1_0_callbacks,
    countof(Simple_1_0_members),
    Simple_1_0_members,
  };

QAPPFEATURE_INIT(Simple_1_0)
{
    return mgr->registerFeature(features, &Simple_1_0_desc);
}