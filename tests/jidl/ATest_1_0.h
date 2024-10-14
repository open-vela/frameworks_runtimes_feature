// Copyright 2023 Xiaomi, Inc. All rights reserved.

#ifndef JSON_AST_GEN_MODULE_ATEST_H_
#define JSON_AST_GEN_MODULE_ATEST_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FeatureCallbacks to be implemented
void ATest_onRegister(const char* feature_name);
void ATest_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void ATest_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void ATest_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void ATest_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void ATest_onUnregister(const char* feature_name);

// Struct defines
typedef struct _Person {
    FtString _name;
    FtString _gender;
    FtInt _age;
} ATest_Person;

ATest_Person* mallocPerson();

// Function wrappers to be implemented
FtString ATest_wrap_test1(FeatureInstanceHandle feature, AppendData data, FtString a, FtInt b);
void ATest_wrap_test2(FeatureInstanceHandle feature, AppendData data, FtInt a, FtCallbackId cb);
void ATest_wrap_test3(FeatureInstanceHandle feature, AppendData data, FtString a, FtCallbackId cb);
void ATest_wrap_test4(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a);
void ATest_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params);
void ATest_wrap_test5(FeatureInstanceHandle feature, AppendData data, FtArray& values);
FtArray* ATest_wrap_test6(FeatureInstanceHandle feature, AppendData data, FtInt a);
void ATest_wrap_test7(FeatureInstanceHandle feature, AppendData data, FtInt a, ATest_Person* b);
ATest_Person* ATest_wrap_test8(FeatureInstanceHandle feature, AppendData data, FtInt a);

// Property getters and setters to be implemented
FtInt ATest_get_idx(void* feature, AppendData data);
void ATest_set_idx(void* feature, AppendData data, FtInt idx);

// interface vtable functions to be implemented

// Array malloc functions
FtArray* ATest_malloc_int_array();
FtArray* ATest_malloc_string_array();

#endif // JSON_AST_GEN_MODULE_ATEST_H_
