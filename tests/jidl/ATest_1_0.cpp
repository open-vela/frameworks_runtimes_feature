// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "ATest_1_0.h"
#include "ajs_features_init.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

/****** for JIDL function 'test1' ******/
static const FeatureType ATest_1_0_test1_parameters[] = {
    FT_STRING,
    FT_INT,
    FT_PARAM_END};

static const MemberMethod ATest_1_0_test1_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_test1),
    .parameters = ATest_1_0_test1_parameters,
    .return_type = FT_STRING,
};

/****** for JIDL callback 'cb1' ******/
static const FeatureType ATest_1_0_cb1_parameters[] = {
    FT_INT,
    FT_INT,
    FT_PARAM_END};

static const CallbackType ATest_1_0_cb1_callback_type{
    .header = {.type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId)},
    .parameters = ATest_1_0_cb1_parameters,
    .return_type = FT_VOID};

/****** for JIDL function 'test2' ******/
static const FeatureType ATest_1_0_test2_parameters[] = {
    FT_INT,
    FT_MK_COMPLEX(&ATest_1_0_cb1_callback_type),
    FT_PARAM_END};

static const MemberMethod ATest_1_0_test2_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_test2),
    .parameters = ATest_1_0_test2_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL callback 'cb2' ******/
static const FeatureType ATest_1_0_cb2_parameters[] = {
    FT_STRING,
    FT_PARAM_END};

static const CallbackType ATest_1_0_cb2_callback_type{
    .header = {.type = COMPLEX_CALLBACK, .size = sizeof(FeatureCallbackId)},
    .parameters = ATest_1_0_cb2_parameters,
    .return_type = FT_VOID};

/****** for JIDL function 'test3' ******/
static const FeatureType ATest_1_0_test3_parameters[] = {
    FT_STRING,
    FT_MK_COMPLEX(&ATest_1_0_cb2_callback_type),
    FT_PARAM_END};

static const MemberMethod ATest_1_0_test3_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_test3),
    .parameters = ATest_1_0_test3_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL function 'test4' ******/
static const FeatureType ATest_1_0_test4_parameters[] = {
    FT_INT,
    FT_PARAM_END};

static const PromiseType ATest_1_0_promise_FT_INT_FT_INT_type = {
    .header = {.type = COMPLEX_PROMISE, .size = sizeof(FeaturePromiseHandle)},
    .resolveTypes = {FT_INT, FT_INT}};

static const MemberMethod ATest_1_0_test4_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_test4),
    .parameters = ATest_1_0_test4_parameters,
    .return_type = FT_MK_COMPLEX_REF(&ATest_1_0_promise_FT_INT_FT_INT_type),
};

/****** for JIDL function 'print' ******/
static const FeatureType ATest_1_0_print_parameters[] = {
    FT_PARAM_REST_END,
};

static const MemberMethod ATest_1_0_print_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_print),
    .parameters = ATest_1_0_print_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL property 'idx' ******/
static const MemberAccessor ATest_1_0_idx_member_accessor = {
    .getter = FFI_FN(ATest_1_0_get_idx),
    .setter = FFI_FN(ATest_1_0_set_idx),
    .type = FT_INT,
};

/****** for JIDL function 'test5' ******/
static const ArrayType ATest_1_0_int_array = {
    .header = {.type = COMPLEX_ARRAY, .size = sizeof(FTArray)},
    .element_type = FT_INT};

static const FeatureType ATest_1_0_test5_parameters[] = {
    FT_MK_COMPLEX_REF(&ATest_1_0_int_array),
    FT_PARAM_END};

static const MemberMethod ATest_1_0_test5_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_test5),
    .parameters = ATest_1_0_test5_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL function 'test6' ******/
static const FeatureType ATest_1_0_test6_parameters[] = {
    FT_INT,
    FT_PARAM_END};

static const ArrayType ATest_1_0_string_array = {
    .header = {.type = COMPLEX_ARRAY, .size = sizeof(FTArray)},
    .element_type = FT_STRING};

FTArray *ATest_1_0_malloc_string_array()
{
  return (FTArray *)FTMalloc(
      sizeof(FTArray), FT_MK_COMPLEX(&ATest_1_0_string_array));
}
static const MemberMethod ATest_1_0_test6_member_method = {
    .callback = FFI_FN(ATest_1_0_wrap_test6),
    .parameters = ATest_1_0_test6_parameters,
    .return_type = FT_MK_COMPLEX_REF(&ATest_1_0_string_array),
};

// members
static const Member ATest_1_0_members[] = {
    {
        .type = MEMBER_METHOD,
        .name = "test1",
        .method = ATest_1_0_test1_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test2",
        .method = ATest_1_0_test2_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test3",
        .method = ATest_1_0_test3_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test4",
        .method = ATest_1_0_test4_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "print",
        .method = ATest_1_0_print_member_method,
    },
    {
        .type = MEMBER_ACCESSOR,
        .name = "idx",
        .accessor = ATest_1_0_idx_member_accessor,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test5",
        .method = ATest_1_0_test5_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test6",
        .method = ATest_1_0_test6_member_method,
    }};

// callbacks
static const struct FeatureCallbacks ATest_1_0_callbacks
{
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