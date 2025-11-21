/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#include "prompt_server.h"

#define PROMPT_LOOP_PERIOD 100

namespace prompt {

// static task
void PromptServer::InitServerTask(void* user_data)
{
    auto prompt_server = static_cast<PromptServer*>(user_data);
    prompt_server->init();
}

void PromptServer::DestroyServerTask(void* user_data)
{
    auto prompt_server = static_cast<PromptServer*>(user_data);
    std::unique_ptr<PromptServer> del(prompt_server);
}

PromptServer::PromptServer()
    : timer_(nullptr)
{
}

PromptServer::~PromptServer()
{
    uninit();
}

void PromptServer::init()
{
    if (timer_) {
        return;
    }

    lv_timer_cb_t cb = [](struct _lv_timer_t* t) -> void {
        PromptServer* server = (PromptServer*)t->user_data;
        server->run(PROMPT_LOOP_PERIOD);
    };
    timer_ = lv_timer_create(cb, PROMPT_LOOP_PERIOD, this);
    if (!timer_) {
        printf("ERROR: PromptServer lv_timer_create failed\n");
        return;
    }
}

void PromptServer::uninit()
{
    if (timer_) {
        lv_timer_del(timer_);
        timer_ = nullptr;
    }

    // Clear toast queue
    for (auto toast : toast_.queue_) {
        delete toast;
    }
    toast_.queue_.clear();
    if (toast_.cur_) {
        delete toast_.cur_;
        toast_.cur_ = nullptr;
    }

    // Clear dialog queue
    for (auto dialog : dialog_.queue_) {
        delete dialog;
    }
    dialog_.queue_.clear();
}

bool PromptServer::push(Prompt* prompt)
{
    if (!prompt)
        return false;

    promptList_t* queueIs = nullptr;
    if (prompt->type() == Prompt::TYPE_TOAST) {
        queueIs = &toast_.queue_;
    } else if (prompt->type() == Prompt::TYPE_DIALOG) {
        queueIs = &dialog_.queue_;
    }

    if (!queueIs) {
        return false;
    }

    queueIs->push_back(prompt);
    return true;
}

void PromptServer::remove(Prompt* prompt)
{
    if (!prompt)
        return;

    promptList_t* queueIs = nullptr;
    if (prompt->type() == Prompt::TYPE_TOAST) {
        queueIs = &toast_.queue_;
        if (prompt == toast_.cur_) {
            toast_.cur_ = nullptr;
        }
    } else if (prompt->type() == Prompt::TYPE_DIALOG) {
        queueIs = &dialog_.queue_;
    }

    if (!queueIs) {
        return;
    }
    queueIs->remove(prompt);
}

void PromptServer::run(uint32_t tick)
{
    toast_.run(tick);
    dialog_.run(tick);
}

Prompt* PromptServer::Toasts::pop()
{
    Prompt* handle = nullptr;
    if (!queue_.empty()) {
        handle = queue_.front();
        queue_.pop_front();
    }
    return handle;
}

void PromptServer::Toasts::run(uint32_t tick)
{
    if (cur_ && cur_->timeout(tick)) {
        delete cur_; // This will trigger the deletion via event mechanism
        cur_ = nullptr;
    }

    if (!cur_) {
        cur_ = pop();
        if (cur_) {
            cur_->draw();
        }
    }
}

Prompt* PromptServer::Dialogs::pop()
{
    Prompt* handle = nullptr;
    if (!queue_.empty()) {
        handle = queue_.front();
        queue_.pop_front();
    }
    return handle;
}

void PromptServer::Dialogs::run(uint32_t tick)
{
    Prompt* tmp = pop();
    if (tmp) {
        tmp->draw();
    }
}

} // namespace prompt
