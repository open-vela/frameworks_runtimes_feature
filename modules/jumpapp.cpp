// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "jumpapp.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

/****** for JIDL function 'jumpApp' ******/
static const FeatureType jumpApp_jumpApp_parameters[] = {
    FT_STRING,
    FT_STRING,
    FT_PARAM_END
};

static const MemberMethod jumpApp_jumpApp_member_method = {
    .func = { .callback = FFI_FN(jumpApp_wrap_jumpApp) },
    .parameters = jumpApp_jumpApp_parameters,
    .return_type = FT_VOID,
};

// members
static const Member jumpApp_members[] = {
    {
        .type = MEMBER_METHOD,
        .name = "jumpApp",
        .method = jumpApp_jumpApp_member_method,
    },
};

// callbacks
static const struct FeatureCallbacks jumpApp_callbacks {
    jumpApp_onRegister,
        jumpApp_onCreate,
        jumpApp_onRequired,
        jumpApp_onDetached,
        jumpApp_onDestroy,
        jumpApp_onUnregister
};

static const FeatureDescription jumpApp_desc = {
    .version = 1,
    .name = "jumpApp",
    .description = "jumpApp",
    { .dynamic = false },
    .native_callbacks = &jumpApp_callbacks,
    .member_count = countof(jumpApp_members),
    .members = jumpApp_members,
};

QAPPFEATURE_INIT(jumpApp)
{
    return mgr->registerFeature(features, &jumpApp_desc);
}