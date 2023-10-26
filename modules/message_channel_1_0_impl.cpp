/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "message_channel_1_0_impl.h"

#include "feature.h"
#include "feature_context_qjs.h"
#include "feature_log.h"

#include "message_channel_1_0.h"

#define MessageChannelTag "[jidl_feature] MessageChannel_1_0"
#define GET_MESSAGE_CHANNEL(ft_instance) (MessageChannel*)FeatureGetObjectData(ft_instance)

//////////////////// class MessageChannel
MessageChannel::MessageChannel(FeatureInstanceHandle ft_instance, ClientChannel* client_channel,
                               BroadcastChannel* broadcast_channel, ServerHelper* server_help)
      : ft_instance_(ft_instance),
        broadcast_channel_(broadcast_channel),
        client_channel_(client_channel),
        server_help_(server_help),
        message_server_recv_cb_(-1),
        session_server_recv_cb_(-1) {
    if (broadcast_channel_) {
        broadcast_channel_->setBroadcastCallback(this);
    }

    if (client_channel_) {
        client_channel_->setClientChannelCallback(this);
    }
}

MessageChannel::~MessageChannel() {
    // client_channel_ and broadcast_channel_ is the same object.
    if (client_channel_ != nullptr && broadcast_channel_ != nullptr) {
        delete client_channel_;
        client_channel_ = nullptr;
        broadcast_channel_ = nullptr;
    }

    // message_server_channel_ and session_server_channel_ is the same object.
    if (message_server_channel_ != nullptr && session_server_channel_ != nullptr) {
        delete message_server_channel_;
        message_server_channel_ = nullptr;
        session_server_channel_ = nullptr;
    }

    if (server_help_) {
        delete server_help_;
        server_help_ = nullptr;
    }

    if (message_server_recv_cb_ != -1) {
        FeatureRemoveCallback(ft_instance_, message_server_recv_cb_);
    }

    if (session_server_recv_cb_ != -1) {
        FeatureRemoveCallback(ft_instance_, session_server_recv_cb_);
    }

    if (!session_ondata_cb_map_.empty()) {
        for (auto& x : session_ondata_cb_map_) {
            FeatureRemoveCallback(ft_instance_, x.second);
        }
    }

    if (!session_onclose_cb_map_.empty()) {
        for (auto& x : session_onclose_cb_map_) {
            FeatureRemoveCallback(ft_instance_, x.second);
        }
    }
}

void MessageChannel::serverOnMessage(int reply_id, const std::string& message) {
    if (message_server_recv_cb_ != -1) {
        bool ret = FeatureInvokeCallback(ft_instance_, message_server_recv_cb_, reply_id,
                                         message.c_str());
        if (!ret) {
            FEATURE_LOG_ERROR("server onmessage invoke failed !");
        }
    }
}

void MessageChannel::sessionOnMessage(SessionId id, const std::string& message) {
    if (session_server_recv_cb_ != -1) {
        bool ret =
                FeatureInvokeCallback(ft_instance_, session_server_recv_cb_, id, message.c_str());
        if (!ret) {
            FEATURE_LOG_ERROR("server onaccept invoke failed !");
        }
    }
}

void MessageChannel::clientOnSessionMessage(SessionId id, const std::string& message) {
    if (session_ondata_cb_map_.count(id) > 0) {
        bool ret = FeatureInvokeCallback(ft_instance_, session_ondata_cb_map_[id], message.c_str());
        if (!ret) {
            FEATURE_LOG_ERROR("client onopen invoke failed !");
            return;
        }
    }
}

void MessageChannel::clientOnSessionCloseByself(SessionId id, int flag) {
    if (session_onclose_cb_map_.count(id) > 0) {
        bool ret = FeatureInvokeCallback(ft_instance_, session_onclose_cb_map_[id], flag);
        if (!ret) {
            FEATURE_LOG_ERROR("client onclose invoke failed !");
            return;
        }

        FeatureRemoveCallback(ft_instance_, session_onclose_cb_map_[id]);
        session_onclose_cb_map_.erase(id);
    }
}

void MessageChannel::clientOnSessionCloseBypeer(SessionId id, int flag) {
    if (session_onclose_cb_map_.count(id) > 0) {
        bool ret = FeatureInvokeCallback(ft_instance_, session_onclose_cb_map_[id], flag);
        if (!ret) {
            FEATURE_LOG_ERROR("client onclose invoke failed !");
            return;
        }

        FeatureRemoveCallback(ft_instance_, session_onclose_cb_map_[id]);
        session_onclose_cb_map_.erase(id);
    }
    // peer close时删除sessionOnData的callback
    if (session_ondata_cb_map_.find(id) != session_ondata_cb_map_.end()) {
        FeatureRemoveCallback(ft_instance_, session_ondata_cb_map_[id]);
        session_ondata_cb_map_.erase(id);
    }
}

void MessageChannel::clientOnMessage(int32_t id, const std::string& message) {
    FeaturePromiseResolve(ft_instance_, id, message.c_str());
}

void MessageChannel::onReceive(const std::string& target, const std::string& action,
                               const std::string& data) {
    FEATURE_LOG_INFO("%s() target:%s, action:%s, data:%s ", __FUNCTION__, target.c_str(),
                     action.c_str(), data.c_str());

    auto iter = action_cb_map_.find(action);
    if (iter == action_cb_map_.end()) {
        FEATURE_LOG_ERROR("ERROR: no found action callback: %s", action.c_str());
        return;
    }

    bool ret =
            FeatureInvokeCallback(ft_instance_, iter->second, (iter->first).c_str(), data.c_str());
    if (!ret) {
        FEATURE_LOG_ERROR("broadcast recv invoke failed !");
        return;
    }
}

void MessageChannel::sendBroadcast(const std::string& action, const std::string& body) {
    if (broadcast_channel_) {
        broadcast_channel_->sendBroadcast(action, body);
    }
}

void MessageChannel::registerReceiver(const std::string& action, FtCallbackId action_cb) {
    if (broadcast_channel_) {
        broadcast_channel_->registerReceiver(action);
        action_cb_map_[action] = action_cb;
    }
}

void MessageChannel::unregisterReceiver(const std::string& action) {
    if (broadcast_channel_ && action_cb_map_.find(action) != action_cb_map_.end()) {
        FeatureRemoveCallback(ft_instance_, action_cb_map_[action]);
        action_cb_map_.erase(action);
        broadcast_channel_->unregisterReceiver(action);
    }
}

// 目前只支持js服务与native service一对一。扩展为多对一，需将createSession添加jsservice_name参数
int MessageChannel::createSession(const std::string& target) {
    if (client_channel_) {
        return client_channel_->createSession(target);
    }
    return -1;
}

void MessageChannel::sessionClose(SessionId session_id) {
    if (client_channel_ && client_channel_->haveSessionId(session_id)) {
        client_channel_->sessionClose(session_id);
    } else if (session_server_channel_ && session_server_channel_->haveSessionId(session_id)) {
        session_server_channel_->sessionClose(session_id);
    }
}

void MessageChannel::sessionSend(SessionId session_id, const std::string& msg) {
    if (client_channel_ && client_channel_->haveSessionId(session_id)) {
        client_channel_->sessionSend(session_id, msg);
    } else if (session_server_channel_ && session_server_channel_->haveSessionId(session_id)) {
        session_server_channel_->sessionSend(session_id, msg);
    }
}

void MessageChannel::sessionOnData(SessionId session_id, FtCallbackId cb) {
    session_ondata_cb_map_[session_id] = cb;
}

void MessageChannel::sessionOnClose(SessionId session_id, FtCallbackId cb) {
    session_onclose_cb_map_[session_id] = cb;
}

void MessageChannel::sessionOnReceive(FtCallbackId cb) {
    session_server_recv_cb_ = cb;
}

void MessageChannel::setReceiveRequestCallback(FtCallbackId cb) {
    message_server_recv_cb_ = cb;
}

int MessageChannel::sendMessage(const std::string& target, const std::string& msg,
                                FtPromiseId pid) {
    if (client_channel_) {
        return client_channel_->sendMessage(target, msg, pid);
    }
    return -1;
}

void MessageChannel::reply(ReplyId reply_id, const std::string& msg) {
    if (message_server_channel_) {
        message_server_channel_->serverReply(reply_id, msg);
    }
}

void MessageChannel::registerServer(const std::string& name) {
    if (server_help_) {
        server_help_->registerServer(name);
    }
    message_server_channel_ = server_help_->getMessageTransportServer();
    session_server_channel_ = server_help_->getMessageTransportServer();
    if (message_server_channel_ && session_server_channel_) {
        message_server_channel_->setMessageServerChannelCallback(this);
        session_server_channel_->setSessionServerChannelCallback(this);
    }
}

///////////////////////// jidl feature implement
static void initMessageChannel(feature_context_ref ctx, FeatureInstanceHandle ft_instance) {
    ClientConnection* client_connect = new ClientConnection();
    ServerHelper* server_help = new ServerHelper();
    MessageChannel* message_channel =
            new MessageChannel(ft_instance, client_connect, client_connect, server_help);
    FeatureSetObjectData(ft_instance, message_channel);
}

static void freeMessageChannel(FeatureInstanceHandle ft_instance) {
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(ft_instance);
    delete message_channel;
}

void MessageChannel_onRegister(const char* feature_name) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

void MessageChannel_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

void MessageChannel_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    initMessageChannel(static_cast<feature_context_ref>(ctx), handle);
}

void MessageChannel_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    freeMessageChannel(handle);
}

void MessageChannel_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

void MessageChannel_onUnregister(const char* feature_name) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

// Message mode
// for client.
void MessageChannel_wrap_sendMessage(FeatureInstanceHandle feature, AppendData data,
                                     FtPromiseId pid, FtString target, FtString body) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    int res = message_channel->sendMessage(target, body, pid);
    if (res == -1) {
        FeaturePromiseReject(feature, pid, "native sendMessage Failed");
    }
}

// for server
void MessageChannel_wrap_reply(FeatureInstanceHandle feature, AppendData data, FtInt reply_id,
                               FtString reply) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->reply(reply_id, reply);
}

void MessageChannel_wrap_setReceiveRequestCallback(FeatureInstanceHandle feature, AppendData data,
                                                   FtString service_name, FtCallbackId cb) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->setReceiveRequestCallback(cb);
    message_channel->registerServer(service_name);
}

// Session mode
// jidl wrapper for client
void MessageChannel_wrap_createSession(FeatureInstanceHandle feature, AppendData data,
                                       FtPromiseId pid, FtString target) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    int res = message_channel->createSession(target);
    if (res != -1) {
        FeaturePromiseResolve(feature, pid, res);
    } else {
        // TODO:error处理
        FeaturePromiseReject(feature, pid, "native createSession Failed");
    }
}

void MessageChannel_wrap_sessionSend(FeatureInstanceHandle feature, AppendData data,
                                     FtInt session_id, FtString message) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionSend(session_id, message);
}

void MessageChannel_wrap_acceptSession(FeatureInstanceHandle feature, AppendData data,
                                       FtInt session_id) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    // do nothing
}

void MessageChannel_wrap_sessionClose(FeatureInstanceHandle feature, AppendData data,
                                      FtInt session_id) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionClose(session_id);
}

void MessageChannel_wrap_sessionOnOpen(FeatureInstanceHandle feature, AppendData data,
                                       FtInt session_id, FtCallbackId cb) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnData(session_id, cb);
}

void MessageChannel_wrap_sessionOnData(FeatureInstanceHandle feature, AppendData data,
                                       FtInt session_id, FtCallbackId cb) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnData(session_id, cb);
}

void MessageChannel_wrap_sessionOnClose(FeatureInstanceHandle feature, AppendData data,
                                        FtInt session_id, FtCallbackId cb) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnClose(session_id, cb);
}

void MessageChannel_wrap_sessionOnError(FeatureInstanceHandle feature, AppendData data,
                                        FtInt session_id, FtCallbackId cb) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnError(session_id, cb);
}

void MessageChannel_wrap_sessionOnReceive(FeatureInstanceHandle feature, AppendData data,
                                          FtString service_name, FtCallbackId cb) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnReceive(cb);
    message_channel->registerServer(service_name);
}

// Notify mode
void MessageChannel_wrap_notifyMessage(FeatureInstanceHandle feature, AppendData data,
                                       FtString target, FtString body) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sendBroadcast(target, body);
}

void MessageChannel_wrap_setTopicListener(FeatureInstanceHandle feature, AppendData data,
                                          FtString topic, FtCallbackId cb) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->registerReceiver(topic, cb);
}

void MessageChannel_wrap_unsetTopicListener(FeatureInstanceHandle feature, AppendData data,
                                            FtString topic) {
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->unregisterReceiver(topic);
}

void MessageChannel_wrap_print(FeatureInstanceHandle feature, AppendData data, FtString message) {
    FEATURE_LOG_INFO("############js print log:%s", message);
}
