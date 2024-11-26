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

#ifndef __FEATURE_PROTOTYPE_H__
#define __FEATURE_PROTOTYPE_H__

#include "feature_description.h"
#include "feature_main_exports.h"
#include "feature_object_ref.h"
#ifdef CONFIG_FEATURE_ENABLE_TRACKER
#include "feature_tracker.h"
#endif

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace feature_framework {

class FeatureInstance;

class FeaturePrototype {

public:
    FeaturePrototype(const FeatureDescription* description);
    virtual ~FeaturePrototype();

    int addInstance(FeatureObjectUniquePtr<FeatureInstance>&& inst);

    bool removeInstance(size_t pos);

    bool hasInstanceAlive();

    void clearAllInstances();

    void setFeatureManager(class FeatureManager* manager) { feature_manager_ = manager; }

    class FeatureManager* featureManager() const { return feature_manager_; }

    void setNative(void* native) { native_ = native; }

    void* native() { return native_; }

    std::map<const char*, std::unique_ptr<FeaturePrototype>>& children() { return children_; }

    std::vector<FeatureObjectUniquePtr<FeatureInstance>>& instances() { return instances_; }

    const FeatureDescription* description() { return description_; }

    void setModulePrototype(FeaturePrototype* proto);

    FeaturePrototype* modulePrototype() { return module_proto_; }

    FeaturePrototype* getInterfacePrototype(const FeatureDescription* description);

    virtual FeatureInstance* createInterface(VTable* vtable) = 0;

    void onDumpMemory(FeatureMemoryDump* dump, void* userdata);

    void setEventMember(const char* name, const Member* member) { event_map_[name] = member; }

    const Member* getEventMember(const char* name)
    {
        auto it = event_map_.find(name);
        return it != event_map_.end() ? it->second : nullptr;
    }

#ifdef CONFIG_FEATURE_ENABLE_TRACKER
    FeatureTracker& featureTracker()
    {
        return feature_tracker_;
    }
#endif

protected:
    virtual FeaturePrototype* createInterfacePrototype(const FeatureDescription* description) = 0;

private:
    void* native_ = nullptr;
    const FeatureDescription* description_;
    FeatureManager* feature_manager_ = nullptr;
    FeaturePrototype* module_proto_ = nullptr;
    std::map<const char*, std::unique_ptr<FeaturePrototype>> children_; // all interface instance prototype
    std::vector<FeatureObjectUniquePtr<FeatureInstance>> instances_;
    std::map<std::string, const Member*> event_map_;
#ifdef CONFIG_FEATURE_ENABLE_TRACKER
    FeatureTracker feature_tracker_;
#endif
};

}
#endif // __FEATURE_PROTOTYPE_H__
