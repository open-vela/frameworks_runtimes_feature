// Copyright 2023 Xiaomi, Inc. All rights reserved.

#ifndef JSON_AST_GEN_MODULE_MESSAGECHANNEL_H_
#define JSON_AST_GEN_MODULE_MESSAGECHANNEL_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <assert.h>
#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdarg>

// FeatureCallbacks to be implemented
void MessageChannel_onRegister(const char* feature_name);
void MessageChannel_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void MessageChannel_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void MessageChannel_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
void MessageChannel_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
void MessageChannel_onUnregister(const char* feature_name);

// Struct defines

// Function wrappers to be implemented
void MessageChannel_wrap_sendMessage(FeatureInstanceHandle feature, AppendData data,
                                     FtPromiseId pid, FtString target, FtString body);
void MessageChannel_wrap_reply(FeatureInstanceHandle feature, AppendData data, FtInt reply_id,
                               FtString reply);
void MessageChannel_wrap_setReceiveRequestCallback(FeatureInstanceHandle feature, AppendData data,
                                                   FtString service_name, FtCallbackId cb);
void MessageChannel_wrap_notifyMessage(FeatureInstanceHandle feature, AppendData data,
                                       FtString target, FtString body);
void MessageChannel_wrap_setTopicListener(FeatureInstanceHandle feature, AppendData data,
                                          FtString topic, FtCallbackId cb);
void MessageChannel_wrap_unsetTopicListener(FeatureInstanceHandle feature, AppendData data,
                                            FtString topic);
void MessageChannel_wrap_createSession(FeatureInstanceHandle feature, AppendData data,
                                       FtPromiseId pid, FtString target);
void MessageChannel_wrap_sessionSend(FeatureInstanceHandle feature, AppendData data,
                                     FtInt session_id, FtString message);
void MessageChannel_wrap_sessionClose(FeatureInstanceHandle feature, AppendData data,
                                      FtInt session_id);
void MessageChannel_wrap_acceptSession(FeatureInstanceHandle feature, AppendData data,
                                       FtInt session_id);
void MessageChannel_wrap_sessionOnData(FeatureInstanceHandle feature, AppendData data,
                                       FtInt session_id, FtCallbackId cb);
void MessageChannel_wrap_sessionOnClose(FeatureInstanceHandle feature, AppendData data,
                                        FtInt session_id, FtCallbackId cb);
void MessageChannel_wrap_sessionOnReceive(FeatureInstanceHandle feature, AppendData data,
                                          FtString service_name, FtCallbackId cb);
void MessageChannel_wrap_print(FeatureInstanceHandle feature, AppendData data, FtString message);

// Property getters and setters to be implemented

// interface vtable functions to be implemented

// Array malloc functions

#endif // JSON_AST_GEN_MODULE_MESSAGECHANNEL_H_
