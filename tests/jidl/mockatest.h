// Copyright 2023 Xiaomi, Inc. All rights reserved.

#ifndef JSON_AST_GEN_MODULE_MOCKATEST_H_
#define JSON_AST_GEN_MODULE_MOCKATEST_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FeatureCallbacks to be implemented
void mockatest_onRegister(const char* feature_name);
void mockatest_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void mockatest_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void mockatest_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void mockatest_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void mockatest_onUnregister(const char* feature_name);

// Struct defines

// Function wrappers to be implemented
void mockatest_wrap_test(FeatureInstanceHandle feature, AppendData data, FtString test_suit_name, FtString test_case_name, FtCallbackId body);
void mockatest_wrap_expect_true(FeatureInstanceHandle feature, AppendData data, FtBool result, FtString message_info);
void mockatest_wrap_runAllOnce(FeatureInstanceHandle feature, AppendData data);

// Property getters and setters to be implemented

// interface vtable functions to be implemented

// Array malloc functions

#endif // JSON_AST_GEN_MODULE_MOCKATEST_H_
