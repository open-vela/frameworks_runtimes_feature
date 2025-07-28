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

#include "media_impl.h"
#include "framework/application.h"
#include "framework/base/device_info.h"
#include "framework/utils.h"
#include "gui/gui_wrapper.h"
#include "gui/lvgl/string_to_kid.h"
#include "jidl/media.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

using namespace ferry;
using namespace std;

static const char* file_tag = "[jidl_feature] Media_impl";
static void mediaEventCallback(Widget* w, const ANY* info);

#define MEDIA_STRNCPY(dst, src)                           \
    do {                                                  \
        const char* src_val = src;                        \
        size_t len = strlen(src_val) + 1;                 \
        char* tmp = (char*)FeatureMalloc(len, FT_STRING); \
        strlcpy(tmp, src_val, len);                       \
        dst = tmp;                                        \
    } while (0)

#define MEDIA_FAILED_CALLBACK(feature, info)                                                                          \
    do {                                                                                                              \
        previewImage_finish_callback(feature, info->success, info->fail, info->complete, "previewImage failed", 202); \
        return;                                                                                                       \
    } while (0)

template <typename T>
class FTArrayHelper {
private:
    FtArray* _data;

public:
    FTArrayHelper(FtArray* data)
    {
        _data = data;
    }

    ~FTArrayHelper()
    {
    }

    T& operator[](int32_t index)
    {
        return ((T*)_data->_element)[index];
    }

    int32_t size() const { return _data->_size; }
};
class MediaDsc {
public:
    MediaDsc(IApplication* app, char* shape);
    virtual ~MediaDsc();

    void setEventCallbackId(uint64_t type, int32_t cb);
    int32_t getEventCallbackId(uint64_t type);
    void uninit(void);
    IApplication* getApplication();

    IApplication* app_;
    Widget* media_;
    std::string screenShape_;
    std::map<uint64_t, int32_t> event_;
    FeatureInstanceHandle handle_;
    mediaFinishCallback finishCallback_;
};

MediaDsc::MediaDsc(IApplication* app, char* shape)
    : app_(app)
    , media_(nullptr)
    , screenShape_(shape)
    , handle_(nullptr)
    , finishCallback_(nullptr)
{
}

MediaDsc::~MediaDsc()
{
    uninit();
}

void MediaDsc::uninit(void)
{
    IApplication* app = getApplication();
    if (app) {
        if (media_) {
            media_->bindDom(nullptr);
            media_->destroy(true);
            media_ = nullptr;
            gui_flush(app->widgetContext());
        }

        event_.clear();
    }
}

void MediaDsc::setEventCallbackId(uint64_t type, int32_t cb)
{
    event_.insert(std::make_pair(type, cb));
}

int32_t MediaDsc::getEventCallbackId(uint64_t type)
{
    auto iter = event_.find(type);
    return iter != event_.end() ? iter->second : -1;
}

IApplication* MediaDsc::getApplication()
{
    AIOTJS_CHECK_NE(app_, nullptr);
    return app_;
}

void system_media_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

// Application level, single instance.
void system_media_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureManagerHandle manager_handler = FeatureGetManagerHandleFromProto(handle);
    IApplication* app = static_cast<IApplication*>(FeatureGetManagerUserData(manager_handler, "app"));
    AIOTJS_CHECK_NE(app, nullptr);

    MediaDsc* dsc = new MediaDsc(app, (char*)DeviceInfo::screenShape().c_str());
    FeatureSetProtoData(handle, dsc);
}

void system_media_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_media_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_media_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    MediaDsc* dsc = (MediaDsc*)FeatureGetProtoData(handle);
    if (dsc != nullptr) {
        delete dsc;
        dsc = nullptr;
    }
}

void system_media_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

static void media_previewImage_free(MediaPreviewImageParams* params)
{
    if (params->current) {
        FeatureFreeValue(params->current);
    }
}

void previewImage_finish_callback(void* feature, int32_t success, int32_t fail, int32_t complete, const char* msg, int32_t status)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureInstanceHandle feature_handle = static_cast<FeatureInstanceHandle>(feature);
    FtCallbackId success_id, fail_id, complete_id;

    success_id = success;
    fail_id = fail;
    complete_id = complete;

    if (status == 1 && success_id > 0) {
        if (!FeatureInvokeCallback(feature_handle, success_id)) {
            FEATURE_LOG_ERROR("invoke success callback failed!");
        }
    } else if (status == 202 && fail_id > 0) {
        if (!FeatureInvokeCallback(feature_handle, fail_id, msg, status)) {
            FEATURE_LOG_ERROR("invoke fail callback failed!");
        }
    }
    if (complete_id > 0) {
        if (!FeatureInvokeCallback(feature_handle, complete_id)) {
            FEATURE_LOG_ERROR("invoke complete callback failed!");
        }
    }

    FeatureRemoveCallback(feature_handle, success_id);
    FeatureRemoveCallback(feature_handle, fail_id);
    FeatureRemoveCallback(feature_handle, complete_id);
}

static void create_media_widget_previewImage(MediaPreviewImageParams* params)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    MediaDsc* dsc = (MediaDsc*)FeatureGetProtoData(FeatureGetProtoHandle(params->handle));

    IApplication* app = dsc->app_;
    AIOTJS_CHECK_NE(app, nullptr);
    if (((Application*)app)->stopped()) {
        AIOTJS_LOG_WARN("app state is not running, can't previewImage");
        return;
    }
    if (!app->page()) {
        AIOTJS_LOG_WARN("app->page() is nullptr !");
        return;
    }

    Widget* w = gui_create_widget(app->widgetContext(), "media");
    if (!w) {
        return;
    }
    if (dsc->media_ != nullptr) {
        dsc->uninit();
    }
    dsc->media_ = w;
    w->init();
    w->bindDom((ferry::DomEntity*)dsc, app->page()->uid());

    if (params->current != nullptr) {
        int type = 0;
        char* path = ferry::transformUrlPathHelper(app->packageName(), app->packagePath(), static_cast<const char*>(params->current), &type);
        w->setAttr(ATTR_MEDIA_PREVIEW_CURRENT, path);
        free(path);
    }

    std::string shape = dsc->screenShape_;
    w->setAttr(ATTR_MEDIA_PREVIEW_SCREENSHAPE, shape.c_str());

    w->setEvent(ferry::EVENT_BIT_MEDIA_SUCCESS, mediaEventCallback);
    dsc->setEventCallbackId(ferry::EVENT_BIT_MEDIA_SUCCESS, params->success);
    w->setEvent(ferry::EVENT_BIT_MEDIA_FAIL, mediaEventCallback);
    dsc->setEventCallbackId(ferry::EVENT_BIT_MEDIA_FAIL, params->fail);
    w->setEvent(ferry::EVENT_BIT_MEDIA_COMPLETE, mediaEventCallback);
    dsc->handle_ = params->handle;
    dsc->finishCallback_ = params->finish_callback;

    w->execFunc("show");
    gui_flush(app->widgetContext());
    dsc->finishCallback_(dsc->handle_, -1, -1, params->complete, "PreviewImage complete", 1);
    media_previewImage_free(params);
}

void system_media_wrap_previewImage(FeatureInstanceHandle feature, AppendData append_data, system_media_PreviewImageInfo* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);

    // Missing or invalid key.
    if (info->uris->_size == 0) {
        FEATURE_LOG_ERROR("media previewImage uris is empty!");
        MEDIA_FAILED_CALLBACK(feature, info);
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
    char* final_current = nullptr;
    // Missing or passed as null.
    if (!info->current) {
        FEATURE_LOG_INFO("media previewImage current is empty! Use default values.");
        MEDIA_STRNCPY(final_current, validateUri(uris[0]));
    } else {
        ft_type current_type = ft_get_type(context, *(info->current));
        if (current_type == FT_TYPE_NUMBER) {
            int32_t current_index;
            ft_to_int(context, *info->current, &current_index);
            // Index value out of bounds.
            if (current_index < 0 || current_index >= uris.size()) {
                FEATURE_LOG_ERROR("media previewImage current index error!");
                MEDIA_FAILED_CALLBACK(feature, info);
            }
            MEDIA_STRNCPY(final_current, validateUri(uris[current_index]));
        } else if (current_type == FT_TYPE_STRING) {
            const char* current_str = ft_to_string(context, *(info->current));
            if (strcmp(current_str, "") == 0) {
                // Empty string ('') passed.
                MEDIA_STRNCPY(final_current, validateUri(uris[0]));
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
                    MEDIA_FAILED_CALLBACK(feature, info);
                }
                MEDIA_STRNCPY(final_current, current_str);
            }
            ft_free_string(context, current_str);
        } else {
            FEATURE_LOG_ERROR("media previewImage current type error!");
            MEDIA_FAILED_CALLBACK(feature, info);
        }
    }
    FEATURE_LOG_INFO("media previewImage current is %s", final_current);

    /* todo: pass the URIs array and a number-type current index to the GUI wrapper layer. */
    MediaPreviewImageParams mpi_param;
    mpi_param.handle = feature;
    mpi_param.current = final_current;
    mpi_param.success = info->success;
    mpi_param.fail = info->fail;
    mpi_param.complete = info->complete;
    mpi_param.finish_callback = previewImage_finish_callback;

    create_media_widget_previewImage(&mpi_param);
}

static void mediaEventCallback(Widget* w, const ANY* info)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    if (!w || !info) {
        AIOTJS_LOG_ERROR("Failed to process event callback - invalid parameters");
        return;
    }

    AIOTJS_LOG_DEBUG("Process event callback - %d", info->eventType);

    MediaDsc* dsc = (MediaDsc*)w->getDom();
    if (!dsc) {
        AIOTJS_LOG_INFO("media may destroyed, skip event processing...");
        return;
    }
    Application* app = static_cast<Application*>(dsc->app_);
    if (app == nullptr) {
        return;
    }

    uint64_t event_type = info->eventType;
    int32_t cb = dsc->getEventCallbackId(event_type);
    if (event_type == ferry::EVENT_BIT_MEDIA_SUCCESS) {
        dsc->finishCallback_(dsc->handle_, cb, -1, -1, "PreviewImage succcess", 1);
    } else if (event_type == ferry::EVENT_BIT_MEDIA_FAIL) {
        dsc->finishCallback_(dsc->handle_, -1, cb, -1, "PreviewImage failed", 202);
    }

    // quit
    if (event_type == EVENT_BIT_MEDIA_COMPLETE) {
        if (dsc->media_ != nullptr) {
            dsc->uninit();
        }
    }
}