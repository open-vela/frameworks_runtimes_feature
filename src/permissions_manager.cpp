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

#include "permissions_manager.h"

#include "feature_common.h"
#include "feature_context_private.h"
#include "feature_exports.h"
#include "feature_instance.h"
#include "feature_log.h"
#include "feature_manager.h"
#include "feature_utils.h"

namespace feature_framework {

static const char* get_perms_reject_message(FeaturePermsRejectReason reason)
{
    switch (reason) {
    case FEATURE_PERMS_DENIED:
        return "permissions denied";
    case FEATURE_PERMS_ERROR:
        return "permissions error";
    case FEATURE_PERMS_NO_BG:
        return "permissions no background";
    default:
        return "unknown reason";
    }
}

// PermissionsInfo
PermissionsInfo::PermissionsInfo(FeatureInstance* instance, FeaturePermissionsRequestInfo* info)
    : instance_(instance)
    , adata_(info->adata)
    , api_name_(info->api_name)
    , argc_(info->argc)
    , argv_(nullptr)
    , method_(info->method)
    , permissions_(info->permissions)
    , request_cb_(info->cb)
    , args_buf_(nullptr)
    , extra_argc_(0)
    , fixed_argc_(0)
    , pid_(-1)
    , reject_reason_(FEATURE_PERMS_DENIED)
{
    bool has_rest_params = false;
    int opt_argc = 0;
    int32_t int32_count = 0;
    fixed_argc_ = getParamCount(method_->parameters, &has_rest_params, &opt_argc, &int32_count);
    // optional and rest parameters must not set together.
    FEATURE_CHECK_NE(has_rest_params && opt_argc, true);

    // malloc args buff and argv
    int buf_size = sizeof(uintptr_t) * int32_count;
    if (buf_size > 0) {
        args_buf_ = (uintptr_t*)malloc(buf_size);
        memset(args_buf_, 0, buf_size);
    }
    int args_size = argc_ * sizeof(void*);
    argv_ = (void**)malloc(args_size);
    memset(argv_, 0, args_size);

    // handle promise
    FeatureType ret_type = method_->return_type;
    if (FT_IS_PROMISE(ret_type)) {
        extra_argc_ = 1;
        pid_ = *((FtPromiseId*)(info->argv[0]));
        argv_[0] = &pid_;
    }

    // copy args buff data
    uintptr_t* args_buf = args_buf_;
    for (int i = 0; i < fixed_argc_ - opt_argc; i++) {
        auto param_type = method_->parameters[i];
        // fill args_buf_ into argv_ and go next
        argv_[extra_argc_ + i] = args_buf;
        auto arg = (void**)(info->argv[extra_argc_ + i]);
        int count = getAlignedCount(param_type);
        memcpy(args_buf, arg, count * sizeof(uintptr_t));
        args_buf += count;
    }

    vari_params_.vari_count = 0;
    vari_params_.vari_args = nullptr;
    ft_context_ref ft_ctx = instance_->featureManager()->getFeatureContext();
    if (has_rest_params) {
        // copy vari_params struct
        FtVariParams* fp = (FtVariParams*)(info->argv[fixed_argc_ + extra_argc_]);
        vari_params_.vari_count = fp->vari_count;
        int vp_size = sizeof(ft_value_t) * vari_params_.vari_count;
        vari_params_.vari_args = (ft_value_t*)malloc(vp_size);
        memcpy(vari_params_.vari_args, fp->vari_args, vp_size);
        argv_[fixed_argc_ + extra_argc_] = &vari_params_;
        for (int i = 0; i < fp->vari_count; ++i) {
            ft_dup_value(ft_ctx, fp->vari_args[i]);
        }
    } else if (opt_argc > 0) {
        // copy opt args
        for (int i = fixed_argc_ - opt_argc; i < fixed_argc_; i++) {
            auto param_type = method_->parameters[i];
            argv_[extra_argc_ + i] = args_buf;
            auto arg = (void**)(info->argv[extra_argc_ + i]);
            int count = getAlignedCount(param_type);
            memcpy(args_buf, arg, count * sizeof(uintptr_t));
            args_buf += count;
        }
    }

    // dup params
    for (int i = 0; i < fixed_argc_; i++) {
        auto arg = (void**)(info->argv[extra_argc_ + i]);
        if (arg && *arg && FT_NEED_FREE(method_->parameters[i])) {
            FeatureDupValue(*arg);
            dupFtValue(ft_ctx, method_->parameters[i], arg);
        }
    }

    if (instance_->isBlackListed(info)) {
        int count = getAlignedCount(method_->parameters[0]);
        void* arg = info->argv[extra_argc_];
        memset(arg, 0, count * sizeof(uintptr_t));
    }
}

void PermissionsInfo::releaseArgs()
{
    if (!argv_)
        return;

    ft_context_ref ft_ctx = instance_->featureManager()->getFeatureContext();
    for (int i = 0; i < fixed_argc_; i++) {
        auto arg = (void**)argv_[extra_argc_ + i];
        if (arg && *arg && FT_NEED_FREE(method_->parameters[i])) {
            freeFtValue(ft_ctx, method_->parameters[i], arg);
            FeatureFreeValue(*arg);
        }
    }

    if (vari_params_.vari_args)
        free(vari_params_.vari_args);

    if (args_buf_)
        free(args_buf_);

    free(argv_);
    argv_ = nullptr;
}

PermissionsInfo::~PermissionsInfo()
{
    releaseArgs();
}

void PermissionsInfo::GrantPermissions()
{
    FEATURE_CHECK_NE(request_cb_, nullptr);
    void* ret_value = nullptr;
    FeatureType ret_type = method_->return_type;
    bool is_promise = FT_IS_PROMISE(ret_type);
    if (!is_promise && ret_type != FT_VOID) {
        ALLOCA_PARAM_PTR(ret_type, ret_value);
    }
    request_cb_((FeatureInterfaceHandle)instance_, adata_, argv_, argc_, ret_value);
    if (ret_value && FT_NEED_FREE(ret_type)) {
        FeatureFreeValue(*(void**)ret_value);
    }
    releaseArgs();
}

void PermissionsInfo::RejectPermissions()
{
    FeatureType ret_type = method_->return_type;
    if (FT_IS_PROMISE(ret_type) && pid_ >= 0) {
        FeaturePromiseReject((FeatureInterfaceHandle)instance_, pid_,
            reject_reason_, get_perms_reject_message(reject_reason_));
    } else if (fixed_argc_ > 0) {
        auto arg = (void**)argv_[extra_argc_];
        FtCallbackId fail_id = findCallbackIdByName(method_->parameters[0], arg, "fail");
        if (FeatureCheckCallbackId((FeatureInterfaceHandle)instance_, fail_id)) {
            FeatureInvokeCallback((FeatureInterfaceHandle)instance_,
                fail_id, get_perms_reject_message(reject_reason_), reject_reason_);
            FeatureRemoveCallback((FeatureInterfaceHandle)instance_, fail_id);
        }
        FtCallbackId complete_id = findCallbackIdByName(method_->parameters[0], arg, "complete");
        if (FeatureCheckCallbackId((FeatureInterfaceHandle)instance_, complete_id)) {
            FeatureInvokeCallback((FeatureInterfaceHandle)instance_, complete_id);
            FeatureRemoveCallback((FeatureInterfaceHandle)instance_, complete_id);
        }
    }
    releaseArgs();
}

bool PermissionsInfo::HasAsyncCallbacks()
{
    if (argc_ - extra_argc_ <= 0) {
        return false;
    }
    auto arg = (void*)argv_[extra_argc_];
    if (!arg) {
        return false;
    }
    FtCallbackId success_id = findCallbackIdByName(method_->parameters[0], arg, "success");
    FtCallbackId fail_id = findCallbackIdByName(method_->parameters[0], arg, "fail");
    FtCallbackId complete_id = findCallbackIdByName(method_->parameters[0], arg, "complete");
    return (success_id != 0 || fail_id != 0 || complete_id != 0);
}

// PermissionsManager
PermissionsManager::PermissionsManager()
    : p_cb_(nullptr)
    , p_data_(nullptr)
{
}

PermissionsManager::~PermissionsManager()
{
    ClearAllPermissionsInfo();
}

void PermissionsManager::SetPermissionsCallback(FeaturePermissionsCb cb, void* data)
{
    p_cb_ = cb;
    p_data_ = data;
}

void PermissionsManager::GrantPermissions(PermissionsInfo* info)
{
    auto it = perms_set_.find(info);
    if (it != perms_set_.end()) {
        info->GrantPermissions();
        RemovePermissions(info);
    }
}

void PermissionsManager::RejectPermissions(PermissionsInfo* info)
{
    auto it = perms_set_.find(info);
    if (it != perms_set_.end()) {
        info->RejectPermissions();
        RemovePermissions(info);
    }
}

bool PermissionsManager::CheckPermissions(PermissionsInfo* info)
{
    if (!info)
        return false;

    auto it = perms_set_.find(info);
    if (it == perms_set_.end()) {
        return false;
    }
    return true;
}

bool PermissionsManager::RequestPermissions(PermissionsInfo* info)
{
    if (!p_cb_) {
        FEATURE_LOG_DEBUG("no permissions cb!");
        return false;
    }
    if (perms_set_.find(info) == perms_set_.end()) {
        FEATURE_LOG_DEBUG("cann't find permissions info: %p", info);
        return false;
    }

    FeaturePermissionsInfo perms_info;
    perms_info.api_name = info->ApiName();
    perms_info.permissions = info->Permissions();
    perms_info.has_async_cbs = info->HasAsyncCallbacks();
    p_cb_((FeaturePermissionsHandle)info, &perms_info, p_data_);
    return true;
}

uint64_t PermissionsManager::AddPermissions(PermissionsInfo* info)
{
    perms_set_.insert(info);
    return reinterpret_cast<uint64_t>(info);
}

PermissionsInfo* PermissionsManager::GetPermissions(uint64_t id)
{
    auto info = reinterpret_cast<PermissionsInfo*>(id);
    if (perms_set_.find(info) != perms_set_.end()) {
        return info;
    } else {
        return nullptr;
    }
}

void PermissionsManager::RemovePermissions(PermissionsInfo* info)
{
    auto it = perms_set_.find(info);
    if (it != perms_set_.end()) {
        delete *it;
        perms_set_.erase(it);
    }
}

void PermissionsManager::ClearAllPermissionsInfo()
{
    for (auto it = perms_set_.begin(); it != perms_set_.end(); ++it) {
        delete *it;
    }
    perms_set_.clear();
}

void PermissionsManager::RemoveInstancePermissions(FeatureInstance* instance)
{
    for (auto it = perms_set_.begin(); it != perms_set_.end();) {
        if ((*it)->Instance() == instance) {
            delete *it;
            it = perms_set_.erase(it);
        } else {
            ++it;
        }
    }
}

}
