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

#include "feature_tracker.h"

namespace feature_framework {

// Tracker
Tracker::Tracker(const char* name)
    : begin_time_(0)
    , total_duration_(0)
    , times_(0)
    , name_(name)
    , enable_sched_(false)
{
}

Tracker::~Tracker()
{
}

void Tracker::begin(const char* extra)
{
    QuickProfileLogBegin(QUICK_PROFILE_FEATURE_FRAMEWORK, name_.data(), extra ? extra : "");
    if (enable_sched_) {
        FEATURE_NOTE_BEGIN_STR(name_.data());
    }
    begin_time_ = now_ms();
    // FEATURE_LOG_INFO("%s: begin_time: %lld ms", name_.data(), begin_time_);
}

void Tracker::end(const char* extra)
{
    if (begin_time_ == 0) {
        return;
    }
    QuickProfileLogEnd(QUICK_PROFILE_FEATURE_FRAMEWORK, name_.data(), extra ? extra : "");
    if (enable_sched_) {
        FEATURE_NOTE_END_STR(name_.data());
    }
    long long end_time = now_ms();
    long long duration = end_time - begin_time_;
    total_duration_ += duration;
    times_++;
    // FEATURE_LOG_INFO("%s: end_time: %lld ms, duration: %lld ms", name_.data(), end_time, duration);
    begin_time_ = 0;
}

// FeatureTracker
FeatureTracker::FeatureTracker(const char* name)
    : feature_name_(name)
    , is_interface_(false)
{
}

FeatureTracker::~FeatureTracker()
{
    printFeatureInfo();
}

void FeatureTracker::begin(const char* func_name, const char* extra)
{
    auto it = func_info_map_.find(func_name);
    if (it == func_info_map_.end()) {
        auto full_name = feature_name_ + "." + func_name;
        auto ret = func_info_map_.emplace(func_name, full_name.data());
        it = ret.first;
    }
    it->second.begin(extra);
}

void FeatureTracker::end(const char* func_name, const char* extra)
{
    auto it = func_info_map_.find(func_name);
    if (it == func_info_map_.end()) {
        FEATURE_LOG_ERROR("wrong func_name '%s' for feature: %s", func_name, feature_name_.data());
        return;
    }
    it->second.end(extra);
}

void FeatureTracker::printFeatureInfo()
{
    FEATURE_LOG_INFO("###### ================== %s '%s' trace status begin ==================",
        (is_interface_ ? "interface" : "feature"), feature_name_.data());
    for (const auto& pair : func_info_map_) {
        FEATURE_LOG_INFO("function '%s' called %lld times, totally spent: %lld ms, average: %d ms",
            pair.first.data(), pair.second.times(), pair.second.duration(), (pair.second.duration() / pair.second.times()));
    }
    FEATURE_LOG_INFO("###### ================== %s '%s' trace status end ==================\n",
        (is_interface_ ? "interface" : "feature"), feature_name_.data());
}

}