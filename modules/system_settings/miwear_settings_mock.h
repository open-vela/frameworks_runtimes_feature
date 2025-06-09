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
#ifndef SETTINGS_H
#define SETTINGS_H
#ifdef __cplusplus

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#endif

typedef struct miwear_settings miwear_settings_t;
typedef struct miwear_settings_message miwear_settings_message_t;

enum {
    MIWEARSETTINGS_MESSAGE_TYPE_DESTROY, // 实例已销毁
    MIWEARSETTINGS_MESSAGE_TYPE_GET_PROPS, // 获取所有属性
    MIWEARSETTINGS_MESSAGE_TYPE_SET_PROPS, // 设置属性
    MIWEARSETTINGS_MESSAGE_TYPE_PROP_UPDATE, // 属性更新
    MIWEARSETTINGS_MESSAGE_TYPE_SUBSCRIBE, // 订阅
    MIWEARSETTINGS_MESSAGE_TYPE_UNSUBSCRIBE, // 取消订阅
    MIWEAR_SETTINGS_TYPE_MAX,
};

typedef void (*miwear_settings_callback)(miwear_settings_t* settings,
    miwear_settings_message_t* msg,
    void* arg);

typedef struct miwear_settings {
    // uv_async_queue_t* async_queue;
    void* user_data;
    std::unordered_map<std::string, std::string> props;
    using SubInfo = std::pair<miwear_settings_callback, void*>;
    std::unordered_map<std::string, std::vector<SubInfo>> subscribers;
} miwear_settings_t;

typedef struct miwear_settings_message {
    uint8_t type;
    uint8_t status;
    const char* data;
} miwear_settings_message_t;

int miwear_settings_init(void);
void notify_subscribers(const char* key, char* response);

miwear_settings_t* miwear_settings_create(void* user_data);
int miwear_settings_destroy(miwear_settings_t* settings, miwear_settings_callback cb);
int miwear_settings_getprop(miwear_settings_t* settings, const char* key,
    miwear_settings_callback cb, void* arg);
int miwear_settings_setprop(miwear_settings_t* miwearsettings, const char* key,
    miwear_settings_message_t* msg,
    miwear_settings_callback cb, void* arg);
int miwear_settings_subscribe_prop(miwear_settings_t* settings, const char* key,
    miwear_settings_callback onPropUpdate,
    void* arg);
int miwear_settings_unsubscribe_prop(miwear_settings_t* settings,
    const char* key);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // SETTINGS_H