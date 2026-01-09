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
#include "prompt.h"

#include "feature_log.h"

namespace prompt {

// Generic LVGL event handler for Prompt objects
extern "C" void prompt_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    Prompt* prompt = static_cast<Prompt*>(lv_event_get_user_data(e));

    if (!prompt)
        return;

    if (code == LV_EVENT_DELETE) {
        prompt->sendEvent(Prompt::EVENT_ID_COMPLETE);
    } else {
        // Delegate other events to the specific prompt implementation
        prompt->handleLvglEvent(e);
    }
}
Prompt::Prompt(lv_obj_t* root)
    : parent_(root)
    , width_(0)
    , height_(0)
    , user_data_(nullptr)
    , eventCB_(nullptr)
{
    if (!root) {
        FEATURE_LOG_ERROR("root object is null");
        return;
    }
    width_ = lv_obj_get_style_width(parent_, LV_PART_MAIN);
    height_ = lv_obj_get_style_height(parent_, LV_PART_MAIN);
}

Prompt::~Prompt()
{
}

bool Prompt::setAttr(Prompt::ID /* attrId */, const char* /* value */)
{
    return false;
}

bool Prompt::setAttr(Prompt::ID /* attrId */, int32_t /* value */)
{
    return false;
}

bool Prompt::timeout(uint32_t /* tick */)
{
    return true;
}

void Prompt::sendEvent(EventID id, int index)
{
    if (eventCB_) {
        EventInfo info;
        info.index = index;
        info.user_data = user_data_;
        eventCB_(id, &info);
    }
}

} // namespace prompt