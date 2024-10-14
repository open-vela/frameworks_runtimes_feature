
/*
 * Copyright (C) 2023 Xiaomi Corporation
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

#ifndef __FEATURE_EVENT_MANAGER_H__
#define __FEATURE_EVENT_MANAGER_H__

#include "feature_description.h"
#include "feature_log.h"

#include <list>
#include <map>

namespace feature_framework {

template <typename TCtx, typename TTarget>
class EventData {
public:
    EventData(TCtx ctx, const MemberEvent* member_event)
        : ctx_(ctx)
        , member_event_(member_event)
    {
    }

    EventData(const EventData& ed)
        : ctx_(ed.ctx_)
        , member_event_(ed.member_event_)
        , callbacks_(ed.callbacks_)
    {
    }

    ~EventData()
    {
        clear();
    }

    void addCallback(TTarget& cb)
    {
        if (findEvent(cb)) {
            return;
        }
        addRef(ctx_, cb);
        callbacks_.push_back(cb);
    }

    void eraseCallback(TTarget& cb)
    {
        callbacks_.remove_if([this, &cb](TTarget& val) {
            bool ret = isSameValue(ctx_, val, cb);
            if (ret) {
                releaseRef(ctx_, val);
            }
            return ret;
        });
    }

    std::list<TTarget>& getCallbacks()
    {
        return callbacks_;
    }

    const MemberEvent* memberEvent()
    {
        return member_event_;
    }

    void clear()
    {
        for (auto& val : callbacks_) {
            releaseRef(ctx_, val);
        }
        callbacks_.clear();
    }

    int getCallbackCount()
    {
        return callbacks_.size();
    }

private:
    bool findEvent(TTarget& cb)
    {
        for (auto& val : callbacks_) {
            if (isSameValue(ctx_, val, cb))
                return true;
        }
        return false;
    }

    TCtx ctx_;
    const MemberEvent* member_event_;
    std::list<TTarget> callbacks_;
};

template <typename TCtx, typename TTarget, typename TInstance>
class EventManager {
public:
    using event_data_t = EventData<TCtx, TTarget>;
    using event_map_t = std::map<FtEventId, event_data_t*>;

    ~EventManager()
    {
        clearAllEvents();
    }

    FtEventId addEventCallback(const MemberEvent* member_event, TTarget& event_cb)
    {
        auto eid = findEventId(member_event);
        if (eid <= 0) {
            auto data = new event_data_t(context(), member_event);
            eid = member_event->id;
            events_map_[eid] = data;
            if (listener_) {
                listener_(listener_data_, eid, FEATURE_EVENT_ADDED);
            }
        } else {
            FEATURE_LOG_DEBUG("event id %d has already added!", eid);
        }
        events_map_[eid]->addCallback(event_cb);
        return eid;
    }

    void eraseEventCallback(const MemberEvent* member_event, TTarget& event_cb)
    {
        auto eid = findEventId(member_event);
        if (eid <= 0) {
            FEATURE_LOG_DEBUG("event id %d not exist !", eid);
            return;
        }
        events_map_[eid]->eraseCallback(event_cb);
    }

    bool removeEventCallbacks(FtEventId eid, bool keep_id = true)
    {
        if (eid <= 0) {
            FEATURE_LOG_DEBUG("event id %d not exist !", eid);
            return false;
        }
        if (!events_map_.count(eid)) {
            FEATURE_LOG_ERROR("event id %d not exist !", eid);
            return false;
        }
        events_map_[eid]->clear();
        if (keep_id) {
            return true;
        }

        if (listener_) {
            listener_(listener_data_, eid, FEATURE_EVENT_REMOVED);
        }
        delete events_map_[eid];
        events_map_.erase(eid);
        return true;
    }

    std::list<TTarget> getEventCallbacks(const MemberEvent* member_event)
    {
        auto eid = findEventId(member_event);
        if (eid <= 0) {
            FEATURE_LOG_DEBUG("event id %d not exist !", eid);
            return std::list<TTarget>();
        }
        return events_map_[eid]->getCallbacks();
    }

    void clearAllEvents()
    {
        for (auto& it : events_map_) {
            if (listener_) {
                listener_(listener_data_, it.first, FEATURE_EVENT_REMOVED);
            }
            delete it.second;
        }
        events_map_.clear();
    }

    event_data_t* getEventData(FtEventId eid)
    {
        if (!events_map_.count(eid)) {
            return nullptr;
        }
        return events_map_[eid];
    }

    FtEventId findEventId(const MemberEvent* event)
    {
        if (!event)
            return 0;

        for (auto& it : events_map_) {
            FtEventId eid = it.first;
            const MemberEvent* member_event = it.second->memberEvent();
            if (member_event == event) {
                if (strcmp(member_event->name, event->name) != 0) {
                    FEATURE_LOG_DEBUG("event id %d has different name: %s, %s!", eid, member_event->name, event->name);
                }
                return eid;
            }
        }
        return 0;
    }

protected:
    bool doEmitEvent(event_data_t* ev_data, va_list& va, int fixed_argc, int rest_argc)
    {
        if (!ev_data)
            return false;

        auto member_event = ev_data->memberEvent();
        for (auto& cb : ev_data->getCallbacks()) {
            va_list va_copy;
            va_copy(va_copy, va);
            static_cast<TInstance*>(this)->doInvokeCallback(member_event->parameters, cb, va_copy, fixed_argc, rest_argc);
            va_end(va_copy);
        }
        return true;
    }

    void doSetEventChangeListener(FeatureEventChangeListener listener, void* data)
    {
        listener_ = listener;
        listener_data_ = data;
    }

    FtEventId doGetEventId(const char* name)
    {
        for (auto& it : events_map_) {
            FtEventId eid = it.first;
            const MemberEvent* member_event = it.second->memberEvent();
            if (strcmp(member_event->name, name) == 0) {
                return eid;
            }
        }
        return 0;
    }

    const char* doGetEventName(FtEventId eid)
    {
        event_data_t* ev_data = getEventData(eid);
        if (ev_data) {
            return ev_data->memberEvent()->name;
        }
        return nullptr;
    }

    int doGetEventCallbackCount(FtEventId eid)
    {
        event_data_t* ev_data = getEventData(eid);
        if (ev_data) {
            return ev_data->getCallbackCount();
        }
        return 0;
    }

private:
    TCtx context()
    {
        return static_cast<TInstance*>(this)->getContext();
    }

    FtEventId curr_eid_ = 0;
    event_map_t events_map_;
    FeatureEventChangeListener listener_ = nullptr;
    void* listener_data_ = nullptr;
};

}
#endif // __FEATURE_EVENT_MANAGER_H__
