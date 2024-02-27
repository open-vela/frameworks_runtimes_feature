// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "mockatest.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

/****** for JIDL callback 'test_body' ******/
static const FeatureType mockatest_test_body_parameters[] = {
    FT_PARAM_END
};

static const CallbackType mockatest_test_body_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = mockatest_test_body_parameters,
    .return_type = FT_VOID
};

/****** for JIDL function 'test' ******/
static const FeatureType mockatest_test_parameters[] = {
    FT_STRING,
    FT_STRING,
    FT_MK_COMPLEX(&mockatest_test_body_callback_type),
    FT_PARAM_END
};

static const MemberMethod mockatest_test_member_method = {
    .func = { .callback = FFI_FN(mockatest_wrap_test) },
    .parameters = mockatest_test_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL function 'expect_true' ******/
static OptionalType mockatest_expect_true_param_message_info_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = ""
};

static const FeatureType mockatest_expect_true_parameters[] = {
    FT_BOOLEAN,
    FT_MK_OPTIONAL(&mockatest_expect_true_param_message_info_opt_type),
    FT_PARAM_END
};

static const MemberMethod mockatest_expect_true_member_method = {
    .func = { .callback = FFI_FN(mockatest_wrap_expect_true) },
    .parameters = mockatest_expect_true_parameters,
    .return_type = FT_VOID,
};

/****** for JIDL function 'runAllOnce' ******/
static const FeatureType mockatest_runAllOnce_parameters[] = {
    FT_PARAM_END
};

static const MemberMethod mockatest_runAllOnce_member_method = {
    .func = { .callback = FFI_FN(mockatest_wrap_runAllOnce) },
    .parameters = mockatest_runAllOnce_parameters,
    .return_type = FT_VOID,
};

// members
static const Member mockatest_members[] = {
    {
        .type = MEMBER_METHOD,
        .name = "test",
        .method = &mockatest_test_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "expect_true",
        .method = &mockatest_expect_true_member_method,
    },
    {
        .type = MEMBER_METHOD,
        .name = "runAllOnce",
        .method = &mockatest_runAllOnce_member_method,
    },
};

// callbacks
static const struct FeatureCallbacks mockatest_callbacks {
    mockatest_onRegister,
        mockatest_onCreate,
        mockatest_onRequired,
        mockatest_onDetached,
        mockatest_onDestroy,
        mockatest_onUnregister
};

static const FeatureDescription mockatest_desc = {
    .version = 1,
    .name = "mockatest",
    .description = "mockatest",
    { .dynamic = false },
    .native_callbacks = &mockatest_callbacks,
    .member_count = countof(mockatest_members),
    .members = mockatest_members,
};

QAPPFEATURE_INIT(mockatest)
{
    return FeatureRegisterFeature(handle, &mockatest_desc);
}
