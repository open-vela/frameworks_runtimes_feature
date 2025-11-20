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

#pragma once

#include <map>

#include "channel.h"
#include "feature_exports.h"
#include "message_transport.h"
#include "modules/message_channel_api.h"

typedef void (*RequestCb)(const char* data, void* user_data);
typedef void (*ServiceMsgCb)(void* handle, int32_t id, const char* data, void* user_data);
typedef void (*SubscribeCb)(const char* name, const char* data, void* user_data);
typedef void* UserDataHandle;

using message_transport::ClientConnection;
using message_transport::MessageTransportServer;
using message_transport::ServerHelper;

class MessageChannel : public MessageServerChannelCallback,
                       public SessionServerChannelCallback,
                       public ClientChannelCallback,
                       public BroadcastChannelCallback {
public:
    MessageChannel();

    virtual ~MessageChannel();

    void setFeatureInstanceHandle(FeatureInstanceHandle ft_instance);

    // override MessageServerChannelCallback
    void serverOnMessage(ReplyId reply_id, const std::string& message) override;

    // override SessionServerChannelCallback
    void sessionOnMessage(SessionId id, const std::string& message) override;

    // override ClientChannelCallback
    void clientOnSessionMessage(SessionId id,
        const std::string& message) override;
    void clientOnSessionCloseByself(SessionId id, int flag) override;
    void clientOnSessionCloseBypeer(SessionId id, int flag) override;
    void clientOnMessage(int32_t id, const std::string& message) override;
    void clientOnTimeOut(int32_t id) override;

    // override BroadcastChannelCallback
    void onReceive(const std::string& target, const std::string& action,
        const std::string& data) override;

    // notify
    void sendBroadcast(const std::string& action, const std::string& body);
    void registerReceiver(const std::string& action, FtCallbackId action_cb);
    void unregisterReceiver(const std::string& action);
    void unregisterReceiverCb(const std::string& action, FtCallbackId action_cb);

    // session
    int createSession(const std::string& target);
    void sessionOnData(SessionId session_id, FtCallbackId cb);
    void sessionOnClose(SessionId session_id, FtCallbackId cb);
    void sessionOnReceive(FtCallbackId cb);
    void sessionSend(SessionId session_id, const std::string& msg);
    void sessionClose(SessionId session_id);

    // Message
    int sendMessage(const std::string& target, const std::string& msg,
        FtPromiseId pid);
    void setReceiveRequestCallback(FtCallbackId cb);
    void reply(ReplyId reply_id, const std::string& msg);

    // register server
    void registerServer(const std::string& name);

    // attach loop
    void attachLoop(uv_loop_t* loop);

    // for c api
    int sendMessageForC(const std::string& target, const std::string& msg,
        RequestCb cb, UserDataHandle user_data);
    void setReceiveRequestCallbackForC(ServiceMsgCb cb, UserDataHandle user_data);
    void replyForC(ReplyId reply_id, const std::string& msg);
    void sendBroadcastForC(const std::string& action, const std::string& body);
    void registerReceiverForC(const std::string& action, SubscribeCb cb, UserDataHandle user_data);
    void unregisterReceiverForC(const std::string& action);

    // 用于判断在初始化feature环境时,是否传入了loop
    bool isFeatureLoopValid();

private:
    FeatureInstanceHandle ft_instance_;
    BroadcastChannel* broadcast_channel_;
    MessageServerChannel* message_server_channel_;
    SessionServerChannel* session_server_channel_;
    ClientChannel* client_channel_;
    ServerHelper* server_help_;

    FtCallbackId message_server_recv_cb_;
    FtCallbackId session_server_recv_cb_;

    std::map<std::string, std::vector<FtCallbackId>> action_cb_map_;
    std::map<SessionId, FtCallbackId> session_ondata_cb_map_;
    std::map<SessionId, FtCallbackId> session_onclose_cb_map_;

    std::map<int32_t, std::pair<RequestCb, UserDataHandle>> request_map_;
    std::pair<int32_t, std::pair<ServiceMsgCb, UserDataHandle>> service_msg_map_;
    std::map<int32_t, std::pair<SubscribeCb, UserDataHandle>> subscribe_map_;
};