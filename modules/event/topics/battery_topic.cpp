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

#include "event/topic.h"
#include "feature.h"
#include "system/state.h"

using namespace ft_system_event;
#define BATTERY_EVENT_NAME struct battery_state

static bool deal_battary_change(FeatureInstanceHandle handle, void* pre, void* cur, ft_value_t& res)
{
    BATTERY_EVENT_NAME* pre_state = static_cast<BATTERY_EVENT_NAME*>(pre);
    BATTERY_EVENT_NAME* cur_state = static_cast<BATTERY_EVENT_NAME*>(cur);
    if (pre_state == nullptr || pre_state->level != cur_state->level) {
        double level = (double)(cur_state->level) / 100;
        res = ft_from_double(FeatureGetContext(handle), level);
        return true;
    }
    return false;
}

static bool deal_charging(FeatureInstanceHandle handle, void* pre, void* cur, ft_value_t& res)
{
    BATTERY_EVENT_NAME* pre_state = static_cast<BATTERY_EVENT_NAME*>(pre);
    BATTERY_EVENT_NAME* cur_state = static_cast<BATTERY_EVENT_NAME*>(cur);
    if (pre_state != nullptr && pre_state->state == 0 && cur_state->state == 1) {
        res = ft_undefined(FeatureGetContext(handle));
        return true;
    }
    return false;
}

static bool deal_discharging(FeatureInstanceHandle handle, void* pre, void* cur, ft_value_t& res)
{
    BATTERY_EVENT_NAME* pre_state = static_cast<BATTERY_EVENT_NAME*>(pre);
    BATTERY_EVENT_NAME* cur_state = static_cast<BATTERY_EVENT_NAME*>(cur);
    if (pre_state != nullptr && pre_state->state == 1 && cur_state->state == 0) {
        res = ft_undefined(FeatureGetContext(handle));
        return true;
    }
    return false;
}

static const event_info_t battery_events[] = {
    { "usual.event.BATTERY_CHANGED",
        deal_battary_change },
    { "usual.event.CHARGING",
        deal_charging },
    { "usual.event.DISCHARGING",
        deal_discharging }
};

DECLARE_EXTERN_TOPIC(battery_topic) = {
    ORB_ID(battery_state),
    sizeof(battery_events) / sizeof(battery_events[0]),
    battery_events
};
