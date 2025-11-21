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

#include "message_channel_impl.h"

#include "feature.h"
#include "feature_context_qjs.h"
#include "feature_log.h"

#include "message_channel.h"

#define MessageChannelTag "[jidl_feature] messageChannel"
#define GET_MESSAGE_CHANNEL(ft_instance) \
    (MessageChannel*)FeatureGetObjectData(ft_instance)

typedef struct CallData {
    CallData(FeatureInstanceHandle handle,
        const std::string& action,
        const std::string& message,
        const std::vector<FtCallbackId>& vector_id)
        : handle_(handle)
        , action_(action)
        , message_(message)
        , vector_id_(vector_id)
    {
    }

    FeatureInstanceHandle handle_;
    std::string action_;
    std::string message_;
    std::vector<FtCallbackId> vector_id_;
} CallData;

//////////////////// class MessageChannel
MessageChannel::MessageChannel()
    : ft_instance_(nullptr)
    , message_server_channel_(nullptr)
    , session_server_channel_(nullptr)
    , message_server_recv_cb_(-1)
    , session_server_recv_cb_(-1)
{
    ClientConnection* client_connect = new ClientConnection();
    client_channel_ = client_connect;
    broadcast_channel_ = client_connect;
    server_help_ = new ServerHelper();
    if (broadcast_channel_) {
        broadcast_channel_->setBroadcastCallback(this);
    }

    if (client_channel_) {
        client_channel_->setClientChannelCallback(this);
    }
}

MessageChannel::~MessageChannel()
{
    if (!action_cb_map_.empty()) {
        for (auto& item : action_cb_map_) {
            // 退出时通知ams清理bpbinder
            broadcast_channel_->unregisterReceiver(item.first);
            for (auto& id : item.second) {
                FeatureRemoveCallback(ft_instance_, id);
            }
        }
    }

    // client_channel_ and broadcast_channel_ is the same object.
    if (client_channel_ != nullptr && broadcast_channel_ != nullptr) {
        client_channel_->clearUvTimer();
        delete client_channel_;
        client_channel_ = nullptr;
        broadcast_channel_ = nullptr;
    }

    if (server_help_) {
        delete server_help_;
        server_help_ = nullptr;
    }

    if (message_server_channel_ && session_server_channel_) {
        message_server_channel_->clearMessageServerChannelCallback();
        session_server_channel_->clearSessionServerChannelCallback();
        message_server_channel_ = nullptr;
        session_server_channel_ = nullptr;
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

void MessageChannel::setFeatureInstanceHandle(FeatureInstanceHandle ft_instance)
{
    ft_instance_ = ft_instance;
    if (broadcast_channel_) {
        broadcast_channel_->setUserData(ft_instance);
    }
}

void MessageChannel::serverOnMessage(ReplyId reply_id, const std::string& message)
{
    if (message_server_recv_cb_ != -1) {
        if (ft_instance_ != nullptr) {
            bool ret = FeatureInvokeCallback(ft_instance_, message_server_recv_cb_,
                reply_id, message.c_str());
            if (!ret) {
                FEATURE_LOG_ERROR("server onmessage invoke failed !");
            }
        } else {
            auto pair = service_msg_map_.second;
            ServiceMsgCb cb = pair.first;
            cb((void*)this, reply_id, message.c_str(), pair.second);
        }
    }
}

void MessageChannel::sessionOnMessage(SessionId id,
    const std::string& message)
{
    if (session_server_recv_cb_ != -1) {
        bool ret = FeatureInvokeCallback(ft_instance_, session_server_recv_cb_, id,
            message.c_str());
        if (!ret) {
            FEATURE_LOG_ERROR("server onaccept invoke failed !");
        }
    }
}

void MessageChannel::clientOnSessionMessage(SessionId id,
    const std::string& message)
{
    if (session_ondata_cb_map_.count(id) > 0) {
        bool ret = FeatureInvokeCallback(ft_instance_, session_ondata_cb_map_[id],
            message.c_str());
        if (!ret) {
            FEATURE_LOG_ERROR("client onopen invoke failed !");
            return;
        }
    }
}

void MessageChannel::clientOnSessionCloseByself(SessionId id, int flag)
{
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

void MessageChannel::clientOnSessionCloseBypeer(SessionId id, int flag)
{
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

void MessageChannel::clientOnMessage(int32_t id, const std::string& message)
{
    if (ft_instance_ != nullptr) {
        FeaturePromiseResolve(ft_instance_, id, message.c_str());
    } else {
        auto pair = request_map_[id];
        RequestCb cb = pair.first;
        cb(message.c_str(), pair.second);
        request_map_.erase(id);
    }
}

void MessageChannel::clientOnTimeOut(int32_t id)
{
    FEATURE_LOG_WARN("client sendMessage timeout!");
    if (ft_instance_ != nullptr) {
        FeaturePromiseReject(ft_instance_, id, 202, "reply timeout");
    } else {
        auto pair = request_map_[id];
        RequestCb cb = pair.first;
        cb("reply timeout", pair.second);
        request_map_.erase(id);
    }
}

void MessageChannel::onReceive(const std::string& target,
    const std::string& action,
    const std::string& data)
{
    FEATURE_LOG_DEBUG("%s() target:%s, action:%s, data:%s ", __FUNCTION__,
        target.c_str(), action.c_str(), data.c_str());

    auto iter = action_cb_map_.find(action);
    if (iter == action_cb_map_.end()) {
        FEATURE_LOG_ERROR("ERROR: no found action callback: %s", action.c_str());
        return;
    }

    std::vector<FtCallbackId> vec = iter->second;
    if (ft_instance_ != nullptr) {
#ifdef CONFIG_QUICKAPP_ACTIVITY_ASYNC
        if (isFeatureLoopValid()) {
            CallData* call_data = new CallData(ft_instance_, iter->first, data, vec);
            FeaturePost(
                ft_instance_, [](int mode, void* callback_data) {
                    CallData* data_ptr = (CallData*)callback_data;
                    for (auto& id : data_ptr->vector_id_) {
                        bool ret = FeatureInvokeCallback(data_ptr->handle_, id, data_ptr->action_.c_str(), data_ptr->message_.c_str());
                        if (!ret) {
                            FEATURE_LOG_ERROR("broadcast recv invoke failed !");
                            break;
                        }
                    }
                    delete data_ptr;
                },
                call_data);
        } else { // systemui是没有为FeatureManager设置loop的，且systemui并不会创建js线程而出现跨线程调用feature的callback，所以直接调用FeatureInvokeCallback
            for (auto& id : vec) {
                bool ret = FeatureInvokeCallback(ft_instance_, id,
                    (iter->first).c_str(), data.c_str());
                if (!ret) {
                    FEATURE_LOG_ERROR("broadcast recv invoke failed !");
                    return;
                }
            }
        }
#else
        for (auto& id : vec) {
            bool ret = FeatureInvokeCallback(ft_instance_, id,
                (iter->first).c_str(), data.c_str());
            if (!ret) {
                FEATURE_LOG_ERROR("broadcast recv invoke failed !");
                return;
            }
        }
#endif
    } else {
        for (auto& id : vec) {
            auto pair = subscribe_map_[id];
            SubscribeCb cb = pair.first;
            cb((iter->first).c_str(), data.c_str(), pair.second);
        }
    }
}

void MessageChannel::sendBroadcast(const std::string& action,
    const std::string& body)
{
    if (broadcast_channel_) {
        broadcast_channel_->sendBroadcast(action, body);
    }
}

void MessageChannel::registerReceiver(const std::string& action,
    FtCallbackId action_cb)
{
    if (broadcast_channel_ && action_cb_map_.find(action) == action_cb_map_.end()) {
        broadcast_channel_->registerReceiver(action);
    }
    action_cb_map_[action].push_back(action_cb);
}

void MessageChannel::unregisterReceiver(const std::string& action)
{
    if (broadcast_channel_ && action_cb_map_.find(action) != action_cb_map_.end()) {
        for (auto& id : action_cb_map_[action]) {
            FeatureRemoveCallback(ft_instance_, id);
        }
        action_cb_map_.erase(action);
        broadcast_channel_->unregisterReceiver(action);
    }
}

void MessageChannel::unregisterReceiverCb(const std::string& action, FtCallbackId action_cb)
{
    if (action_cb_map_.find(action) != action_cb_map_.end()) {
        std::vector<FtCallbackId>& vec = action_cb_map_[action];
        auto it = std::find(vec.begin(), vec.end(), action_cb);
        if (it != vec.end()) {
            FeatureRemoveCallback(ft_instance_, *it);
            vec.erase(it);
        }
        if (broadcast_channel_ && vec.empty()) {
            action_cb_map_.erase(action);
            broadcast_channel_->unregisterReceiver(action);
        }
    }
}

// 目前只支持js服务与native
// service一对一。扩展为多对一，需将createSession添加jsservice_name参数
int MessageChannel::createSession(const std::string& target)
{
    if (client_channel_) {
        return client_channel_->createSession(target);
    }
    return -1;
}

void MessageChannel::sessionClose(SessionId session_id)
{
    if (client_channel_ && client_channel_->haveSessionId(session_id)) {
        client_channel_->sessionClose(session_id);
    } else if (session_server_channel_ && session_server_channel_->haveSessionId(session_id)) {
        session_server_channel_->sessionClose(session_id);
    }
}

void MessageChannel::sessionSend(SessionId session_id, const std::string& msg)
{
    if (client_channel_ && client_channel_->haveSessionId(session_id)) {
        client_channel_->sessionSend(session_id, msg);
    } else if (session_server_channel_ && session_server_channel_->haveSessionId(session_id)) {
        session_server_channel_->sessionSend(session_id, msg);
    }
}

void MessageChannel::sessionOnData(SessionId session_id, FtCallbackId cb)
{
    session_ondata_cb_map_[session_id] = cb;
}

void MessageChannel::sessionOnClose(SessionId session_id, FtCallbackId cb)
{
    session_onclose_cb_map_[session_id] = cb;
}

void MessageChannel::sessionOnReceive(FtCallbackId cb)
{
    session_server_recv_cb_ = cb;
}

void MessageChannel::setReceiveRequestCallback(FtCallbackId cb)
{
    message_server_recv_cb_ = cb;
}

int MessageChannel::sendMessage(const std::string& target,
    const std::string& msg, FtPromiseId pid)
{
    if (client_channel_) {
        return client_channel_->sendMessage(target, msg, pid);
    }
    return -1;
}

void MessageChannel::reply(ReplyId reply_id, const std::string& msg)
{
    if (message_server_channel_) {
        message_server_channel_->serverReply(reply_id, msg);
    }
}

void MessageChannel::registerServer(const std::string& name)
{
    if (server_help_) {
        server_help_->registerServer(name);
        message_server_channel_ = server_help_->getMessageTransportServer().get();
        session_server_channel_ = server_help_->getMessageTransportServer().get();
    }

    if (message_server_channel_ && session_server_channel_) {
        message_server_channel_->setMessageServerChannelCallback(this);
        session_server_channel_->setSessionServerChannelCallback(this);
    }
}

void MessageChannel::attachLoop(uv_loop_t* loop)
{
    if (!loop) {
        FEATURE_LOG_ERROR("loop is null !");
        return;
    }
    client_channel_->attachLoop(loop);
}

int MessageChannel::sendMessageForC(const std::string& target, const std::string& msg, RequestCb cb, UserDataHandle user_data)
{
    int32_t id = (intptr_t)cb;
    request_map_.insert(std::make_pair(id, std::make_pair(cb, user_data)));
    return sendMessage(target, msg, id);
}

void MessageChannel::setReceiveRequestCallbackForC(ServiceMsgCb cb, UserDataHandle user_data)
{
    int32_t id = (intptr_t)cb;
    service_msg_map_ = std::make_pair(id, std::make_pair(cb, user_data));
    setReceiveRequestCallback(id);
}

void MessageChannel::replyForC(ReplyId reply_id, const std::string& msg)
{
    reply(reply_id, msg);
}

void MessageChannel::sendBroadcastForC(const std::string& action, const std::string& body)
{
    sendBroadcast(action, body);
}

void MessageChannel::registerReceiverForC(const std::string& action, SubscribeCb cb, UserDataHandle user_data)
{
    int32_t id = (intptr_t)cb;
    subscribe_map_.insert(std::make_pair(id, std::make_pair(cb, user_data)));
    registerReceiver(action, id);
}

void MessageChannel::unregisterReceiverForC(const std::string& action)
{
    unregisterReceiver(action);
    for (auto& id : action_cb_map_[action]) {
        subscribe_map_.erase(id);
    }
}

bool MessageChannel::isFeatureLoopValid()
{
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(ft_instance_);
    uv_loop_t* loop = FeatureGetUVLoop(manager);
    if (!loop) {
        FEATURE_LOG_ERROR("loop is null !");
        return false;
    }
    return true;
}

///////////////////////// jidl feature implement
static void initMessageChannel(FeatureInstanceHandle ft_instance)
{
    MessageChannel* message_channel = new MessageChannel();
    FeatureSetObjectData(ft_instance, message_channel);
    message_channel->setFeatureInstanceHandle(ft_instance);
    // attach loop
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(ft_instance);
    uv_loop_t* loop = FeatureGetUVLoop(manager);
    message_channel->attachLoop(loop);
}

static void freeMessageChannel(FeatureInstanceHandle ft_instance)
{
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(ft_instance);
    delete message_channel;
}

void system_messageChannel_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

void system_messageChannel_onCreate(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

void system_messageChannel_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    initMessageChannel(handle);
}

void system_messageChannel_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
    freeMessageChannel(handle);
}

void system_messageChannel_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

void system_messageChannel_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", MessageChannelTag, __FUNCTION__);
}

// Message mode
// for client.
void system_messageChannel_wrap_sendMessage(FeatureInstanceHandle feature,
    AppendData append_data,
    FtPromiseId pid, FtString target,
    FtString body)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    int res = message_channel->sendMessage(target, body, pid);
    if (res == -1) {
        FeaturePromiseReject(feature, pid, 202, "native sendMessage Failed");
    }
}

// for server
void system_messageChannel_wrap_reply(FeatureInstanceHandle feature,
    AppendData append_data, FtInt reply_id,
    FtString reply)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->reply(reply_id, reply);
}

void system_messageChannel_wrap_setReceiveRequestCallback(
    FeatureInstanceHandle feature, AppendData append_data,
    FtString service_name, FtCallbackId cb)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->setReceiveRequestCallback(cb);
    message_channel->registerServer(service_name);
}

// Session mode
// jidl wrapper for client
void system_messageChannel_wrap_createSession(FeatureInstanceHandle feature,
    AppendData append_data,
    FtPromiseId pid,
    FtString target)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    int res = message_channel->createSession(target);
    if (res != -1) {
        FeaturePromiseResolve(feature, pid, res);
    } else {
        // TODO:error处理
        FeaturePromiseReject(feature, pid, 202, "native createSession Failed");
    }
}

void system_messageChannel_wrap_sessionSend(FeatureInstanceHandle feature,
    AppendData append_data,
    FtInt session_id,
    FtString message)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionSend(session_id, message);
}

void system_messageChannel_wrap_acceptSession(FeatureInstanceHandle feature,
    AppendData append_data,
    FtInt session_id)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    // do nothing
}

void system_messageChannel_wrap_sessionClose(FeatureInstanceHandle feature,
    AppendData append_data,
    FtInt session_id)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionClose(session_id);
}

void system_messageChannel_wrap_sessionOnData(FeatureInstanceHandle feature,
    AppendData append_data,
    FtInt session_id,
    FtCallbackId cb)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnData(session_id, cb);
}

void system_messageChannel_wrap_sessionOnClose(FeatureInstanceHandle feature,
    AppendData append_data,
    FtInt session_id,
    FtCallbackId cb)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnClose(session_id, cb);
}

void system_messageChannel_wrap_sessionOnReceive(FeatureInstanceHandle feature,
    AppendData append_data,
    FtString service_name,
    FtCallbackId cb)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sessionOnReceive(cb);
    message_channel->registerServer(service_name);
}

// Notify mode
void system_messageChannel_wrap_notifyMessage(FeatureInstanceHandle feature,
    AppendData append_data,
    FtString target, FtString body)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->sendBroadcast(target, body);
}

void system_messageChannel_wrap_setTopicListener(FeatureInstanceHandle feature,
    AppendData append_data,
    FtString topic,
    FtCallbackId cb)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->registerReceiver(topic, cb);
}

void system_messageChannel_wrap_unsetTopicListener(
    FeatureInstanceHandle feature, AppendData append_data, FtString topic)
{
    FEATURE_LOG_DEBUG("%s::%s()", MessageChannelTag, __FUNCTION__);
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->unregisterReceiver(topic);
}

void system_messageChannel_wrap_unsetTopicListenerCb(FeatureInstanceHandle feature, AppendData append_data, FtString topic, FtCallbackId cb)
{
    MessageChannel* message_channel = GET_MESSAGE_CHANNEL(feature);
    message_channel->unregisterReceiverCb(topic, cb);
}

void system_messageChannel_wrap_print(FeatureInstanceHandle feature,
    AppendData append_data,
    FtString message)
{
    FEATURE_LOG_INFO("############js print log:%s", message);
}

extern "C" {
MessageChannelHandle message_channel_init()
{
    return static_cast<MessageChannelHandle>(new MessageChannel());
}

void message_channel_uninit(MessageChannelHandle handle)
{
    MessageChannel* channel = static_cast<MessageChannel*>(handle);
    delete channel;
}

void message_channel_send_async_request(MessageChannelHandle handle, const char* name, const char* data, RequestCb cb, void* user_data)
{
    MessageChannel* channel = static_cast<MessageChannel*>(handle);
    channel->sendMessageForC(name, data, cb, user_data);
}

void message_channel_send_async_response(MessageChannelHandle handle, ReplyId id, const char* data)
{
    MessageChannel* channel = static_cast<MessageChannel*>(handle);
    channel->replyForC(id, data);
}

void message_channel_add_async_service(MessageChannelHandle handle, const char* name, ServiceMsgCb cb, void* user_data)
{
    MessageChannel* channel = static_cast<MessageChannel*>(handle);
    channel->setReceiveRequestCallbackForC(cb, user_data);
    channel->registerServer(name);
}

void message_channel_publish(MessageChannelHandle handle, const char* topic_name, const char* data)
{
    MessageChannel* channel = static_cast<MessageChannel*>(handle);
    channel->sendBroadcastForC(topic_name, data);
}
void message_channel_subscribe(MessageChannelHandle handle, const char* topic_name, SubscribeCb cb, void* user_data)
{
    MessageChannel* channel = static_cast<MessageChannel*>(handle);
    channel->registerReceiverForC(topic_name, cb, user_data);
}

void message_channel_unsubscribe(MessageChannelHandle handle, const char* topic_name)
{
    MessageChannel* channel = static_cast<MessageChannel*>(handle);
    channel->unregisterReceiverForC(topic_name);
}
}
