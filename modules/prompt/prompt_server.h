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
#ifndef FEATURE_MODULES_PROMPT_PROMPT_SERVER_H
#define FEATURE_MODULES_PROMPT_PROMPT_SERVER_H

#include "prompt_dialog.h"
#include "prompt_toast.h"
#include <list>
#include <memory>

#define TOAST_QUEUE_LIMIT 10
namespace prompt {

class PromptServer {
public:
    typedef std::list<Prompt*> PromptList;

    PromptServer();
    virtual ~PromptServer();

    struct Toasts {
        Toasts()
            : cur_(nullptr)
        {
        }
        Prompt* pop();
        void run(uint32_t tick);

        Prompt* cur_;
        PromptList queue_;
    };

    struct Dialogs {
        Prompt* pop();
        void run(uint32_t tick);
        PromptList queue_;
    };

    static void InitServerTask(void* user_data);
    static void DestroyServerTask(void* user_data);

    static Prompt* popPrompt(PromptList& queue);

    void init();
    void uninit();

    bool push(Prompt* prompt);
    void remove(Prompt* prompt);
    void run(uint32_t tick);

private:
    lv_timer_t* timer_;
    Toasts toast_;
    Dialogs dialog_;
};

} // namespace prompt

#endif // FEATURE_MODULES_PROMPT_PROMPT_SERVER_H
