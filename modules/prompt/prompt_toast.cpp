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

#include "prompt_toast.h"
#include "font.h"

#include "feature_log.h"

namespace prompt {

class ToastStyle {
public:
    ToastStyle()
        : fontIs(PROMPT_TOAST_FONT_SIZE)
    {
        lv_style_init(&style);
        lv_style_set_text_font(&style, fontIs.getFont());
        lv_style_set_bg_color(&style, lv_color_make(36, 36, 36));
        lv_style_set_bg_opa(&style, 242);
        lv_style_set_radius(&style, LV_RADIUS_CIRCLE);
        lv_style_set_pad_ver(&style, 16);
        lv_style_set_pad_hor(&style, 25);
        lv_style_set_text_color(&style, lv_color_white());
    }

    virtual ~ToastStyle()
    {
        lv_style_reset(&style);
    }

    Font fontIs;
    lv_style_t style;
};

PromptToast::PromptToast(lv_obj_t* parent)
    : Prompt(parent)
    , style_(std::make_unique<ToastStyle>())
    , duration_(1500)
    , counter_(0)
    , obj_(nullptr)
{
}

PromptToast::~PromptToast()
{
    close();
}

Prompt::Type PromptToast::type() const
{
    return Prompt::TYPE_TOAST;
}

bool PromptToast::setAttr(ID attrId, const char* value)
{
    if (!value)
        return false;
    if (attrId == ID_MESSAGE) {
        msg_ = value;
        return true;
    }
    return false;
}

bool PromptToast::setAttr(ID attrId, int32_t value)
{
    if (attrId == ID_DURATION) {
        if (value >= 1500 && value <= 10000) {
            duration_ = value;
            return true;
        }
    }

    return false;
}

bool PromptToast::timeout(uint32_t tick)
{
    counter_ += tick;
    if (counter_ >= duration_) {
        sendEvent(EVENT_ID_COMPLETE);
        return true;
    }
    return false;
}

void PromptToast::close()
{
    FEATURE_LOG_INFO("PromptToast::close() BEGIN, this=%p, obj_=%p", this, obj_);
    if (obj_) {
        lv_obj_del(obj_);
        obj_ = nullptr;
    }
}

/**
 * @brief 绘制提示框(Toast)的函数
 * 该函数负责创建并配置一个提示框对象，设置其样式、文本内容和动画效果
 */
void PromptToast::draw()
{
    FEATURE_LOG_INFO("PromptToast::draw() BEGIN, this=%p", this);
    // 如果对象已经存在，则直接返回
    if (obj_) {
        FEATURE_LOG_INFO("PromptToast::draw() obj_ exists, returning. this=%p", this);
        return;
    }

    // Use a simple label for toast message, which is more appropriate.
    obj_ = lv_spangroup_create(getParent());
    if (!obj_) {
        counter_ = duration_; // ensure timeout happens immediately
        FEATURE_LOG_WARN("Failed to create toast object");
        FEATURE_LOG_INFO("PromptToast::draw() Failed to create obj_, returning. this=%p", this);
        return;
    }

    lv_obj_align(obj_, LV_ALIGN_BOTTOM_MID, 0, -50); // Position at bottom center
    lv_obj_add_flag(obj_, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow event propagation
    lv_spangroup_set_align(obj_, LV_TEXT_ALIGN_CENTER);
    lv_spangroup_set_overflow(obj_, LV_SPAN_OVERFLOW_ELLIPSIS);
    lv_obj_add_style(obj_, &style_->style, 0); // 查 InnerToastStyle

    lv_span_t* span_ = lv_spangroup_new_span(obj_);
    lv_span_set_text_static(span_, msg_.c_str());

    width_ = lv_obj_get_style_width(getParent(), LV_PART_MAIN);
    height_ = lv_obj_get_style_height(getParent(), LV_PART_MAIN);

    int strLen = (int)lv_spangroup_get_expand_width(obj_, width_);
    int width = width_ * 0.9;
    if (strLen < width) {
        lv_spangroup_set_mode(obj_, LV_SPAN_MODE_EXPAND);
    } else {
        lv_obj_set_width(obj_, width);
        lv_spangroup_set_mode(obj_, LV_SPAN_MODE_BREAK);
    }

    // Animation to fade out.
    // The timeout() mechanism in PromptServer will handle the destruction.
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj_);
    uint32_t tmp = 1500 / 4;
    lv_anim_set_time(&a, tmp); // 500ms fade out
    if (duration_ > tmp) {
        lv_anim_set_delay(&a, duration_ - tmp);
    }
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
    FEATURE_LOG_INFO("PromptToast::draw() END, this=%p", this);
}

}