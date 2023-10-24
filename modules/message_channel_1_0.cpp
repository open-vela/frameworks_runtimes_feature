// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "message_channel_1_0.h"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

/****** for JIDL function 'sendMessage' ******/
static const FeatureType MessageChannel_sendMessage_parameters[] = {FT_STRING, FT_STRING,
                                                                    FT_PARAM_END};

static const PromiseType MessageChannel_promise_FT_STRING_FT_STRING_type =
        {.header = {.type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId)},
         .resolveTypes = {FT_STRING, FT_STRING}};

static const MemberMethod MessageChannel_sendMessage_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_sendMessage)},
        .parameters = MessageChannel_sendMessage_parameters,
        .return_type = FT_MK_COMPLEX_REF(&MessageChannel_promise_FT_STRING_FT_STRING_type),
};

/****** for JIDL function 'reply' ******/
static const FeatureType MessageChannel_reply_parameters[] = {FT_INT, FT_STRING, FT_PARAM_END};

static const MemberMethod MessageChannel_reply_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_reply)},
        .parameters = MessageChannel_reply_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL callback 'cb_receive_request' ******/
static const FeatureType MessageChannel_cb_receive_request_parameters[] = {FT_INT, FT_STRING,
                                                                           FT_PARAM_END};

static const CallbackType MessageChannel_cb_receive_request_callback_type{
        .header = {.type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId)},
        .parameters = MessageChannel_cb_receive_request_parameters,
        .return_type = FT_VOID};

/****** for JIDL function 'setReceiveRequestCallback' ******/
static const FeatureType MessageChannel_setReceiveRequestCallback_parameters[] =
        {FT_STRING, FT_MK_COMPLEX(&MessageChannel_cb_receive_request_callback_type), FT_PARAM_END};

static const MemberMethod MessageChannel_setReceiveRequestCallback_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_setReceiveRequestCallback)},
        .parameters = MessageChannel_setReceiveRequestCallback_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL function 'notifyMessage' ******/
static const FeatureType MessageChannel_notifyMessage_parameters[] = {FT_STRING, FT_STRING,
                                                                      FT_PARAM_END};

static const MemberMethod MessageChannel_notifyMessage_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_notifyMessage)},
        .parameters = MessageChannel_notifyMessage_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL callback 'onTopicMessage' ******/
static const FeatureType MessageChannel_onTopicMessage_parameters[] = {FT_STRING, FT_STRING,
                                                                       FT_PARAM_END};

static const CallbackType MessageChannel_onTopicMessage_callback_type{
        .header = {.type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId)},
        .parameters = MessageChannel_onTopicMessage_parameters,
        .return_type = FT_VOID};

/****** for JIDL function 'setTopicListener' ******/
static const FeatureType MessageChannel_setTopicListener_parameters[] =
        {FT_STRING, FT_MK_COMPLEX(&MessageChannel_onTopicMessage_callback_type), FT_PARAM_END};

static const MemberMethod MessageChannel_setTopicListener_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_setTopicListener)},
        .parameters = MessageChannel_setTopicListener_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL function 'unsetTopicListener' ******/
static const FeatureType MessageChannel_unsetTopicListener_parameters[] = {FT_STRING, FT_PARAM_END};

static const MemberMethod MessageChannel_unsetTopicListener_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_unsetTopicListener)},
        .parameters = MessageChannel_unsetTopicListener_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL callback 'cb_ondata' ******/
static const FeatureType MessageChannel_cb_ondata_parameters[] = {FT_STRING, FT_PARAM_END};

static const CallbackType
        MessageChannel_cb_ondata_callback_type{.header = {.type = COMPLEX_CALLBACK,
                                                          .size = sizeof(FtCallbackId)},
                                               .parameters = MessageChannel_cb_ondata_parameters,
                                               .return_type = FT_VOID};

/****** for JIDL callback 'cb_onclose' ******/
static const FeatureType MessageChannel_cb_onclose_parameters[] = {FT_INT, FT_PARAM_END};

static const CallbackType
        MessageChannel_cb_onclose_callback_type{.header = {.type = COMPLEX_CALLBACK,
                                                           .size = sizeof(FtCallbackId)},
                                                .parameters = MessageChannel_cb_onclose_parameters,
                                                .return_type = FT_VOID};

/****** for JIDL callback 'cb_onerror' ******/
static const FeatureType MessageChannel_cb_onerror_parameters[] = {FT_INT, FT_STRING, FT_PARAM_END};

static const CallbackType
        MessageChannel_cb_onerror_callback_type{.header = {.type = COMPLEX_CALLBACK,
                                                           .size = sizeof(FtCallbackId)},
                                                .parameters = MessageChannel_cb_onerror_parameters,
                                                .return_type = FT_VOID};

/****** for JIDL callback 'cb_onreceive' ******/
static const FeatureType MessageChannel_cb_onreceive_parameters[] = {FT_INT, FT_STRING,
                                                                     FT_PARAM_END};

static const CallbackType
        MessageChannel_cb_onreceive_callback_type{.header = {.type = COMPLEX_CALLBACK,
                                                             .size = sizeof(FtCallbackId)},
                                                  .parameters =
                                                          MessageChannel_cb_onreceive_parameters,
                                                  .return_type = FT_VOID};

/****** for JIDL function 'createSession' ******/
static const FeatureType MessageChannel_createSession_parameters[] = {FT_STRING, FT_PARAM_END};

static const PromiseType MessageChannel_promise_FT_INT_FT_STRING_type =
        {.header = {.type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId)},
         .resolveTypes = {FT_INT, FT_STRING}};

static const MemberMethod MessageChannel_createSession_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_createSession)},
        .parameters = MessageChannel_createSession_parameters,
        .return_type = FT_MK_COMPLEX_REF(&MessageChannel_promise_FT_INT_FT_STRING_type),
};

/****** for JIDL function 'sessionSend' ******/
static const FeatureType MessageChannel_sessionSend_parameters[] = {FT_INT, FT_STRING,
                                                                    FT_PARAM_END};

static const MemberMethod MessageChannel_sessionSend_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_sessionSend)},
        .parameters = MessageChannel_sessionSend_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL function 'sessionClose' ******/
static const FeatureType MessageChannel_sessionClose_parameters[] = {FT_INT, FT_PARAM_END};

static const MemberMethod MessageChannel_sessionClose_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_sessionClose)},
        .parameters = MessageChannel_sessionClose_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL function 'acceptSession' ******/
static const FeatureType MessageChannel_acceptSession_parameters[] = {FT_INT, FT_PARAM_END};

static const MemberMethod MessageChannel_acceptSession_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_acceptSession)},
        .parameters = MessageChannel_acceptSession_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL function 'sessionOnData' ******/
static const FeatureType MessageChannel_sessionOnData_parameters[] =
        {FT_INT, FT_MK_COMPLEX(&MessageChannel_cb_ondata_callback_type), FT_PARAM_END};

static const MemberMethod MessageChannel_sessionOnData_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_sessionOnData)},
        .parameters = MessageChannel_sessionOnData_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL const 'SESSION_REASON_CLOSE' ******/
const FtInt MessageChannel_g_const_SESSION_REASON_CLOSE = 0;
const FtInt MessageChannel_init_const_SESSION_REASON_CLOSE(FeatureInstanceHandle feature,
                                                           AppendData data) {
    return MessageChannel_g_const_SESSION_REASON_CLOSE;
};

static const MemberConst MessageChannel_SESSION_REASON_CLOSE_member_const =
        {.type = FT_INT,
         //.func = { .callback = FFI_FN(MessageChannel_init_const_SESSION_REASON_CLOSE) },
         .func = {.callback = nullptr},
         .data = {.i32 = MessageChannel_g_const_SESSION_REASON_CLOSE}};

/****** for JIDL const 'SESSION_REASON_CLOSE_PEER' ******/
const FtInt MessageChannel_g_const_SESSION_REASON_CLOSE_PEER = 1;
const FtInt MessageChannel_init_const_SESSION_REASON_CLOSE_PEER(FeatureInstanceHandle feature,
                                                                AppendData data) {
    return MessageChannel_g_const_SESSION_REASON_CLOSE_PEER;
};

static const MemberConst MessageChannel_SESSION_REASON_CLOSE_PEER_member_const =
        {.type = FT_INT,
         //.func = { .callback = FFI_FN(MessageChannel_init_const_SESSION_REASON_CLOSE_PEER) },
         .func = {.callback = nullptr},
         .data = {.i32 = MessageChannel_g_const_SESSION_REASON_CLOSE_PEER}};

/****** for JIDL const 'SESSION_REASON_PEER_SHUTDOWN' ******/
const FtInt MessageChannel_g_const_SESSION_REASON_PEER_SHUTDOWN = 2;
const FtInt MessageChannel_init_const_SESSION_REASON_PEER_SHUTDOWN(FeatureInstanceHandle feature,
                                                                   AppendData data) {
    return MessageChannel_g_const_SESSION_REASON_PEER_SHUTDOWN;
};

static const MemberConst MessageChannel_SESSION_REASON_PEER_SHUTDOWN_member_const =
        {.type = FT_INT,
         //.func = { .callback = FFI_FN(MessageChannel_init_const_SESSION_REASON_PEER_SHUTDOWN) },
         .func = {.callback = nullptr},
         .data = {.i32 = MessageChannel_g_const_SESSION_REASON_PEER_SHUTDOWN}};

/****** for JIDL function 'sessionOnClose' ******/
static const FeatureType MessageChannel_sessionOnClose_parameters[] =
        {FT_INT, FT_MK_COMPLEX(&MessageChannel_cb_onclose_callback_type), FT_PARAM_END};

static const MemberMethod MessageChannel_sessionOnClose_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_sessionOnClose)},
        .parameters = MessageChannel_sessionOnClose_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL function 'sessionOnReceive' ******/
static const FeatureType MessageChannel_sessionOnReceive_parameters[] =
        {FT_STRING, FT_MK_COMPLEX(&MessageChannel_cb_onreceive_callback_type), FT_PARAM_END};

static const MemberMethod MessageChannel_sessionOnReceive_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_sessionOnReceive)},
        .parameters = MessageChannel_sessionOnReceive_parameters,
        .return_type = FT_VOID,
};

/****** for JIDL function 'print' ******/
static const FeatureType MessageChannel_print_parameters[] = {FT_STRING, FT_PARAM_END};

static const MemberMethod MessageChannel_print_member_method = {
        .func = {.callback = FFI_FN(MessageChannel_wrap_print)},
        .parameters = MessageChannel_print_parameters,
        .return_type = FT_VOID,
};

// members
static const Member MessageChannel_members[] = {
        {
                .type = MEMBER_METHOD,
                .name = "sendMessage",
                .method = MessageChannel_sendMessage_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "reply",
                .method = MessageChannel_reply_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "setReceiveRequestCallback",
                .method = MessageChannel_setReceiveRequestCallback_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "notifyMessage",
                .method = MessageChannel_notifyMessage_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "setTopicListener",
                .method = MessageChannel_setTopicListener_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "unsetTopicListener",
                .method = MessageChannel_unsetTopicListener_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "createSession",
                .method = MessageChannel_createSession_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "sessionSend",
                .method = MessageChannel_sessionSend_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "sessionClose",
                .method = MessageChannel_sessionClose_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "acceptSession",
                .method = MessageChannel_acceptSession_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "sessionOnData",
                .method = MessageChannel_sessionOnData_member_method,
        },
        {
                .type = MEMBER_CONST,
                .name = "SESSION_REASON_CLOSE",
                .value = MessageChannel_SESSION_REASON_CLOSE_member_const,
        },
        {
                .type = MEMBER_CONST,
                .name = "SESSION_REASON_CLOSE_PEER",
                .value = MessageChannel_SESSION_REASON_CLOSE_PEER_member_const,
        },
        {
                .type = MEMBER_CONST,
                .name = "SESSION_REASON_PEER_SHUTDOWN",
                .value = MessageChannel_SESSION_REASON_PEER_SHUTDOWN_member_const,
        },
        {
                .type = MEMBER_METHOD,
                .name = "sessionOnClose",
                .method = MessageChannel_sessionOnClose_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "sessionOnReceive",
                .method = MessageChannel_sessionOnReceive_member_method,
        },
        {
                .type = MEMBER_METHOD,
                .name = "print",
                .method = MessageChannel_print_member_method,
        },
};

// callbacks
static const struct FeatureCallbacks MessageChannel_callbacks {
    MessageChannel_onRegister, MessageChannel_onCreate, MessageChannel_onRequired,
            MessageChannel_onDetached, MessageChannel_onDestroy, MessageChannel_onUnregister
};

static const FeatureDescription MessageChannel_desc = {
        .version = 1,
        .name = "MessageChannel",
        .description = "MessageChannel",
        {.dynamic = false},
        .native_callbacks = &MessageChannel_callbacks,
        .member_count = countof(MessageChannel_members),
        .members = MessageChannel_members,
};

QAPPFEATURE_INIT(MessageChannel) {
    return mgr->registerFeature(features, &MessageChannel_desc);
}