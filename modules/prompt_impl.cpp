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

#include "prompt.h" // generate by prompt.jidl

#include <cfloat>
#include <map>
#include <math.h>
#include <memory>
#include <string>

#include "feature_exports.h"
#include "feature_log.h"
#include "feature_main_exports.h"
#include "feature_utils.h"

#include "quickapp.h" // The only dependency from quickapp

#include "prompt/prompt.h"
#include "prompt/prompt_dialog.h"
#include "prompt/prompt_server.h"
#include "prompt/prompt_toast.h"
#include "prompt/prompt_utils.h"

#include "ash/memory/weak_ptr.h"

using namespace prompt; // NOLINT

#define PROMPT_DURATION_DEFAULT 1500
#define PROMPT_DURATION_SHORT 2000
#define PROMPT_DURATION_LONG 3500
static const char* file_tag = "[jidl_feature] Prompt_impl";

// A structure to hold callback information on the JS thread.
struct DialogCallbackInfo {
    FeatureInstanceHandle feature;
    FtCallbackId success_id;
    FtCallbackId cancel_id;
    FtCallbackId complete_id;
};

// Data structures for passing data to C-style callbacks for UI thread
struct ToastTaskData {
    std::string msg;
    int duration;
    NativeWidgetHandle root_view;
    PromptServer* prompt_server;
};

class PromptManager;
struct DialogTaskData {
    std::string title;
    std::string message;
    bool autocancel;
    uint32_t dialog_id;
    FeatureInstanceHandle feature_handle;
    NativeWidgetHandle root_view;
    PromptServer* prompt_server;
    ash::WeakPtr<PromptManager> prompt_manager;
};

// Data for the callback from UI thread to App thread (passed via FeaturePost's void* data)
struct FeaturePostEventData {
    uint32_t dialog_id;
    Prompt::EventID event_id;
    int index;
    FeatureInstanceHandle feature_handle; // Crucial for FeaturePost
    ash::WeakPtr<PromptManager> prompt_manager;
};

// Context stored in the prompt's user_data on the UI thread
struct DialogUIContext {
    uint32_t dialog_id;
    PromptServer* prompt_server;
    FeatureInstanceHandle feature_handle;
    ash::WeakPtr<PromptManager> prompt_manager;
};

class FeatureHandle {
public:
    FeatureHandle(FeatureInstanceHandle handle)
        : handle_(handle)
    {
        FeatureDupInstanceHandle(handle_);
    }
    ~FeatureHandle()
    {
        FeatureFreeInstanceHandle(handle_);
    }
    FeatureHandle(const FeatureHandle& rhs)
    {
        handle_ = rhs.handle_;
        FeatureDupInstanceHandle(handle_);
    }
    FeatureHandle& operator=(const FeatureHandle& rhs)
    {
        if (this == &rhs) {
            return *this;
        }
        FeatureFreeInstanceHandle(handle_);
        handle_ = rhs.handle_;
        FeatureDupInstanceHandle(handle_);
    }
    FeatureHandle(FeatureHandle&& rhs)
    {
        handle_ = rhs.handle_;
        rhs.handle_ = nullptr;
    }
    FeatureHandle& operator=(FeatureHandle&& rhs)
    {
        FeatureFreeInstanceHandle(handle_);
        handle_ = rhs.handle_;
        rhs.handle_ = nullptr;
        return *this;
    }
    FeatureInstanceHandle getFeatureHandle() { return handle_; }

private:
    FeatureInstanceHandle handle_;
};

class PromptManager : public ash::SupportsWeakPtr<PromptManager>, public FeatureHandle {
public:
    PromptManager(QApplicationHandle app_handle, FeatureInstanceHandle handle)
        : SupportsWeakPtr<PromptManager>(this)
        , FeatureHandle(handle)
        , app_handle_(app_handle)
        , next_dialog_id_(0)
        , dialog_callbacks_()
        , prompt_server_(std::make_unique<PromptServer>())
    {
        FEATURE_CHECK(app_handle_, "app is null");
        QApplicationPostUITask(app_handle, &PromptServer::InitServerTask, nullptr, prompt_server_.get());
    }

    ~PromptManager()
    {
        FEATURE_LOG_INFO("~PromptManager() BEGIN, this=%p, prompt_server_=%p", this, prompt_server_.get());
        QApplicationPostUITask(app_handle_, nullptr, &PromptServer::DestroyServerTask, prompt_server_.release());
    }

    void ShowToast(const std::string& msg, int duration)
    {
        auto data = std::make_unique<ToastTaskData>();
        data->msg = msg;
        data->duration = duration;
        data->root_view = QApplicationGetGUIRoot(app_handle_);
        data->prompt_server = prompt_server_.get();

        QApplicationPostUITask(
            app_handle_, &PromptManager::ShowToastTask, [](void* user_data) {
                ToastTaskData* ptr = static_cast<ToastTaskData*>(user_data);
                std::unique_ptr<ToastTaskData> del(ptr);
            },
            data.release());
    }

    void ShowDialog(FeatureInstanceHandle feature, const std::string& title, const std::string& message,
        bool autocancel, FtCallbackId success_id, FtCallbackId cancel_id,
        FtCallbackId complete_id)
    {
        uint32_t dialog_id = next_dialog_id_++;
        // Make sure to dup handle here if it's ref-counted and will be used asynchronously
        // For now, assuming FeaturePost keeps it alive or it's raw handle
        auto cb_info = new DialogCallbackInfo { feature, success_id, cancel_id, complete_id };
        dialog_callbacks_[dialog_id] = cb_info;

        auto data = std::make_unique<DialogTaskData>();
        data->title = title;
        data->message = message;
        data->autocancel = autocancel;
        data->dialog_id = dialog_id;
        data->root_view = QApplicationGetGUIRoot(app_handle_);
        data->prompt_server = prompt_server_.get();
        data->feature_handle = feature;
        data->prompt_manager = AsWeakPtr();

        QApplicationPostUITask(
            app_handle_, &PromptManager::ShowDialogTask, [](void* user_data) {
                // use unique ptr to manage the memory
                std::unique_ptr<DialogTaskData> del(static_cast<DialogTaskData*>(user_data));
            },
            data.release());
    }

    DialogCallbackInfo* findDialogInfo(uint32_t dialog_id)
    {
        if (auto it = dialog_callbacks_.find(dialog_id); it != dialog_callbacks_.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool eraseDialogInfo(uint32_t dialog_id)
    {
        return dialog_callbacks_.erase(dialog_id) > 0;
    }

private:
    static void ShowToastTask(void* user_data)
    {
        ToastTaskData* data = static_cast<ToastTaskData*>(user_data);

        auto toast = prompt::CreatePrompt(prompt::Prompt::TYPE_TOAST, reinterpret_cast<lv_obj_t*>(data->root_view));
        toast->setAttr(Prompt::ID_MESSAGE, data->msg.c_str());
        toast->setAttr(Prompt::ID_DURATION, data->duration);
        auto toast_ptr = toast.release();
        if (!data->prompt_server->push(toast_ptr)) {
            toast.reset(toast_ptr); // for auto delete
        }
    }

    static void ShowDialogTask(void* user_data)
    {
        DialogTaskData* data = static_cast<DialogTaskData*>(user_data);
        auto dialog = prompt::CreatePrompt(prompt::Prompt::TYPE_DIALOG, reinterpret_cast<lv_obj_t*>(data->root_view));

        dialog->setAttr(Prompt::ID_TITLE, data->title.c_str());
        dialog->setAttr(Prompt::ID_MESSAGE, data->message.c_str());
        dialog->setAttr(Prompt::ID_AUTOCANCEL, (int32_t)data->autocancel);

        // Store FeatureInstanceHandle and dialog_id together for UI -> App callback
        auto ui_context = std::make_unique<DialogUIContext>();
        ui_context->dialog_id = data->dialog_id;
        ui_context->prompt_server = data->prompt_server;
        ui_context->feature_handle = data->feature_handle;
        ui_context->prompt_manager = data->prompt_manager;

        dialog->setEventCB(PromptManager::DialogEventCallback, ui_context.release());
        data->prompt_server->push(dialog.release());
    }

    // This is the static callback that will be executed on the UI thread when a Prompt event occurs.
    static void DialogEventCallback(Prompt::EventID id, Prompt::EventInfo* info)
    {
        if (!info)
            return;

        DialogUIContext* ui_context = static_cast<DialogUIContext*>(info->user_data);
        if (!ui_context) {
            FEATURE_LOG_ERROR("DialogEventCallback: DialogUIContext not found in Prompt user data.");
            return;
        }

        std::unique_ptr<FeaturePostEventData> data_for_app_thread = std::make_unique<FeaturePostEventData>();

        data_for_app_thread->dialog_id = ui_context->dialog_id;
        data_for_app_thread->event_id = id;
        data_for_app_thread->index = info->index;
        data_for_app_thread->feature_handle = ui_context->feature_handle;
        data_for_app_thread->prompt_manager = ui_context->prompt_manager;

        // it is used to check 'ui_context->feature_handle'.
        // feature_handle is ownered by prompt_manager.
        if (ui_context->prompt_manager.Get()) {
            // Post this event back to the App/JS thread using FeaturePost
            FeaturePostEventData* data = data_for_app_thread.release();
            bool ret = FeaturePost(ui_context->feature_handle, &PromptManager::HandleDialogEventOnAppThread, data);
            if (!ret) {
                std::unique_ptr<FeaturePostEventData> del(data);
                FEATURE_LOG_ERROR("DialogEventCallback: Failed to post event to App/JS thread.");
            }
        }

        // If this is the final event, clean up the UI context attached to the prompt.
        if (id == Prompt::EVENT_ID_COMPLETE) {
            std::unique_ptr<DialogUIContext> del(ui_context);
        }
    }

    // This function is executed on the App/JS thread by FeaturePost.
    static void HandleDialogEventOnAppThread(int status, void* user_data)
    {
        FeaturePostEventData* data = static_cast<FeaturePostEventData*>(user_data);
        if (!data)
            return;

        std::unique_ptr<FeaturePostEventData> event_data(data);
        if (status == FEATURE_TASK_MODE_FREE) { // Check status for FeaturePost success/failure (not usually used this way, but good practice)
            FEATURE_LOG_WARN("HandleDialogEventOnAppThread: FeaturePost status indicates an issue (%d).", status);
            return;
        }

        auto prompt_manager = event_data->prompt_manager.Get();
        if (!prompt_manager) {
            FEATURE_LOG_ERROR("HandleDialogEventOnAppThread: PromptManager is no longer valid.");
            return;
        }

        auto cb_info = prompt_manager->findDialogInfo(event_data->dialog_id);

        if (!cb_info) {
            FEATURE_LOG_ERROR("HandleDialogEventOnAppThread: DialogCallbackInfo not found for dialog_id %u.", event_data->dialog_id);
            return;
        }

        switch (event_data->event_id) {
        case Prompt::EVENT_ID_SUCCESS:
            if (cb_info->success_id >= 0) {
                system_prompt_SuccessInfo* success_info = system_promptMallocSuccessInfo();
                success_info->index = event_data->index;
                FeatureInvokeCallback(cb_info->feature, cb_info->success_id, success_info);
                FeatureFreeValue(success_info);
            }
            break;
        case Prompt::EVENT_ID_CANCEL:
            if (cb_info->cancel_id >= 0) {
                FeatureInvokeCallback(cb_info->feature, cb_info->cancel_id);
            }
            break;
        case Prompt::EVENT_ID_COMPLETE:
            if (cb_info->complete_id >= 0) {
                FeatureInvokeCallback(cb_info->feature, cb_info->complete_id);
            }
            delete cb_info; // DialogCallbackInfo is consumed
            prompt_manager->eraseDialogInfo(event_data->dialog_id);
            break;
        default:
            break;
        }
    }

    QApplicationHandle app_handle_;
    uint32_t next_dialog_id_;
    std::map<uint32_t, DialogCallbackInfo*> dialog_callbacks_;

    std::unique_ptr<PromptServer> prompt_server_;
};

// JIDL function implementations

void system_prompt_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_prompt_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_prompt_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FeatureManagerHandle feature_manager = FeatureGetManagerHandleFromInstance(handle);
    QApplicationHandle app = static_cast<QApplicationHandle>(FeatureGetManagerUserData(feature_manager, "app"));

    auto prompt_manager = std::make_unique<PromptManager>(app, handle);
    FeatureSetObjectData(handle, prompt_manager.release());
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_prompt_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    auto prompt_manager = static_cast<PromptManager*>(FeatureGetObjectData(handle));

    std::unique_ptr<PromptManager> del(prompt_manager);
}

void system_prompt_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_prompt_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_prompt_wrap_showToast(FeatureInstanceHandle feature, AppendData append_data, system_prompt_ToastInfo* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    auto prompt_manager = static_cast<PromptManager*>(FeatureGetObjectData(feature));
    if (!prompt_manager) {
        FEATURE_LOG_WARN("PromptManager is not available, toast will not be shown.");
        return;
    }

    if (info->message == nullptr || *(info->message) == '\0') {
        FEATURE_LOG_ERROR("Toast message is null or empty!");
        return;
    }

    double duration_ms = PROMPT_DURATION_DEFAULT;
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    if (info->duration && ft_to_double(ft_ctx, *info->duration, &duration_ms)) {
        if (fabs(duration_ms - 0.0) < DBL_EPSILON) {
            duration_ms = PROMPT_DURATION_SHORT;
        } else if (fabs(duration_ms - 1.0) < DBL_EPSILON) {
            duration_ms = PROMPT_DURATION_LONG;
        }
    }
    prompt_manager->ShowToast(info->message, static_cast<int>(duration_ms));
}

void system_prompt_wrap_showDialog(FeatureInstanceHandle feature, AppendData append_data, system_prompt_DialogInfo* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    auto prompt_manager = static_cast<PromptManager*>(FeatureGetObjectData(feature));
    if (!prompt_manager) {
        FEATURE_LOG_WARN("PromptManager is not available, dialog will not be shown.");
        return;
    }

    prompt_manager->ShowDialog(
        feature,
        info->title ? info->title : "",
        info->message ? info->message : "",
        info->autocancel,
        info->success,
        info->cancel,
        info->complete);
}