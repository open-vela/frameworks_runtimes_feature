// Copyright 2023 Xiaomi, Inc. All rights reserved.

#ifndef JSON_AST_GEN_MODULE_JUMPAPP_H_
#define JSON_AST_GEN_MODULE_JUMPAPP_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <assert.h>
#include <cstdarg>
#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FeatureCallbacks to be implemented
void jumpApp_onRegister(const char* feature_name);
void jumpApp_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void jumpApp_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void jumpApp_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void jumpApp_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void jumpApp_onUnregister(const char* feature_name);

// Struct defines

// Function wrappers to be implemented
void jumpApp_wrap_jumpApp(FeatureInstanceHandle feature, AppendData data, FtString uri, FtString arg);

// Property getters and setters to be implemented

// interface vtable functions to be implemented

// Array malloc functions

#endif // JSON_AST_GEN_MODULE_JUMPAPP_H_
