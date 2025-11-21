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
#ifndef FEATURE_MODULES_PROMPT_PROMPT_UTILS_H
#define FEATURE_MODULES_PROMPT_PROMPT_UTILS_H

#include <cstdint>
#include <string>

#include "prompt.h"

namespace prompt {
std::unique_ptr<Prompt> CreatePrompt(Prompt::Type type, lv_obj_t* parent);
} // namespace prompt

#endif // FEATURE_MODULES_PROMPT_PROMPT_UTILS_H