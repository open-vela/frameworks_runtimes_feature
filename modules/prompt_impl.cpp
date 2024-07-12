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

#include "modules/prompt_impl.h"
#include "prompt.h"

static const char* file_tag = "[jidl_feature] Prompt_impl";

void system_prompt_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_prompt_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

// due to onCreate is called before PromptInterfaceHandler is created,
// so promptInit is executed during onRequired, the implementation of quickapp ensures PromptManager is only one globally.
void system_prompt_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PromptInterfaceHandler* pm_hander = static_cast<PromptInterfaceHandler*>(FeatureInstanceGetManagerUserData(handle, "PromptInterfaceHandler"));
    promptInit init = pm_hander->init;
    if (init) {
        init(handle, pm_hander->data);
    }
}

void system_prompt_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PromptInterfaceHandler* pm_hander = static_cast<PromptInterfaceHandler*>(FeatureInstanceGetManagerUserData(handle, "PromptInterfaceHandler"));
    promptCleanOnDetached cleanup = pm_hander->cleanup;
    if (cleanup) {
        cleanup(handle);
    }
}

void system_prompt_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureManagerHandle feature_manager = FeatureGetManagerHandleFromProto(handle);
    PromptInterfaceHandler* pm_hander = static_cast<PromptInterfaceHandler*>(FeatureGetManagerUserData(feature_manager, "PromptInterfaceHandler"));
    promptUninit uninit = pm_hander->uninit;
    if (uninit) {
        uninit(handle);
    }
}

void system_prompt_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_prompt_wrap_showToast(FeatureInstanceHandle feature, AppendData append_data, system_prompt_ToastInfo* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PromptInterfaceHandler* pm_hander = static_cast<PromptInterfaceHandler*>(FeatureInstanceGetManagerUserData(feature, "PromptInterfaceHandler"));
    promptShowToast show_toast = pm_hander->show_toast;
    if (show_toast) {
        show_toast(feature, info->message, info->duration);
    }
}

static char* copyStr(const char* src)
{
    if (src == NULL) {
        return NULL;
    }
    return strdup(src);
}

PromptDialogParams* prompt_dialog_malloc(FeatureInstanceHandle feature)
{
    PromptDialogParams* params = (PromptDialogParams*)malloc(sizeof(PromptDialogParams));
    params->handle = feature;
    params->msg = NULL;
    params->title = NULL;
    params->buttons = NULL;
    params->autocancel = true;
    params->success = -1;
    params->cancel = -1;
    params->complete = -1;
    params->success_cb = NULL;
    params->cancel_cb = NULL;
    params->complete_cb = NULL;
    return params;
}

void prompt_dialog_free(PromptDialogParams* params)
{
    if (params == NULL) {
        return;
    }
    if (params->msg) {
        free(params->msg);
    }
    if (params->title) {
        free(params->title);
    }
    if (params->buttons) {
        free(params->buttons);
    }
    free(params);
    params = NULL;
}

void showDialog_success_cb(FeatureInstanceHandle feature, FtCallbackId success, int index)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    system_prompt_SuccessInfo* success_info = system_promptMallocSuccessInfo();
    success_info->index = index;
    if (!FeatureInvokeCallback(feature, success, success_info)) {
        FEATURE_LOG_ERROR("invoke success callback failed!");
    }
    FeatureRemoveCallback(feature, success);
    FeatureFreeValue(success_info);
}

void showDialog_cancel_cb(FeatureInstanceHandle feature, FtCallbackId cancel)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    if (!FeatureInvokeCallback(feature, cancel)) {
        FEATURE_LOG_ERROR("invoke cancel callback failed!");
    }
    FeatureRemoveCallback(feature, cancel);
}

void showDialog_complete_cb(FeatureInstanceHandle feature, FtCallbackId complete)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    if (!FeatureInvokeCallback(feature, complete)) {
        FEATURE_LOG_ERROR("invoke complete callback failed!");
    }
    FeatureRemoveCallback(feature, complete);
}

void system_prompt_wrap_showDialog(FeatureInstanceHandle feature, AppendData append_data, system_prompt_DialogInfo* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PromptInterfaceHandler* pm_hander = static_cast<PromptInterfaceHandler*>(FeatureInstanceGetManagerUserData(feature, "PromptInterfaceHandler"));
    promptShowDialog show_dialog = pm_hander->show_dialog;
    PromptDialogParams* params = prompt_dialog_malloc(feature);
    if (params == NULL) {
        FEATURE_LOG_ERROR("prompt dialog malloc failed");
        return;
    }
    // TODO: buttons
    params->msg = copyStr(info->message);
    params->title = copyStr(info->title);
    params->autocancel = info->autocancel;
    params->success = info->success;
    params->cancel = info->cancel;
    params->complete = info->complete;
    params->success_cb = showDialog_success_cb;
    params->cancel_cb = showDialog_cancel_cb;
    params->complete_cb = showDialog_complete_cb;
    if (show_dialog) {
        show_dialog(params);
    }
    prompt_dialog_free(params);
}