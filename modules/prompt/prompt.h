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
#ifndef FEATURE_MODULES_PROMPT_PROMPT_H
#define FEATURE_MODULES_PROMPT_PROMPT_H

#include "lvgl/lvgl.h"
#include <string>

// This is the C-style callback that LVGL will use.
// It will be given a pointer to the Prompt object as user_data.
extern "C" void prompt_event_cb(lv_event_t* e);

namespace prompt {

class Prompt {
public:
    enum Type {
        TYPE_NONE,
        TYPE_TOAST,
        TYPE_DIALOG
    };

    enum ID {
        ID_MESSAGE,
        ID_DURATION,
        ID_TITLE,
        ID_BUTTONS,
        ID_AUTOCANCEL
    };

    enum EventID {
        EVENT_ID_NONE,
        EVENT_ID_SUCCESS,
        EVENT_ID_CANCEL,
        EVENT_ID_COMPLETE,
        EVENT_ID_END
    };

    struct EventInfo {
        int index; // index which button is clicked
        void* user_data;
    };

    typedef void (*PromptEventCB_t)(EventID, EventInfo*);

public:
    Prompt(lv_obj_t* parent);
    virtual ~Prompt();
    /* Return the prompt type */
    virtual Type type() const = 0;
    virtual bool setAttr(ID attrId, const char* value);
    virtual bool setAttr(ID attrId, int32_t value);
    virtual bool timeout(uint32_t tick);
    virtual void draw() = 0;
    virtual void close() = 0;

    lv_obj_t* getParent() { return parent_; }
    void setEventCB(PromptEventCB_t cb, void* user_data) { eventCB_ = cb, user_data_ = user_data; }
    void setUserData(void* data) { user_data_ = data; }
    void* getUserData() { return user_data_; }

    void sendEvent(EventID id, int index = 0);
    virtual void handleLvglEvent(lv_event_t* e) { } // New virtual method

    void setSize(int width, int height)
    {
        width_ = width;
        height_ = height;
    }

protected:
    lv_obj_t* parent_;
    int width_;
    int height_;
    void* user_data_;
    PromptEventCB_t eventCB_;
};

} // namespace prompt

#endif
