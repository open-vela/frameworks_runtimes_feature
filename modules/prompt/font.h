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
#ifndef FERRY_FONT_H
#define FERRY_FONT_H

#include "lvgl/lvgl.h" // Assuming LVGL is available via other means

#include <string>

// Define default font sizes if not already defined
#ifndef PROMPT_DIALOG_MSG_FONT_SIZE
#define PROMPT_DIALOG_MSG_FONT_SIZE 20
#endif
#ifndef PROMPT_DIALOG_TITLE_FONT_SIZE
#define PROMPT_DIALOG_TITLE_FONT_SIZE 24
#endif
#ifndef PROMPT_TOAST_FONT_SIZE
#define PROMPT_TOAST_FONT_SIZE 20
#endif
namespace prompt {
class Font {
public:
    enum FONT_STYLE {
        STYLE_NORMAL = 0,
        STYLE_ITALIC = 1 << 0,
        STYLE_BOLD = 1 << 1
    };
    Font(uint16_t size, uint16_t style = STYLE_NORMAL, const char* name = nullptr);
    const lv_font_t* getFont()
    {
        return font_;
    }
    ~Font();

private:
    std::string name_;
    uint16_t size_;
    uint16_t style_;
    lv_font_t* font_;
};
}
#endif // FERRY_FONT_H
