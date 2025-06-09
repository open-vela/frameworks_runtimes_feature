/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#include "miwear_settings_mock.h"

#include <cassert>

struct DataInfo {
    const char* key;
    int type = 0;
    union {
        int num;
        const char* str;
    } u;
} static_value[] = {
    { .key = "car.showVehicleBall", .type = 0, .u = { .num = 0 } },
    { .key = "car.misOnline", .type = 0, .u = { .num = 0 } },
    { .key = "car.cockpitMode", .type = 0, .u = { .num = 0 } },
    { .key = "car.navigationVibration", .type = 0, .u = { .num = 0 } },
    { .key = "car.fatigueDrivingVibration", .type = 0, .u = { .num = 0 } },
    { .key = "car.drivingHeartRate", .type = 0, .u = { .num = 0 } },
    { .key = "car.gestureControl", .type = 0, .u = { .num = 0 } },
    { .key = "car.wristRolling", .type = 0, .u = { .num = 0 } },
    { .key = "car.wristShaking", .type = 0, .u = { .num = 0 } },
    { .key = "car.fingerSnapping", .type = 0, .u = { .num = 0 } },
    { .key = "car.vid", .type = 1, .u = { .str = "123" } },
};

miwear_settings_t* miwear_settings_create(void* user_data)
{
    miwear_settings_t* settings = new miwear_settings_t;
    settings->user_data = user_data;
    for (auto& data : static_value) {
        if (data.type == 0) {
            settings->props[data.key] = std::to_string(data.u.num);
        } else { // string
            settings->props[data.key] = data.u.str;
        }
    }
    return settings;
}
int miwear_settings_destroy(miwear_settings_t* settings, miwear_settings_callback cb)
{
    miwear_settings_message_t msg = {
        .type = MIWEARSETTINGS_MESSAGE_TYPE_DESTROY,
        .status = 0,
        .data = nullptr,
    };
    cb(settings, &msg, nullptr);
    delete settings;
    return 0;
}

bool is_number(const std::string& s)
{
    bool is_num = true;
    for (auto c : s) {
        if (!isdigit(c) || c != '.') {
            is_num = false;
            break;
        }
    }
    return is_num;
}

int miwear_settings_getprop(miwear_settings_t* settings, const char* key,
    miwear_settings_callback cb, void* arg)
{
    assert(key);

    char buffer[512] = { 0 };
    miwear_settings_message_t msg = {
        .type = MIWEARSETTINGS_MESSAGE_TYPE_GET_PROPS,
        .status = 0,
        .data = buffer,
    };
    size_t len = sizeof(buffer);
    if (strcmp(key, "car") == 0) {
        const char* prefix = "{ \"car\": ";
        strcpy(buffer, prefix);
        auto cur = buffer + strlen(prefix);
        auto end = buffer + len;

        *cur++ = '{';
        bool first = true;
        for (auto& [sub_key, val] : settings->props) {
            if (!first) {
                *cur++ = ',';
            }
            bool is_num = is_number(val);

            int cnt = 0;
            if (is_num) {
                cnt = snprintf(cur, end - cur, "\"%s\" : %s", sub_key.c_str() + 4, val.c_str());
            } else {
                cnt = snprintf(cur, end - cur, "\"%s\" : \"%s\"", sub_key.c_str() + 4, val.c_str());
            }
            assert(cnt >= 0);
            cur += cnt;
            first = false;
        }
        *cur++ = '}';

        *cur++ = '}'; // suffix
    } else if (strncmp(key, "car", 3) == 0) {
        // start with car, get the specific prop
        bool is_num = is_number(settings->props[key]);
        if (is_num) {
            snprintf(buffer, sizeof(buffer), "{\"%s\": %s}", key, settings->props[key].c_str());
        } else {
            snprintf(buffer, sizeof(buffer), "{\"%s\": \"%s\"}", key, settings->props[key].c_str());
        }
    } else {
        msg.status = 1;
    }

    cb(settings, &msg, arg);

    return 0;
}

void trigger_prop_update(miwear_settings_t* miwearsettings, const char* key)
{
    const int len = 100;
    char buffer[len] = { 0 };
    snprintf(buffer, len, "{ \"%s\": %s}", key, miwearsettings->props[key].c_str());
    miwear_settings_message_t msg {
        .type = MIWEARSETTINGS_MESSAGE_TYPE_PROP_UPDATE,
        .status = 0,
        .data = buffer,
    };
    for (auto& [cb, arg] : miwearsettings->subscribers[key]) {
        cb(miwearsettings, &msg, arg);
    }
}

int miwear_settings_setprop(miwear_settings_t* miwearsettings, const char* key,
    miwear_settings_message_t* msg,
    miwear_settings_callback cb, void* arg)
{
    assert(key);
    assert(msg);
    assert(msg->data);
    miwear_settings_message_t mew_msg {
        .type = MIWEARSETTINGS_MESSAGE_TYPE_SET_PROPS,
        .status = 0,
        .data = nullptr,
    };
    if (!miwearsettings->props.count(key)) {
        mew_msg.status = 1;
    } else {
        miwearsettings->props[key] = (const char*)msg->data;
        trigger_prop_update(miwearsettings, key);
    }

    cb(miwearsettings, &mew_msg, arg);

    return 0;
}
int miwear_settings_subscribe_prop(miwear_settings_t* settings, const char* key,
    miwear_settings_callback onPropUpdate,
    void* arg)
{
    assert(key);
    assert(onPropUpdate);

    settings->subscribers[key].emplace_back(onPropUpdate, arg);

    return 0;
}
int miwear_settings_unsubscribe_prop(miwear_settings_t* settings,
    const char* key)
{
    // TODO, 这里做不到区分，feature实例。如果有多个feature实例，订阅了同一个属性，
    // 那么取消订阅的时候，会把所有订阅都取消掉
    assert(key);
    settings->subscribers.erase(key);
    return 0;
}
