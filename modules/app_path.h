/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APP_PATH_H
#define APP_PATH_H

#ifdef __NuttX__
#include <nuttx/config.h>
#endif

#include <limits.h>

#include "feature_types.h"

#ifndef CONFIG_PATH_MAX
#define CONFIG_PATH_MAX PATH_MAX
#endif

int app_check_path(const char* path);
char* app_relative_to_absolute_path(const char* pkg, const char* relative_path);
char* app_relative_path_generator(const char* pkg, const char* type, const char* filename);
char* app_absolute_to_relative_path(const char* pkg, const char* absolute_path);
char* app_absolute_path_generator(const char* pkg, const char* type, const char* filename);
bool check_disk_limit();
void notify_disk_space_insufficient(FeatureInstanceHandle feature, const char* pkg);
bool is_path_in_tmp(const char* path);

#endif
