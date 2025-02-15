/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#ifndef __FEATURE_TRACKER_H__
#define __FEATURE_TRACKER_H__

#include "feature_log.h"
#include "feature_trace.h"
#include "framework_log.h"

#include <map>
#include <string>

namespace feature_framework {

class Tracker {
public:
    explicit Tracker(const char* name);

    ~Tracker();

    void begin(const char* extra);

    void end(const char* extra);

    long long duration() const
    {
        return total_duration_;
    }

    long long times() const
    {
        return times_;
    }

private:
    inline static int64_t now_ms()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    long long begin_time_;
    long long total_duration_;
    long long times_;
    std::string name_;
    bool enable_sched_;
};

class FeatureTracker {
public:
    explicit FeatureTracker(const char* name);

    ~FeatureTracker();

    void begin(const char* func_name, const char* extra = "");

    void end(const char* func_name, const char* extra = "");

    void setIsInterface(bool is_interface)
    {
        is_interface_ = is_interface;
    }

    bool isInterface()
    {
        return is_interface_;
    }

    void setName(const char* name)
    {
        feature_name_ = name;
    }

private:
    void printFeatureInfo();

    std::string feature_name_;
    bool is_interface_;
    std::map<std::string, Tracker> func_info_map_;
};

}
#endif // __FEATURE_TRACKER_H__