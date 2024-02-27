// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "ATest_1_0.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

/****** for JIDL function 'test1' ******/
static const FeatureType ATest_test1_parameters[] = {
    FT_STRING,
    FT_INT,
    FT_PARAM_END
};

static const MemberMethod ATest_test1_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test1) },
    .parameters = ATest_test1_parameters,
    .return_type = FT_STRING,
};

/****** for JIDL callback 'cb1' ******/
static const FeatureType ATest_cb1_parameters[] = {
    FT_INT,
    FT_INT,
    FT_PARAM_END
};

static const CallbackType ATest_cb1_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = ATest_cb1_parameters,
    .return_type = FT_VOID
};

/****** for JIDL function 'test2' ******/
static const FeatureType ATest_test2_parameters[] = {
    FT_INT,
    FT_MK_COMPLEX(&ATest_cb1_callback_type),
    FT_PARAM_END
};

static const MemberMethod ATest_test2_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test2) },
    .parameters = ATest_test2_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL callback 'cb2' ******/
static const FeatureType ATest_cb2_parameters[] = {
    FT_STRING,
    FT_PARAM_END
};

static const CallbackType ATest_cb2_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = ATest_cb2_parameters,
    .return_type = FT_VOID
};

/****** for JIDL function 'test3' ******/
static const FeatureType ATest_test3_parameters[] = {
    FT_STRING,
    FT_MK_COMPLEX(&ATest_cb2_callback_type),
    FT_PARAM_END
};

static const MemberMethod ATest_test3_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test3) },
    .parameters = ATest_test3_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL function 'test4' ******/
static const FeatureType ATest_test4_parameters[] = {
    FT_INT,
    FT_PARAM_END
};

static const PromiseType ATest_promise_FT_INT_FT_INT_type = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_INT, FT_INT }
};

static const MemberMethod ATest_test4_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test4) },
    .parameters = ATest_test4_parameters,
    .return_type = FT_MK_COMPLEX(&ATest_promise_FT_INT_FT_INT_type),
};

/****** for JIDL function 'print' ******/
static const FeatureType ATest_print_parameters[] = {
    FT_PARAM_REST_END,
};

static const MemberMethod ATest_print_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_print) },
    .parameters = ATest_print_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL property 'idx' ******/
static const MemberAccessor ATest_idx_member_accessor = {
    .getter = { .callback = FFI_FN(ATest_get_idx) },
    .setter = { .callback = FFI_FN(ATest_set_idx) },
    .type = FT_INT,
};

/****** for JIDL function 'test5' ******/
static const ArrayType ATest_int_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_INT
};

FtArray* ATest_malloc_int_array()
{
    return (FtArray*)FeatureMalloc(
        sizeof(FtArray), FT_MK_COMPLEX(&ATest_int_array));
}

static const FeatureType ATest_test5_parameters[] = {
    FT_MK_COMPLEX(&ATest_int_array),
    FT_PARAM_END
};

static const MemberMethod ATest_test5_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test5) },
    .parameters = ATest_test5_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL function 'test6' ******/
static const FeatureType ATest_test6_parameters[] = {
    FT_INT,
    FT_PARAM_END
};

static const ArrayType ATest_string_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_STRING
};

FtArray* ATest_malloc_string_array()
{
    return (FtArray*)FeatureMalloc(
        sizeof(FtArray), FT_MK_COMPLEX(&ATest_string_array));
}

static const MemberMethod ATest_test6_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test6) },
    .parameters = ATest_test6_parameters,
    .return_type = FT_MK_COMPLEX(&ATest_string_array),
};

/****** for JIDL struct 'Person' ******/
static ObjectMember ATest_Person_struct_members[] = {
    { "name", FT_STRING, offsetof(ATest_Person, _name), sizeof(FtString) },
    { "gender", FT_STRING, offsetof(ATest_Person, _gender), sizeof(FtString) },
    { "age", FT_INT, offsetof(ATest_Person, _age), sizeof(FtInt) },
    { nullptr },
};

// complex defination
static const ObjectMapType ATest_Person_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(ATest_Person) },
    .members = ATest_Person_struct_members
};

ATest_Person* mallocPerson()
{
    return (ATest_Person*)FeatureMalloc(
        sizeof(ATest_Person), FT_MK_COMPLEX(&ATest_Person_struct_type));
}

/****** for JIDL function 'test7' ******/
static const FeatureType ATest_test7_parameters[] = {
    FT_INT,
    FT_MK_COMPLEX(&ATest_Person_struct_type),
    FT_PARAM_END
};

static const MemberMethod ATest_test7_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test7) },
    .parameters = ATest_test7_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL function 'test8' ******/
static const FeatureType ATest_test8_parameters[] = {
    FT_INT,
    FT_PARAM_END
};

static const MemberMethod ATest_test8_member_method = {
    .func = { .callback = FFI_FN(ATest_wrap_test8) },
    .parameters = ATest_test8_parameters,
    .return_type = FT_MK_COMPLEX(&ATest_Person_struct_type),
};

// members
static const Member ATest_members[] = {
    {
        .type = MEMBER_METHOD,
        .name = "test1",
        .method = &ATest_test1_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test2",
        .method = &ATest_test2_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test3",
        .method = &ATest_test3_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test4",
        .method = &ATest_test4_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "print",
        .method = &ATest_print_member_method,
    },
    {
        .type = MEMBER_ACCESSOR,
        .name = "idx",
        .accessor = &ATest_idx_member_accessor,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test5",
        .method = &ATest_test5_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test6",
        .method = &ATest_test6_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test7",
        .method = &ATest_test7_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "test8",
        .method = &ATest_test8_member_method,
    },
};

// callbacks
static const struct FeatureCallbacks ATest_callbacks {
    ATest_onRegister,
        ATest_onCreate,
        ATest_onRequired,
        ATest_onDetached,
        ATest_onDestroy,
        ATest_onUnregister
};

static const FeatureDescription ATest_desc = {
    .version = 1,
    .name = "ATest",
    .description = "ATest",
    { .dynamic = false },
    .native_callbacks = &ATest_callbacks,
    .member_count = countof(ATest_members),
    .members = ATest_members,
};

QAPPFEATURE_INIT(ATest)
{
    return FeatureRegisterFeature(handle, &ATest_desc);
}
