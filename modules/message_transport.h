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

#include <list>
#include <map>

#include "app/BroadcastReceiver.h"
#include "app/UvLoop.h"

#include "channel.h"
#include "message_transport/BnMessageTransport.h"
#include "message_transport/BnReply.h"
#include "message_transport/IMessageTransport.h"

using android::sp;
using android::binder::Status;
using os::app::BroadcastReceiver;
using os::app::Intent;
using os::app::UvTimer;

#define REPLY_TIMEOUT_MS 10000

namespace message_transport {
enum SESSION_CLOSE_FLAG {
    SESSION_REASON_CLOSE = 0,
    SESSION_REASON_CLOSE_PEER = 1,
    SESSION_REASON_PEER_SHUTDOWN = 2,
};

class ClientConnection;
class MessageTransportServer;
class MessageReply;

// MsgTask and TaskBoard is used to manage message Reply tasks for message mode
class MsgTask {
public:
    MsgTask(sp<MessageReply> reply, ReplyId id)
        : reply_(reply)
        , id_(id)
        , is_done_(false)
        , timer_(nullptr)
    {
    }

    void startTimer(uv_loop_t* loop, uint32_t msTimeout);

    void stopTimer();

    sp<MessageReply> getIReply() const
    {
        return reply_;
    }

    int getReplyId() const
    {
        return id_;
    }

    void doing(const std::string& message);

    bool isDone() const
    {
        return is_done_;
    }

private:
    sp<MessageReply> reply_;
    ReplyId id_;
    bool is_done_;
    UvTimer* timer_;
};

class TaskBoard {
public:
    TaskBoard();
    void attachLoop(uv_loop_t* loop);
    void clearTimer();
    void commitTask(const std::shared_ptr<MsgTask>& task);
    void executeTask(int reply_id, const std::string& message);

private:
    std::list<std::shared_ptr<MsgTask>> task_list_;
    uv_loop_t* loop_;
};

////////////////////////////////////// for client
// session client mode reply
class SessionMessageReply : public BnReply {
public:
    SessionMessageReply(SessionId id)
        : id_(id)
        , client_channel_cb_(nullptr)
    {
    }
    Status onReply(const ::std::string& reply) override;
    Status onSessionClose() override;
    void setClientChannelCallback(ClientChannelCallback* cb)
    {
        client_channel_cb_ = cb;
    }
    void setClientConnection(ClientConnection* connect)
    {
        client_connection_ = connect;
    }

private:
    SessionId id_;
    ClientChannelCallback* client_channel_cb_;
    ClientConnection* client_connection_;
};

// message client mode reply
class MessageReply : public BnReply {
public:
    MessageReply(int32_t id)
        : pid_(id)
        , client_channel_cb_(nullptr)
        , client_connection_(nullptr)
    {
    }
    Status onReply(const ::std::string& reply) override;
    // no use
    Status onSessionClose() override { return Status::ok(); }

    void setClientChannelCallback(ClientChannelCallback* cb)
    {
        client_channel_cb_ = cb;
    }
    void setClientConnection(ClientConnection* connect)
    {
        client_connection_ = connect;
    }
    void onTimeout() const;
    void onReplyToClient(std::string message) const;

private:
    int32_t pid_; // promise id
    ClientChannelCallback* client_channel_cb_;
    ClientConnection* client_connection_;
};

class NotifyBroadcastReceiver : public BroadcastReceiver {
public:
    void onReceive(const Intent& intent) override;
    void setBroadcastChannelCallback(BroadcastChannelCallback* cb)
    {
        broadcast_cb_ = cb;
    }

private:
    BroadcastChannelCallback* broadcast_cb_;
};

class ClientConnection : public ClientChannel, public BroadcastChannel {
public:
    ClientConnection()
        : channel_data_handle_(nullptr)
    {
    }
    virtual ~ClientConnection();

    void attachLoop(uv_loop_t* loop) override;
    void clearUvTimer() override;

    // override ClientChannel session mode interface
    int createSession(const std::string& target) override;
    void sessionSend(SessionId id, const std::string& msg) override;
    void sessionClose(SessionId id) override;
    bool haveSessionId(SessionId id) override;

    // override ClientChannel message mode interface
    int sendMessage(const std::string& target, const std::string& msg,
        int32_t pid) override;

    // override BroadcastChannel
    void sendBroadcast(const std::string& action,
        const std::string& data) override;
    void registerReceiver(const std::string& action) override;
    void unregisterReceiver(const std::string& action) override;

    // for sessionReply
    void eraseSessionReply(SessionId id);
    void eraseSessionClient(SessionId id);
    // for MessageReply
    const TaskBoard& getTaskBoard() const;
    void setUserData(ChannelDataHandle channel_data_handle) override;

private:
    std::map<SessionId, sp<message_transport::IMessageTransport>>
        session_client_map_;
    std::map<SessionId, sp<SessionMessageReply>> session_reply_map_;
    std::map<std::string, sp<NotifyBroadcastReceiver>> broadcast_reply_;
    TaskBoard task_board_;
    ChannelDataHandle channel_data_handle_;
};

////////////////////////////////////// for server
class ServerHelper {
public:
    ServerHelper()
        : register_flag_(false)
        , transport_server_(nullptr)
    {
    }
    ~ServerHelper() { }

    // register binder server
    void registerServer(const std::string& name);
    sp<MessageTransportServer> getMessageTransportServer();

private:
    bool register_flag_;
    sp<MessageTransportServer> transport_server_;
};

class MessageTransportServer : public BnMessageTransport,
                               public MessageServerChannel,
                               public SessionServerChannel {
public:
    MessageTransportServer() { }
    virtual ~MessageTransportServer() { }

    // override BnMessageTransport
    Status
    sendMessage(const ::std::string& message,
        const ::android::sp<::message_transport::IReply>& reply) override;
    Status sendSessionMessage(
        const ::std::string& message,
        const ::android::sp<::message_transport::IReply>& reply) override;

    // override MessageServerChannel
    void serverReply(int reply_id, const std::string& message) override;

    // override SessionServerChannel
    void sessionSend(SessionId id, const std::string& message) override;
    void sessionClose(SessionId id) override;
    bool haveSessionId(SessionId id) override;

private:
    std::map<ReplyId, sp<::message_transport::IReply>> reply_map_;
};
} // namespace message_transport