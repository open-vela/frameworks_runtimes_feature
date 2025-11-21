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
#include "font.h"

#include "feature_log.h"
#include <uikit/uikit.h>

namespace prompt {

#ifndef FONT_DEFAULT_NORMAL_NAME
#ifdef CONFIG_FONT_DEFAULT_NORMAL_NAME
#define FONT_DEFAULT_NORMAL_NAME CONFIG_FONT_DEFAULT_NORMAL_NAME
#else
#define FONT_DEFAULT_NORMAL_NAME "MiSansW_Regular"
#endif
#endif

#ifndef FONT_DEFAULT_BOLD_NAME
#ifdef CONFIG_FONT_DEFAULT_BOLD_NAME
#define FONT_DEFAULT_BOLD_NAME CONFIG_FONT_DEFAULT_BOLD_NAME
#else
#define FONT_DEFAULT_BOLD_NAME "MiSansW_Demibold"
#endif
#endif

#ifndef FONT_DEFAULT_SIZE
#ifdef CONFIG_FONT_DEFAULT_SIZE
#define FONT_DEFAULT_SIZE CONFIG_FONT_DEFAULT_SIZE
#else
#define FONT_DEFAULT_SIZE 30
#endif
#endif

Font::Font(uint16_t size, uint16_t style, const char* name)
    : name_(name ? name : (style == STYLE_BOLD ? FONT_DEFAULT_BOLD_NAME : FONT_DEFAULT_NORMAL_NAME))
    , size_(size ? size : FONT_DEFAULT_SIZE)
    , style_(style)
    , font_(nullptr)
{
    font_ = vg_font_create(name_.c_str(), size_, (lv_freetype_font_style_t)style_);
    if (!font_) {
        FEATURE_LOG_ERROR("Failed to create font %s", name_.c_str());
    }
}

Font::~Font()
{
    vg_font_destroy(font_);
}
}