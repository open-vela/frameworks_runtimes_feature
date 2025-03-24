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

#include "feature_instance.h"
#include "feature_context.h"
#include "feature_log.h"
#include "feature_utils.h"

#include <string.h>
using namespace FEATURE;

namespace feature_framework {

typedef void (*finalizer_func)(FeatureInstance*);

/////////////////////////////////////////////////
FeatureInstance::FeatureInstance(FeaturePrototype* proto, const FeatureDescription* description)
    : instance_id_(-1)
    , is_interface_(0)
    , initialized_(0)
    , detached_(0)
    , vtable_(nullptr)
    , native_(nullptr)
    , proto_(proto)
    , description_(description)
{
    feature_list_initialize(this);
    if (proto_ && proto_->featureManager()) {
        feature_list_add_tail(proto_->featureManager()->getFeatureNodeList(), this);
    }
}

FeatureInstance::FeatureInstance(FeaturePrototype* module_proto, const VTable* vtable, const FeatureDescription* description)
    : instance_id_(-1)
    , is_interface_(vtable ? 1 : 0)
    , initialized_(0)
    , detached_(0)
    , vtable_(vtable)
    , native_(nullptr)
    , proto_(module_proto)
    , description_(description)
{
    feature_list_initialize(this);
    if (proto_ && proto_->featureManager()) {
        feature_list_add_tail(proto_->featureManager()->getFeatureNodeList(), this);
    }
}

FeatureInstance::~FeatureInstance()
{
    if (vtable_ && vtable_->finalizer) {
        finalizer_func finalizer = (finalizer_func)(vtable_->finalizer);
        finalizer(this);
    }

    if (feature_list_in_list(this)) {
        feature_list_delete(this);
    }
}

void FeatureInstance::initialize()
{
    if (initialized_ == 1)
        return;

    auto proto = prototype();
    FEATURE_CHECK_NE(proto, nullptr);
    FeatureObjectUniquePtr<FeatureInstance> interf(this);
    int iid = proto->addInstance(std::move(interf));
    setInstanceId(iid);
    initialized_ = 1;
}

void FeatureInstance::onDumpMemory(FeatureMemoryDump* dump, void* userdata)
{
    dump->count(malloc_size(this), userdata);
}

void FeatureInstance::onDetached()
{
    detached_ = 1;
    auto manager = featureManager();
    FEATURE_CHECK_NE(manager, nullptr);
    manager->permissionsManager().RemoveInstancePermissions(this);
}

static void permision_request_task(int mode, void* data)
{
    if (mode == FEATURE_TASK_MODE_NORMAL) {
        auto info = (PermissionsInfo*)data;
        auto manager = info->Instance()->featureManager();
        manager->permissionsManager().RequestPermissions(info);
    }
}

bool FeatureInstance::requestPermissions(FeaturePermissionsRequestInfo* info)
{
    auto manager = featureManager();
    FEATURE_CHECK_NE(manager, nullptr);
    if (manager->permissionsManager().HasPermissionCb()) {
        PermissionsInfo* perms_info = new PermissionsInfo(this, info);
        manager->permissionsManager().AddPermissions(perms_info);
        manager->addTask((FeatureInstanceHandle)this, permision_request_task, perms_info);
        if (isBlackListed(info)) {
            return false;
        }
        return true;
    }
    FEATURE_LOG_DEBUG("no permissions cb!");
    return false;
}

bool FeatureInstance::isBlackListed(FeaturePermissionsRequestInfo* info)
{
    FEATURE_CHECK_NE(info, nullptr);
    if (strcmp(description_->name, "system.device") == 0
        && strcmp(info->api_name, "getDeviceId") == 0) {
        return true;
    }
    return false;
}
} // namespace feature_framework
