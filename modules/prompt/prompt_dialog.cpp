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

#include "prompt_dialog.h"
#include "font.h"

extern "C" {
LV_IMG_DECLARE(prompt_ok)
LV_IMG_DECLARE(prompt_cancel)
}

namespace prompt {

class DialogStyle {
public:
    DialogStyle()
        : fontMsg(kPromptDialogMsgSize)
        , fontTitle(kPromptDialogTitleSize)
    {
        lv_style_init(&styleMsg);
        lv_style_set_text_color(&styleMsg, lv_color_white());
        lv_style_set_text_font(&styleMsg, fontMsg.getFont());

        lv_style_init(&styleTitle);
        lv_style_set_text_color(&styleTitle, lv_color_white());
        lv_style_set_text_font(&styleTitle, fontTitle.getFont());

        lv_style_init(&styleBtn);
        lv_style_set_size(&styleBtn, 110, 110);
        lv_style_set_text_font(&styleBtn, fontMsg.getFont());
        lv_style_set_radius(&styleBtn, LV_RADIUS_CIRCLE);
        lv_style_set_bg_color(&styleBtn, LV_COLOR_MAKE(13, 132, 255));
        lv_style_set_shadow_width(&styleBtn, 0);
    }

    virtual ~DialogStyle()
    {
        lv_style_reset(&styleMsg);
        lv_style_reset(&styleTitle);
        lv_style_reset(&styleBtn);
    }

    Font fontMsg;
    Font fontTitle;
    lv_style_t styleMsg;
    lv_style_t styleBtn;
    lv_style_t styleTitle;
};

/**********************
 *  MEMBER FUNCTIONS
 **********************/

PromptDialog::PromptDialog(lv_obj_t* parent)
    : Prompt(parent)
    , style_(std::make_unique<DialogStyle>())
    , obj_(nullptr)
    , objOk_(nullptr)
    , objCancel_(nullptr)
    , autoCancel_(true)
{
}

PromptDialog::~PromptDialog()
{
    close();
}

Prompt::Type PromptDialog::type() const
{
    return Prompt::TYPE_DIALOG;
}

bool PromptDialog::setAttr(ID attrId, const char* value)
{
    if (!value)
        return false;
    if (attrId == ID_MESSAGE) {
        msg_ = value;
        return true;
    }
    if (attrId == ID_TITLE) {
        title_ = value;
        return true;
    }
    return false;
}

bool PromptDialog::setAttr(ID attrId, int32_t value)
{
    if (attrId == ID_AUTOCANCEL) {
        autoCancel_ = value ? true : false;
        return true;
    }
    return false;
}

void PromptDialog::close()
{
    if (obj_) {
        lv_obj_del(obj_);
        obj_ = nullptr;
        objOk_ = nullptr;
        objCancel_ = nullptr;
        delete this;
    }
}

void PromptDialog::draw()
{
    const int32_t ofsbtnY = 30;
    const int32_t ofstitleY = 38;

    if (obj_)
        return;
    lv_obj_create_info_t info = { false };
    lv_obj_set_size(getParent(), LV_HOR_RES, LV_VER_RES);
    lv_obj_center(getParent());
    lv_obj_set_style_bg_opa(getParent(), LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(getParent(), lv_color_black(), LV_PART_MAIN);

    obj_ = lv_obj_create(getParent());
    lv_obj_set_size(obj_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(obj_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(obj_, 0, LV_PART_MAIN);
    lv_obj_center(obj_);

    objCancel_ = lv_btn_create(obj_);
    lv_obj_align(objCancel_, LV_ALIGN_BOTTOM_LEFT, 90, -ofsbtnY);
    lv_obj_add_style(objCancel_, &style_->styleBtn, LV_PART_MAIN);
    lv_obj_set_style_bg_color(objCancel_, LV_COLOR_MAKE(255, 255, 255), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(objCancel_, LV_OPA_10, LV_PART_MAIN);
    lv_obj_t* imgCancel = lv_image_create(objCancel_);
    lv_image_set_src(imgCancel, &prompt_cancel);
    lv_obj_center(imgCancel);

    objOk_ = lv_btn_create(obj_);
    lv_obj_align(objOk_, LV_ALIGN_BOTTOM_RIGHT, -90, -ofsbtnY);
    lv_obj_add_style(objOk_, &style_->styleBtn, LV_PART_MAIN);
    lv_obj_t* imgOk = lv_image_create(objOk_);
    lv_image_set_src(imgOk, &prompt_ok);
    lv_obj_center(imgOk);

    lv_obj_t* objtitle_ = lv_spangroup_create(obj_);
    lv_obj_align(objtitle_, LV_ALIGN_TOP_MID, 0, ofstitleY);
    lv_spangroup_set_align(objtitle_, LV_TEXT_ALIGN_CENTER);
    lv_spangroup_set_overflow(objtitle_, LV_SPAN_OVERFLOW_ELLIPSIS);
    lv_obj_add_style(objtitle_, &style_->styleTitle, LV_PART_MAIN);
    lv_span_t* titleSpan = lv_spangroup_new_span(objtitle_);
    lv_span_set_text_static(titleSpan, title_.c_str());

    int32_t btnHeight = lv_obj_get_style_height(objOk_, LV_PART_MAIN);
    int32_t titleHeight = lv_obj_get_style_text_font(objtitle_, LV_PART_MAIN)->line_height;
    int32_t pad = lv_obj_get_style_pad_bottom(obj_, LV_PART_MAIN) + lv_obj_get_style_pad_top(obj_, LV_PART_MAIN);
    int32_t msg_height = height_ - pad - ofstitleY - titleHeight - ofsbtnY - btnHeight - 10;

    lv_obj_t* msgContainer = lv_obj_create_ex(obj_, &info);
    lv_obj_set_y(msgContainer, ofstitleY + titleHeight + 5);
    lv_obj_set_size(msgContainer, LV_PCT(100), msg_height);

    lv_obj_t* objmsg_ = lv_spangroup_create(msgContainer);
    lv_obj_set_width(objmsg_, LV_PCT(90));
    lv_obj_center(objmsg_);
    lv_spangroup_set_mode(objmsg_, LV_SPAN_MODE_BREAK);
    lv_spangroup_set_align(objmsg_, LV_TEXT_ALIGN_CENTER);
    lv_spangroup_set_overflow(objmsg_, LV_SPAN_OVERFLOW_ELLIPSIS);
    lv_obj_add_style(objmsg_, &style_->styleMsg, LV_PART_MAIN);
    lv_span_t* span_ = lv_spangroup_new_span(objmsg_);
    lv_span_set_text_static(span_, msg_.c_str());

    // Attach generic prompt_event_cb to all relevant objects
    lv_obj_add_event_cb(obj_, prompt_event_cb, LV_EVENT_DELETE, this);
    lv_obj_add_event_cb(objOk_, prompt_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(objCancel_, prompt_event_cb, LV_EVENT_CLICKED, this);
}

void PromptDialog::handleLvglEvent(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));

    if (code == LV_EVENT_CLICKED) {
        if (target == objOk_) {
            sendEvent(Prompt::EVENT_ID_SUCCESS, 0);
        } else if (target == objCancel_) {
            sendEvent(Prompt::EVENT_ID_CANCEL, 1);
        } else if (target == obj_ && autoCancel_) {
            sendEvent(Prompt::EVENT_ID_CANCEL);
        }
        close();
    }
}

}