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

#ifndef __PERMISSIONS_MANAGER_H__
#define __PERMISSIONS_MANAGER_H__

#include "feature_description.h"
#include "feature_main_exports.h"

#include <set>

namespace feature_framework {

class FeatureInstance;
class FeatureManager;

class PermissionsInfo {
public:
    PermissionsInfo(FeatureInstance* instance, FeaturePermissionsRequestInfo* info);
    ~PermissionsInfo();
    FeatureInstance* Instance() { return instance_; }
    const MemberMethod* Method() { return method_; }
    const FeaturePermissions* Permissions() { return permissions_; }
    const char* ApiName() { return api_name_; }
    void GrantPermissions();
    void RejectPermissions();
    void SetRejectReason(FeaturePermsRejectReason reason) { reject_reason_ = reason; }
    int RejectReason() { return reject_reason_; }
    bool HasAsyncCallbacks();

private:
    void releaseArgs();

    FeatureInstance* instance_;
    AppendData adata_;
    const char* api_name_;
    int argc_;
    void** argv_;
    const MemberMethod* method_;
    const FeaturePermissions* permissions_;
    FeaturePermissionsRequestCb request_cb_;
    uintptr_t* args_buf_;
    FtVariParams vari_params_;
    int extra_argc_;
    int fixed_argc_;
    FtPromiseId pid_;
    FeaturePermsRejectReason reject_reason_;
};

class PermissionsManager {
public:
    PermissionsManager();
    ~PermissionsManager();
    void SetPermissionsCallback(FeaturePermissionsCb cb, void* data);
    bool HasPermissionCb() { return p_cb_ != nullptr; }
    void GrantPermissions(PermissionsInfo* info);
    void RejectPermissions(PermissionsInfo* info);
    bool CheckPermissions(PermissionsInfo* info);
    bool RequestPermissions(PermissionsInfo* info);
    void RemoveInstancePermissions(FeatureInstance* instance);
    uint64_t AddPermissions(PermissionsInfo* info);
    PermissionsInfo* GetPermissions(uint64_t id);
    void RemovePermissions(PermissionsInfo* info);

private:
    void ClearAllPermissionsInfo();

    FeaturePermissionsCb p_cb_;
    void* p_data_;
    std::set<PermissionsInfo*> perms_set_;
};

}
#endif // __PERMISSIONS_MANAGER_H__