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
#ifndef FEATURE_MODULES_PROMPT_PROMPT_TOAST_H
#define FEATURE_MODULES_PROMPT_PROMPT_TOAST_H

#include "prompt.h"

namespace prompt {

class ToastStyle;
class PromptToast : public Prompt {
public:
    PromptToast(lv_obj_t* parent);
    ~PromptToast();
    Type type() const override;
    bool setAttr(ID attrId, const char* value) override;
    bool setAttr(ID attrId, int32_t value) override;
    bool timeout(uint32_t tick) override;
    void draw() override;
    void close() override;

private:
    std::unique_ptr<ToastStyle> style_;
    std::string msg_;
    uint32_t duration_; // ms
    uint32_t counter_; // ms
    lv_obj_t* obj_;
};

} // namespace prompt

#endif
