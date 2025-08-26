/*
 * Copyright (C) 2025 Xiaomi Corporation. All rights reserved.
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
 *
 */

#include "ash/device_info/device_info.h"
#include "ash/logging/logging.h"
#include "ash/memory/weak_ptr.h"
#include "ash/message_loop/message_loop.h"
#include "framework/application.h"
#include "framework/utils.h"
#include "gui/gui_wrapper.h"
#include "gui/lvgl/string_to_kid.h"
#include "jidl/media.h"
#include "modules/modules_utils.h"

#include <map>
#include <string>
#include <unistd.h>

static const char* file_tag = "[jidl_feature] Media_impl";

struct WidgetDeleter {
    void operator()(ferry::Widget* widget) { widget->destroy(true); }
};

using WidgetPtr = std::unique_ptr<ferry::Widget, WidgetDeleter>;

class MediaManagerOnUI;

class PreviewImageInfo {
public:
    PreviewImageInfo(std::function<void()> success_cb,
        std::function<void()> fail_cb,
        std::function<void()> complete_cb,
        FtCallbackId success_callback_id,
        FtCallbackId fail_callback_id,
        FtCallbackId complete_callback_id,
        FeatureInstanceHandle feature);
    ~PreviewImageInfo();

    void Success();
    void Fail();
    void Complete();

private:
    std::function<void()> success_cb_;
    std::function<void()> fail_cb_;
    std::function<void()> complete_cb_;
    FtCallbackId success_callback_id_;
    FtCallbackId fail_callback_id_;
    FtCallbackId complete_callback_id_;
    FeatureInstanceHandle feature_;
};

class MediaManager : public ash::SupportsWeakPtr<MediaManager> {
public:
    MediaManager(FeatureInstanceHandle feature, ferry::Application* application);
    ~MediaManager();

    void PreviewImage(const std::string& current,
        std::function<void()> success_cb,
        std::function<void()> fail_cb,
        std::function<void()> complete_cb,
        FtCallbackId success_callback_id,
        FtCallbackId fail_callback_id,
        FtCallbackId complete_callback_id,
        FeatureInstanceHandle feature);

private:
    void PreviewImageSuccess(uint32_t preview_image_id);
    void PreviewImageFail(uint32_t preview_image_id);
    void PreviewImageComplete(uint32_t preview_image_id);

    uint32_t AllocPreviewImageId();

    FeatureInstanceHandle feature_;
    ferry::Application* application_;
    std::unique_ptr<MediaManagerOnUI> media_manager_on_ui_;
    std::map<uint32_t, std::unique_ptr<PreviewImageInfo>> pending_preview_images_;
    uint32_t next_preview_image_id_;
    std::string screen_shape_;

    friend class MediaManagerOnUI;
};

class PreviewImageInfoOnUI {
public:
    PreviewImageInfoOnUI(MediaManagerOnUI* media_manager,
        uint32_t preview_image_id,
        WidgetPtr widget);
    ~PreviewImageInfoOnUI() = default;

    MediaManagerOnUI* media_manager();
    uint32_t preview_image_id();
    ferry::Widget* widget();

private:
    MediaManagerOnUI* media_manager_;
    uint32_t preview_image_id_;
    WidgetPtr widget_;
};

class MediaManagerOnUI : public ash::SupportsWeakPtr<MediaManagerOnUI> {
public:
    MediaManagerOnUI(ash::WeakPtr<MediaManager> media_manager,
        std::shared_ptr<ash::TaskRunner> media_manager_runner,
        AIOTJS::WidgetContextHandle widget_context);
    ~MediaManagerOnUI() = default;

    void PreviewImage(uint32_t id, const std::string& path, const std::string& shape);

private:
    void PreviewImageSuccess(uint32_t preview_image_id);
    void PreviewImageFail(uint32_t preview_image_id);
    void PreviewImageComplete(uint32_t preview_image_id);

    static void PreviewImageSuccessCB(ferry::Widget* w, const ferry::ANY* info);
    static void PreviewImageFailCB(ferry::Widget* w, const ferry::ANY* info);
    static void PreviewImageCompleteCB(ferry::Widget* w, const ferry::ANY* info);

    ash::WeakPtr<MediaManager> media_manager_;
    std::shared_ptr<ash::TaskRunner> media_manager_runner_;
    AIOTJS::WidgetContextHandle widget_context_;
    std::map<uint32_t, std::unique_ptr<PreviewImageInfoOnUI>> pending_preview_images_;
};

PreviewImageInfo::PreviewImageInfo(std::function<void()> success_cb,
    std::function<void()> fail_cb,
    std::function<void()> complete_cb,
    FtCallbackId success_callback_id,
    FtCallbackId fail_callback_id,
    FtCallbackId complete_callback_id,
    FeatureInstanceHandle feature)
    : success_cb_(std::move(success_cb))
    , fail_cb_(std::move(fail_cb))
    , complete_cb_(std::move(complete_cb))
    , success_callback_id_(success_callback_id)
    , fail_callback_id_(fail_callback_id)
    , complete_callback_id_(complete_callback_id)
    , feature_(feature)
{
}

PreviewImageInfo::~PreviewImageInfo()
{
    FeatureRemoveCallback(feature_, success_callback_id_);
    FeatureRemoveCallback(feature_, fail_callback_id_);
    FeatureRemoveCallback(feature_, complete_callback_id_);
}

void PreviewImageInfo::Success()
{
    if (success_cb_)
        success_cb_();
}

void PreviewImageInfo::Fail()
{
    if (fail_cb_)
        fail_cb_();
}

void PreviewImageInfo::Complete()
{
    if (complete_cb_)
        complete_cb_();
}

MediaManager::MediaManager(FeatureInstanceHandle feature, ferry::Application* application)
    : SupportsWeakPtr<MediaManager>(this)
    , feature_(feature)
    , application_(application)
    , media_manager_on_ui_(std::make_unique<MediaManagerOnUI>(
          AsWeakPtr(),
          ash::MessageLoop::Current()->GetTaskRunner(),
          application->widgetContext()))
    , next_preview_image_id_(0)
    , screen_shape_(ash::DeviceInfo::screenShape())
{
}

MediaManager::~MediaManager()
{
    // use taskrunner to ensure that the task will be executed eventually
    application_->getUITaskRunner()->PostTask([mm_on_ui = std::move(media_manager_on_ui_)]() {
    });
}

void MediaManager::PreviewImage(const std::string& current,
    std::function<void()> success_cb,
    std::function<void()> fail_cb,
    std::function<void()> complete_cb,
    FtCallbackId success_callback_id,
    FtCallbackId fail_callback_id,
    FtCallbackId complete_callback_id,
    FeatureInstanceHandle feature)
{
    uint32_t id = AllocPreviewImageId();
    pending_preview_images_.emplace(id,
        std::make_unique<PreviewImageInfo>(
            std::move(success_cb),
            std::move(fail_cb),
            std::move(complete_cb),
            success_callback_id,
            fail_callback_id,
            complete_callback_id,
            feature));
    int type = 0;
    char* path = ferry::transformUrlPathHelper(application_->packageName(), application_->packagePath(), current.c_str(), &type);
    application_->postUITask([mm_on_ui = media_manager_on_ui_.get(), id,
                                 path, shape = screen_shape_]() {
        mm_on_ui->PreviewImage(id, path, shape);
    });
    free(path);
}

void MediaManager::PreviewImageSuccess(uint32_t preview_image_id)
{
    auto it = pending_preview_images_.find(preview_image_id);
    ASH_DCHECK(it != pending_preview_images_.end());
    it->second->Success();
}

void MediaManager::PreviewImageFail(uint32_t preview_image_id)
{
    auto it = pending_preview_images_.find(preview_image_id);
    ASH_DCHECK(it != pending_preview_images_.end());
    it->second->Fail();
}

void MediaManager::PreviewImageComplete(uint32_t preview_image_id)
{
    auto it = pending_preview_images_.find(preview_image_id);
    ASH_DCHECK(it != pending_preview_images_.end());
    it->second->Complete();

    pending_preview_images_.erase(it);
}

uint32_t MediaManager::AllocPreviewImageId()
{
    uint32_t id = next_preview_image_id_++;
    while (pending_preview_images_.find(id) != pending_preview_images_.end()) {
        ++id;
    }
    return id;
}

PreviewImageInfoOnUI::PreviewImageInfoOnUI(MediaManagerOnUI* media_manager,
    uint32_t preview_image_id,
    WidgetPtr widget)
    : media_manager_(media_manager)
    , preview_image_id_(preview_image_id)
    , widget_(std::move(widget))
{
}

MediaManagerOnUI* PreviewImageInfoOnUI::media_manager()
{
    return media_manager_;
}

uint32_t PreviewImageInfoOnUI::preview_image_id()
{
    return preview_image_id_;
}

ferry::Widget* PreviewImageInfoOnUI::widget()
{
    return widget_.get();
}

MediaManagerOnUI::MediaManagerOnUI(ash::WeakPtr<MediaManager> media_manager,
    std::shared_ptr<ash::TaskRunner> media_manager_runner,
    AIOTJS::WidgetContextHandle widget_context)
    : SupportsWeakPtr(this)
    , media_manager_(std::move(media_manager))
    , media_manager_runner_(std::move(media_manager_runner))
    , widget_context_(widget_context)
{
}

void MediaManagerOnUI::PreviewImage(uint32_t id, const std::string& path, const std::string& shape)
{
    WidgetPtr w(gui_create_widget(widget_context_, "media"));
    ASH_DCHECK(w.get());
    w->init();

    if (!path.empty())
        w->setAttr(ATTR_MEDIA_PREVIEW_CURRENT, path.c_str());
    if (!shape.empty())
        w->setAttr(ATTR_MEDIA_PREVIEW_SCREENSHAPE, shape.c_str());
    w->setEvent(ferry::EVENT_BIT_MEDIA_SUCCESS, PreviewImageSuccessCB);
    w->setEvent(ferry::EVENT_BIT_MEDIA_FAIL, PreviewImageFailCB);
    w->setEvent(ferry::EVENT_BIT_MEDIA_COMPLETE, PreviewImageCompleteCB);

    w->execFunc("show");

    std::unique_ptr<PreviewImageInfoOnUI> preview_image_info = std::make_unique<PreviewImageInfoOnUI>(this, id, std::move(w));

    preview_image_info->widget()->bindUserData(preview_image_info.get());

    pending_preview_images_.emplace(id, std::move(preview_image_info));
}

void MediaManagerOnUI::PreviewImageSuccess(uint32_t preview_image_id)
{
    ASH_DCHECK(pending_preview_images_.find(preview_image_id) != pending_preview_images_.end());
    media_manager_runner_->PostTask([weak_mm = media_manager_, preview_image_id]() {
        MediaManager* mm = weak_mm.Get();
        if (!mm)
            return;
        mm->PreviewImageSuccess(preview_image_id);
    });
}

void MediaManagerOnUI::PreviewImageFail(uint32_t preview_image_id)
{
    ASH_DCHECK(pending_preview_images_.find(preview_image_id) != pending_preview_images_.end());
    media_manager_runner_->PostTask([weak_mm = media_manager_, preview_image_id]() {
        MediaManager* mm = weak_mm.Get();
        if (!mm)
            return;
        mm->PreviewImageFail(preview_image_id);
    });
}

void MediaManagerOnUI::PreviewImageComplete(uint32_t preview_image_id)
{
    ASH_DCHECK(pending_preview_images_.find(preview_image_id) != pending_preview_images_.end());
    media_manager_runner_->PostTask([weak_mm = media_manager_, preview_image_id]() {
        MediaManager* mm = weak_mm.Get();
        if (!mm)
            return;
        mm->PreviewImageComplete(preview_image_id);
    });

    ash::MessageLoop::Current()->GetTaskRunner()->PostTask([weak = AsWeakPtr(), preview_image_id, this]() {
        auto mm_ui = weak.Get();
        if (!mm_ui) {
            return;
        }
        pending_preview_images_.erase(preview_image_id);
    });
}

void MediaManagerOnUI::PreviewImageSuccessCB(ferry::Widget* w, const ferry::ANY* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PreviewImageInfoOnUI* preview_image_info = reinterpret_cast<PreviewImageInfoOnUI*>(w->getUserData());
    preview_image_info->media_manager()->PreviewImageSuccess(preview_image_info->preview_image_id());
}

void MediaManagerOnUI::PreviewImageFailCB(ferry::Widget* w, const ferry::ANY* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PreviewImageInfoOnUI* preview_image_info = reinterpret_cast<PreviewImageInfoOnUI*>(w->getUserData());
    preview_image_info->media_manager()->PreviewImageFail(preview_image_info->preview_image_id());
}

void MediaManagerOnUI::PreviewImageCompleteCB(ferry::Widget* w, const ferry::ANY* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    PreviewImageInfoOnUI* preview_image_info = reinterpret_cast<PreviewImageInfoOnUI*>(w->getUserData());
    preview_image_info->media_manager()->PreviewImageComplete(preview_image_info->preview_image_id());
}

void system_media_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_media_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_media_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureManagerHandle manager_handler = FeatureGetManagerHandleFromInstance(handle);
    ferry::Application* app = static_cast<ferry::Application*>(FeatureGetManagerUserData(manager_handler, "app"));
    ASH_DCHECK_NE(app, nullptr);

    std::unique_ptr<MediaManager> media_manager = std::make_unique<MediaManager>(handle, app);
    FeatureSetObjectData(handle, media_manager.release());
}

void system_media_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    MediaManager* media_manager = static_cast<MediaManager*>(FeatureGetObjectData(handle));
    ASH_DCHECK_NE(media_manager, nullptr);
    std::unique_ptr<MediaManager> holder(media_manager);
}

void system_media_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_media_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

static void media_fail_callback(FeatureInstanceHandle feature, system_media_PreviewImageInfo* info)
{
    if (!FeatureInvokeCallback(feature, info->fail, "previewImage failed", 202)) {
        FEATURE_LOG_ERROR("invoke fail callback failed!");
    }
    FeatureRemoveCallback(feature, info->success);
    FeatureRemoveCallback(feature, info->fail);
    FeatureRemoveCallback(feature, info->complete);
}

void system_media_wrap_previewImage(FeatureInstanceHandle feature, AppendData append_data, system_media_PreviewImageInfo* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);

    // Missing or invalid key.
    if (info->uris->_size == 0) {
        FEATURE_LOG_ERROR("media previewImage uris is empty!");
        media_fail_callback(feature, info);
        return;
    }

    FTArrayHelper<const char*> uris(info->uris);
    // Non-string types or null elements in array, display the default placeholder image.
    auto validateUri = [=](const char* uri) -> const char* {
        if (uri == nullptr) {
            FEATURE_LOG_INFO("media previewImage uris element error! Display the default placeholder image.");
            return "valError";
        }
        return uri;
    };

    ft_context_ref context = FeatureGetContext(feature);
    std::string final_current;
    // Missing or passed as null.
    if (!info->current) {
        FEATURE_LOG_INFO("media previewImage current is empty! Use default values.");
        final_current = validateUri(uris[0]);
    } else {
        ft_type current_type = ft_get_type(context, *(info->current));
        if (current_type == FT_TYPE_NUMBER) {
            int32_t current_index;
            ft_to_int(context, *info->current, &current_index);
            // Index value out of bounds.
            if (current_index < 0 || current_index >= uris.size()) {
                FEATURE_LOG_ERROR("media previewImage current index error!");
                media_fail_callback(feature, info);
                return;
            }
            final_current = validateUri(uris[current_index]);
        } else if (current_type == FT_TYPE_STRING) {
            const char* current_str = ft_to_string(context, *(info->current));
            if (strcmp(current_str, "") == 0) {
                // Empty string ('') passed.
                final_current = validateUri(uris[0]);
            } else {
                bool found = false;
                for (int32_t i = 0; i < uris.size(); i++) {
                    if (strcmp(current_str, uris[i]) == 0) {
                        found = true;
                        break;
                    }
                }
                // String type but unmatched in URIS array.
                if (!found) {
                    FEATURE_LOG_ERROR("media previewImage current is not in uris!");
                    ft_free_string(context, current_str);
                    media_fail_callback(feature, info);
                    return;
                }
                final_current = current_str;
            }
            ft_free_string(context, current_str);
        } else {
            FEATURE_LOG_ERROR("media previewImage current type error!");
            media_fail_callback(feature, info);
            return;
        }
    }
    FEATURE_LOG_INFO("media previewImage current is %s", final_current.c_str());

    /* todo: pass the URIs array and a number-type current index to the GUI wrapper layer. */
    MediaManager* media_manager = reinterpret_cast<MediaManager*>(FeatureGetObjectData(feature));
    ASH_DCHECK_NE(media_manager, nullptr);
    media_manager->PreviewImage(
        final_current,
        [feature, id = info->success]() {
            if (!FeatureInvokeCallback(feature, id))
                FEATURE_LOG_ERROR("%s::invoke success callback failed!", file_tag);
        },
        [feature, id = info->fail]() {
            if (!FeatureInvokeCallback(feature, id))
                FEATURE_LOG_ERROR("%s::invoke fail callback failed!", file_tag, "previewImage failed", 202);
        },
        [feature, id = info->complete]() {
            if (!FeatureInvokeCallback(feature, id))
                FEATURE_LOG_ERROR("%s::invoke complete callback failed!", file_tag);
        },
        info->success, info->fail, info->complete, feature);
}
