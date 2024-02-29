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

#include "message_transport.h"

#include <app/ActivityManager.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/Log.h>
#include <utils/String8.h>

using android::defaultServiceManager;
using android::IBinder;
using android::interface_cast;
using android::IServiceManager;
using android::String16;
using os::app::ActivityManager;

namespace message_transport {
Status SessionMessageReply::onReply(const ::std::string& reply)
{
    client_channel_cb_->clientOnSessionMessage(id_, reply);
    return Status::ok();
}

Status SessionMessageReply::onSessionClose()
{
    client_channel_cb_->clientOnSessionCloseBypeer(id_,
        SESSION_REASON_CLOSE_PEER);
    client_connection_->eraseSessionReply(id_);
    client_connection_->eraseSessionClient(id_);
    return Status::ok();
}

Status MessageReply::onReply(const ::std::string& reply)
{
    auto task_board = client_connection_->getTaskBoard();
    task_board.executeTask((intptr_t)this, reply);
    return Status::ok();
}

void MessageReply::onTimeout() const
{
    client_channel_cb_->clientOnTimeOut(pid_);
}

void MessageReply::onReplyToClient(std::string message) const
{
    client_channel_cb_->clientOnMessage(pid_, message);
}

void NotifyBroadcastReceiver::onReceive(const Intent& intent)
{
    if (broadcast_cb_ != nullptr) {
        broadcast_cb_->onReceive(intent.mTarget, intent.mAction, intent.mData);
    }
}

void ClientConnection::attachLoop(uv_loop_t* loop)
{
    task_board_.attachLoop(loop);
}

void ClientConnection::clearUvTimer()
{
    task_board_.clearTimer();
}

int ClientConnection::createSession(const std::string& target)
{
    sp<IMessageTransport> service;
    if (android::getService<IMessageTransport>(android::String16(target.c_str()),
            &service)
        != android::NO_ERROR) {
        ALOGE("ServiceManager can't find the service:%s", target.c_str());
        return -1;
    }

    ALOGI("imessagetransport service is %p", service.get());
    int32_t id = (intptr_t)service.get();
    session_client_map_.insert(std::make_pair(id, service));
    return id;
}

void ClientConnection::sessionSend(SessionId id, const std::string& msg)
{
    if (session_client_map_.find(id) != session_client_map_.end()) {
        sp<IMessageTransport> service = session_client_map_[id];

        // TODO:目前认为binder线程与js线程在同一个线程, 后续需要做兼容
        // 判断服务是否alive；如果alive，再发送消息
        if (!android::IInterface::asBinder(service)->isBinderAlive()) {
            ALOGE("imessagetransport service is not alive:%" PRIi32 "", id);
            return;
        }
        sp<SessionMessageReply> reply;
        if (session_reply_map_.find(id) == session_reply_map_.end()) {
            reply = new SessionMessageReply(id);
            reply->setClientChannelCallback(client_channel_cb_);
            reply->setClientConnection(this);
            session_reply_map_.insert(std::make_pair(id, reply));
        } else {
            reply = session_reply_map_[id];
        }
        Status status = service->sendSessionMessage(msg, reply);
        if (!status.isOk()) {
            ALOGE("sendSessionMessage error: %s. SessionId(%" PRIi32 ")",
                status.toString8().c_str(), id);
        }
    }
}

void ClientConnection::sessionClose(SessionId id)
{
    if (session_client_map_.find(id) != session_client_map_.end()) {
        session_client_map_.erase(id);
        client_channel_cb_->clientOnSessionCloseByself(id, SESSION_REASON_CLOSE);
    }
}

bool ClientConnection::haveSessionId(SessionId id)
{
    return session_client_map_.find(id) != session_client_map_.end();
}

int ClientConnection::sendMessage(const std::string& target,
    const std::string& msg, int32_t pid_)
{
    sp<IMessageTransport> service;
    if (android::getService<IMessageTransport>(android::String16(target.c_str()),
            &service)
        != android::NO_ERROR) {
        ALOGE("ServiceManager can't find the service:%s", target.c_str());
        return -1;
    }
    sp<MessageReply> reply = new MessageReply(pid_);
    reply->setClientChannelCallback(client_channel_cb_);
    reply->setClientConnection(this);
    task_board_.commitTask(std::make_shared<MsgTask>(reply, (intptr_t)reply.get()));
    Status status = service->sendMessage(msg, reply);
    if (!status.isOk()) {
        ALOGE("sendMessage error: %s. target:%s", status.toString8().c_str(),
            target.c_str());
        return -1;
    }
    return 0;
}

void ClientConnection::registerReceiver(const std::string& action)
{
    sp<NotifyBroadcastReceiver> receiver(new NotifyBroadcastReceiver());
    receiver->setBroadcastChannelCallback(broadcast_cb_);
    ActivityManager am;
    am.registerReceiver(action, receiver);
    broadcast_reply_.insert(std::make_pair(action, receiver));
}

void ClientConnection::unregisterReceiver(const std::string& action)
{
    if (broadcast_reply_.find(action) != broadcast_reply_.end()) {
        ActivityManager am;
        am.unregisterReceiver(broadcast_reply_[action]);
    }
}

void ClientConnection::sendBroadcast(const std::string& action,
    const std::string& data)
{
    ActivityManager am;
    Intent intent;
    intent.setAction(action);
    intent.setData(data);
    am.sendBroadcast(intent);
}

void ClientConnection::eraseSessionReply(SessionId id)
{
    if (session_reply_map_.find(id) != session_reply_map_.end()) {
        session_reply_map_.erase(id);
    }
}

void ClientConnection::eraseSessionClient(SessionId id)
{
    if (session_client_map_.find(id) != session_client_map_.end()) {
        session_client_map_.erase(id);
    }
}

const TaskBoard& ClientConnection::getTaskBoard() const
{
    return task_board_;
}

void MsgTask::startTimer(uv_loop_t* loop, uint32_t msTimeout)
{
    timer_ = new UvTimer();
    timer_->init(loop, [this](void*) {
        if (!is_done_) {
            is_done_ = true;
            reply_->onTimeout();
        }
    });
    timer_->start(msTimeout);
}

void MsgTask::stopTimer()
{
    if (timer_) {
        timer_->stop();
        delete timer_;
        timer_ = nullptr;
    }
}

void MsgTask::doing(const std::string& message)
{
    is_done_ = true;
    reply_->onReplyToClient(message);
}

TaskBoard::TaskBoard() { }

void TaskBoard::attachLoop(uv_loop_t* loop)
{
    loop_ = loop;
}

void TaskBoard::clearTimer()
{
    for (auto iter = task_list_.begin(); iter != task_list_.end();) {
        (*iter)->stopTimer();
        iter = task_list_.erase(iter);
    }
}

void TaskBoard::commitTask(const std::shared_ptr<MsgTask>& task)
{
    task->startTimer(loop_, REPLY_TIMEOUT_MS);
    task_list_.emplace_back(task);
}

void TaskBoard::executeTask(int reply_id, const std::string& message)
{
    for (auto iter = task_list_.begin(); iter != task_list_.end();) {
        if ((*iter)->isDone()) {
            /** This situation is handled by timeout. need delete it*/
            (*iter)->stopTimer();
            auto tmp = iter;
            ++iter;
            task_list_.erase(tmp);
            continue;
        }
        if ((*iter)->getReplyId() == reply_id) {
            (*iter)->doing(message);
            // execute finish,
            (*iter)->stopTimer();
            // remove it from list
            iter = task_list_.erase(iter);
            continue;
        }
        ++iter;
    }
}

void ServerHelper::registerServer(const std::string& name)
{
    if (!register_flag_) {
        sp<IServiceManager> sm(defaultServiceManager());
        transport_server_ = new MessageTransportServer();
        // 注册服务
        ALOGI("add %s to service manager", name.c_str());
        sm->addService(String16(name.c_str()), transport_server_);
        register_flag_ = true;
    }
}

sp<MessageTransportServer> ServerHelper::getMessageTransportServer()
{
    return transport_server_;
}

Status MessageTransportServer::sendMessage(
    const ::std::string& message,
    const ::android::sp<::message_transport::IReply>& reply)
{
    int32_t id = (intptr_t)reply.get();
    reply_map_.insert(std::make_pair(id, reply));
    if (message_server_channel_cb_) {
        message_server_channel_cb_->serverOnMessage(id, message);
    } else {
        ALOGW("js server has been stoped, message channel is closed.");
    }
    return Status::ok();
}

Status MessageTransportServer::sendSessionMessage(
    const ::std::string& message,
    const ::android::sp<::message_transport::IReply>& reply)
{
    int32_t id = (intptr_t)reply.get();
    reply_map_.insert(std::make_pair(id, reply));
    if (session_server_channel_cb_) {
        session_server_channel_cb_->sessionOnMessage(id, message);
    } else {
        ALOGW("js server has been stoped, message channel is closed.");
    }
    return Status::ok();
}

void MessageTransportServer::serverReply(int reply_id,
    const std::string& message)
{
    if (reply_map_.find(reply_id) != reply_map_.end()) {
        reply_map_[reply_id]->onReply(message);
        reply_map_.erase(reply_id);
    }
}

void MessageTransportServer::sessionSend(SessionId reply_id,
    const std::string& message)
{
    if (reply_map_.find(reply_id) != reply_map_.end()) {
        Status status = reply_map_[reply_id]->onReply(message);
        if (!status.isOk()) {
            ALOGE("sessionSend error: %s. message:%s", status.toString8().c_str(),
                message.c_str());
        }
    }
}

void MessageTransportServer::sessionClose(SessionId id)
{
    if (reply_map_.find(id) != reply_map_.end()) {
        reply_map_[id]->onSessionClose();
        reply_map_.erase(id);
    }
}

bool MessageTransportServer::haveSessionId(SessionId id)
{
    if (reply_map_.find(id) != reply_map_.end()) {
        return true;
    }
    return false;
}

} // namespace message_transport
