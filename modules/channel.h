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

#include <string>
#include <uv.h>

typedef int32_t ReplyId;
typedef int32_t SessionId;
typedef void* ChannelDataHandle;

class MessageServerChannelCallback {
public:
    virtual void serverOnMessage(ReplyId reply_id, const std::string& message) = 0;
};

class SessionServerChannelCallback {
public:
    virtual void sessionOnMessage(SessionId id, const std::string& message) = 0;
};

class BroadcastChannelCallback {
public:
    virtual void onReceive(const std::string& target, const std::string& action,
        const std::string& data)
        = 0;
};

class ClientChannelCallback {
public:
    // session reply callback
    virtual void clientOnSessionMessage(SessionId id, const std::string& message) = 0;
    virtual void clientOnSessionCloseByself(SessionId id, int flag) = 0;
    virtual void clientOnSessionCloseBypeer(SessionId id, int flag) = 0;

    // message reply callback
    virtual void clientOnMessage(int32_t id, const std::string& message) = 0;
    virtual void clientOnTimeOut(int32_t id) = 0;
};

class MessageServerChannel {
public:
    MessageServerChannel()
        : message_server_channel_cb_(nullptr)
    {
    }
    virtual ~MessageServerChannel() { }
    virtual void serverReply(int reply_id, const std::string& message) = 0;
    void setMessageServerChannelCallback(MessageServerChannelCallback* cb)
    {
        message_server_channel_cb_ = cb;
    }
    void clearMessageServerChannelCallback()
    {
        message_server_channel_cb_ = nullptr;
    }

protected:
    MessageServerChannelCallback* message_server_channel_cb_;
};

class SessionServerChannel {
public:
    SessionServerChannel()
        : session_server_channel_cb_(nullptr)
    {
    }
    virtual ~SessionServerChannel() { }
    virtual void sessionSend(SessionId id, const std::string& message) = 0;
    virtual void sessionClose(SessionId id) = 0;
    virtual bool haveSessionId(SessionId id) = 0;
    void setSessionServerChannelCallback(SessionServerChannelCallback* cb)
    {
        session_server_channel_cb_ = cb;
    }
    void clearSessionServerChannelCallback()
    {
        session_server_channel_cb_ = nullptr;
    }

protected:
    SessionServerChannelCallback* session_server_channel_cb_;
};

class BroadcastChannel {
public:
    BroadcastChannel()
        : broadcast_cb_(nullptr)
    {
    }
    virtual ~BroadcastChannel() { }
    virtual void sendBroadcast(const std::string& action, const std::string& data) = 0;
    virtual void registerReceiver(const std::string& action) = 0;
    virtual void unregisterReceiver(const std::string& action) = 0;
    void setBroadcastCallback(BroadcastChannelCallback* cb)
    {
        broadcast_cb_ = cb;
    }
    virtual void setUserData(ChannelDataHandle channel_data_handle) = 0;

protected:
    BroadcastChannelCallback* broadcast_cb_;
};

class ClientChannel {
public:
    ClientChannel()
        : client_channel_cb_(nullptr)
    {
    }
    virtual ~ClientChannel() { }

    virtual void attachLoop(uv_loop_t* loop) = 0;
    virtual void clearUvTimer() = 0;

    // session
    virtual int createSession(const std::string& target) = 0;
    virtual void sessionSend(SessionId id, const std::string& msg) = 0;
    virtual void sessionClose(SessionId id) = 0;
    virtual bool haveSessionId(SessionId id) = 0;

    // message
    virtual int sendMessage(const std::string& target, const std::string& msg, int32_t pid) = 0;

    void setClientChannelCallback(ClientChannelCallback* cb)
    {
        client_channel_cb_ = cb;
    }

protected:
    ClientChannelCallback* client_channel_cb_;
};