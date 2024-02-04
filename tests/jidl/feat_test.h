// Copyright 2023 Xiaomi, Inc. All rights reserved.

#ifndef JSON_AST_GEN_MODULE_FEAT_TEST_H_
#define JSON_AST_GEN_MODULE_FEAT_TEST_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <assert.h>
#include <cstdarg>
#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FeatureCallbacks to be implemented
void feat_test_onRegister(const char* feature_name);
void feat_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void feat_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void feat_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void feat_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void feat_test_onUnregister(const char* feature_name);

// Struct defines

// Function wrappers to be implemented
FtInt feat_test_wrap_testsuite(FeatureInstanceHandle feature, AppendData append_data, FtString test_suit_name, FtString test_case_name, FtCallbackId body, FtBool is_async);
void feat_test_wrap_done(FeatureInstanceHandle feature, AppendData append_data, FtInt async_id, FtInt err, FtString err_message);
void feat_test_wrap_expect_true(FeatureInstanceHandle feature, AppendData append_data, FtBool result, FtString message_info);
void feat_test_wrap_run_all_tests(FeatureInstanceHandle feature, AppendData append_data);
void feat_test_wrap_print(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params);

// Interface constructors

// interface vtable functions to be implemented

// Property getters and setters to be implemented

// Array malloc functions

#endif // JSON_AST_GEN_MODULE_FEAT_TEST_H_
