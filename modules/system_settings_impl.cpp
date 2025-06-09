/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#include "system_settings.h"

#include <cassert>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef __NuttX__
#include <nuttx/config.h>
#endif

#if defined(CONFIG_APP_CAR_CONTROL) && defined(CONFIG_CAR_APP_SETTINGS)
#include "miwear_settings.h"
#elif defined(CONFIG_APP_CAR_SETTINGS_MOCK)
#include "system_settings/miwear_settings_mock.h"
#endif

#define TAG "[system_settings]"
#define SETTINGS_DEBUG(fmt, ...) FEATURE_LOG_DEBUG(TAG fmt, ##__VA_ARGS__)
#define SETTINGS_INFO(fmt, ...) FEATURE_LOG_INFO(TAG fmt, ##__VA_ARGS__)
#define SETTINGS_WARN(fmt, ...) FEATURE_LOG_WARN(TAG fmt, ##__VA_ARGS__)
#define SETTINGS_ERROR(fmt, ...) FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

namespace system_settings {
class Status {
public:
    Status()
        : code_(0)
    {
    }
    Status(const Status&) = default;
    Status& operator=(const Status&) = default;
    Status(Status&&) = default;
    Status& operator=(Status&&) = default;
    bool ok() const
    {
        return code_ == 0;
    }
    int32_t code() const
    {
        return code_;
    }

    const std::string& err_msg() const
    {
        return err_msg_;
    }

    static Status Ok()
    {
        return Status();
    }
    static Status ArgsError(const std::string err_msg)
    {
        return Status(FT_ERR_ARGS, std::move(err_msg));
    }
    static Status GeneralError(const std::string err_msg)
    {
        return Status(FT_ERR_GENERAL, std::move(err_msg));
    }

private:
    Status(int32_t err_code, std::string msg)
        : code_(err_code)
        , err_msg_(std::move(msg))
    {
    }
    int32_t code_;
    std::string err_msg_;
};

// This is an Settings interface class.
// Different platforms could implement distinct subclasses.
class Settings {
public:
    Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    virtual Status Init() = 0;
    virtual Status SetProp(std::string_view key, std::string_view value, FeatureInstanceHandle handle, FtPromiseId pid) = 0;
    virtual Status GetProp(std::string_view key, FeatureInstanceHandle handle, FtPromiseId pid) = 0;
    virtual Status SubscribeProp(std::string_view key, FeatureInstanceHandle handle, FtCallbackId cid) = 0;
    virtual Status UnsubscribeProp(std::string_view key, FeatureInstanceHandle handle) = 0;

    // Destroy all subscriptions and set/get requests associated with this feature.
    virtual void Detach(FeatureInstanceHandle handle) = 0;

    // For cross-thread ProcessMessage
    virtual void ProcessMessageAsync(miwear_settings_message_t* msg, void* arg) = 0;
    virtual void Destroy() = 0;
    virtual ~Settings() = default;
};

miwear_settings_message_t* CloneMiwearSettingsMessage(miwear_settings_message_t* msg)
{
    assert(msg);
    auto* new_msg = static_cast<miwear_settings_message_t*>(malloc(sizeof(miwear_settings_message_t)));
    memcpy(new_msg, msg, sizeof(miwear_settings_message_t));
    if (msg->data) {
        new_msg->data = strdup(msg->data);
    }
    return new_msg;
}

void DestroyMiwearSettingsMessage(miwear_settings_message_t* msg)
{
    assert(msg);
    if (msg->data) {
        free((void*)msg->data);
    }
    free(msg);
}

class MiwearSettings : public Settings {
public:
    MiwearSettings(uv_loop_t* loop)
        : miwear_settings_(nullptr)
        , loop_(loop)
    {
    }

    Status Init() override
    {
        uv_async_init(loop_, &async_, ProcessMessageAsyncCallback);
        uv_mutex_init(&queue_mutex_);
        async_.data = this;
        miwear_settings_ = miwear_settings_create(this);
        if (!miwear_settings_) {
            return Status::GeneralError("miwear_settings_create failed");
        }

        return Status::Ok();
    }

    void Detach(FeatureInstanceHandle handle) override
    {
        auto detach_handle = [handle](std::set<uintptr_t>& req_set) {
            auto it = req_set.begin();
            while (it != req_set.end()) {
                auto req = reinterpret_cast<RequestCallback*>(*it);
                if (req->handle() == handle) {
                    it = req_set.erase(it);
                    delete req;
                } else {
                    ++it;
                }
            }
        };

        detach_handle(request_set_);
        detach_handle(sub_set_);
    }

    // This is a request class. Settings interface is asynchronous, this class is used to
    // give the result of the operation to user(js) by execute the callback.
    // It encapsulates feature handle, automatic dup and free feature handle.
    class RequestCallback {
    public:
        RequestCallback(FeatureInstanceHandle handle)
            : handle_(handle)
        {
            FeatureDupInstanceHandle(handle_);
        }
        RequestCallback(const RequestCallback&) = delete;
        RequestCallback& operator=(const RequestCallback&) = delete;
        FeatureInstanceHandle handle() const
        {
            return handle_;
        }
        virtual void Call(int status, void* args) = 0;
        virtual ~RequestCallback()
        {
            FeatureFreeInstanceHandle(handle_);
        }

    protected:
        FeatureInstanceHandle handle_;
    };

    // This class stores subscription requests. A subscription is a persistent entity
    // that only gets deleted when the user explicitly calls unsubscribe.
    // Otherwise, it will persist until the owning Setting Feature is Destroyed.
    // It is stored in MiwearSettings::sub_set_
    class SubInfo : public RequestCallback {
    public:
        SubInfo(std::string key, FeatureInstanceHandle handle, FtCallbackId cid)
            : RequestCallback(handle)
            , key_(std::move(key))
            , cid_(cid)
        {
        }
        void Call(int status, void* args) override
        {
            if (status != 0) {
                SETTINGS_WARN("miwear_settings update failed");
                return;
            }

            ft_context_ref ctx_ = FeatureGetContext(handle_);
            const char* str = static_cast<const char*>(args);
            ft_value_t value = ft_parse_json(ctx_, str, strlen(str), nullptr);
            if (ft_get_type(ctx_, value) == FT_TYPE_UNDEF) {
                SETTINGS_WARN("miwear_settings update failed, %s", str);
                return;
            }
            FeatureInvokeCallback(handle_, cid_, &value);
            ft_free_value(ctx_, value);
        }

        void UpdateCid(FtCallbackId cid)
        {
            FeatureRemoveCallback(handle_, cid_);
            cid_ = cid;
        }

        const std::string& key() const
        {
            return key_;
        }
        ~SubInfo()
        {
            FeatureRemoveCallback(handle_, cid_);
        }

    private:
        std::string key_;
        FtCallbackId cid_;
    };

    Status SetProp(std::string_view key, std::string_view value, FeatureInstanceHandle handle, FtPromiseId pid) override
    {
        miwear_settings_message_t msg {
            .type = MIWEARSETTINGS_MESSAGE_TYPE_SET_PROPS,
            .status = 0,
            .data = value.data(),
        };
        // This class stores Set requests. A Set request is a short entity
        // it will be deleted immediately when we get the success message and
        // the callback be called.
        class SetReq : public RequestCallback {
        public:
            SetReq(FeatureInstanceHandle handle, FtPromiseId pid)
                : RequestCallback(handle)
                , pid_(pid)
            {
            }
            void Call(int status, void* args) override
            {
                if (status == 0) {
                    FeaturePromiseResolve(handle_, pid_); // it should be void
                } else {
                    auto err = Status::GeneralError("miwear_settings setprop failed");
                    FeaturePromiseReject(handle_, pid_, err.code(), err.err_msg().c_str());
                }
            }

        private:
            FtPromiseId pid_;
        };

        SetReq* req = new SetReq(handle, pid);

        int ret = miwear_settings_setprop(miwear_settings_, key.data(),
            &msg, MiwearSettingsCallback, req);

        if (ret != 0) {
            delete req;
            return Status::GeneralError("miwear_settings_setprop failed");
        }

        uintptr_t req_id = reinterpret_cast<uintptr_t>(req);
        request_set_.insert(req_id);
        return Status::Ok();
    }

    Status GetProp(std::string_view key, FeatureInstanceHandle handle, FtPromiseId pid) override
    {
        // This class stores Get requests. A Get request is a short entity
        // it will be deleted immediately when we get the success message and
        // the callback be called.
        class GetReq : public RequestCallback {
        public:
            GetReq(FeatureInstanceHandle handle, FtPromiseId pid, std::string key)
                : RequestCallback(handle)
                , pid_(pid)
                , key_(std::move(key))
            {
            }
            void Call(int status, void* args) override
            {
                if (status == 0) {
                    ft_context_ref ctx_ = FeatureGetContext(handle_);
                    const char* str = static_cast<const char*>(args);
                    ft_value_t obj = ft_parse_json(ctx_, str, strlen(str), nullptr);

                    ft_value_t value = ft_obj_get_property(ctx_, obj, key_.c_str());

                    if (ft_get_type(ctx_, obj) == FT_TYPE_UNDEF) {
                        SETTINGS_WARN("miwear_settings get failed, %s", str);
                        FeaturePromiseReject(handle_, pid_, FT_ERR_TASK_FAILED, "invalid json string");
                    } else {
                        FeaturePromiseResolve(handle_, pid_, &value);
                    }

                    ft_free_value(ctx_, value);
                    ft_free_value(ctx_, obj);
                } else {
                    auto err = Status::GeneralError("miwear_settings: getprop failed");
                    FeaturePromiseReject(handle_, pid_, err.code(), err.err_msg().c_str());
                }
            }

        private:
            FtPromiseId pid_;
            std::string key_;
        };

        GetReq* req = new GetReq(handle, pid, std::string(key));
        int ret = miwear_settings_getprop(miwear_settings_, key.data(), MiwearSettingsCallback, req);
        if (ret != 0) {
            delete req;
            return Status::GeneralError("miwear_settings_getprop failed");
        }

        uintptr_t req_id = reinterpret_cast<uintptr_t>(req);
        request_set_.insert(req_id);
        return Status::Ok();
    }

    Status SubscribeProp(std::string_view key, FeatureInstanceHandle handle, FtCallbackId cid) override
    {
        // check if there are subscribed callbacks on key
        for (auto sub_id : sub_set_) {
            auto sub_info = reinterpret_cast<SubInfo*>(sub_id);
            if (sub_info->key() == key && sub_info->handle() == handle) {
                // already subscribed
                SETTINGS_INFO("%s already subscribed", key.data());
                sub_info->UpdateCid(cid);
                return Status::Ok();
            }
        }

        auto* sub_info = new SubInfo(std::string(key), handle, cid);

        int ret = miwear_settings_subscribe_prop(miwear_settings_, key.data(), MiwearSettingsCallback, sub_info);
        if (ret != 0) {
            delete sub_info;
            return Status::GeneralError("miwear_settings_subscribe failed");
        }

        uintptr_t sub_id = reinterpret_cast<uintptr_t>(sub_info);
        sub_set_.insert(static_cast<uintptr_t>(sub_id));

        return Status::Ok();
    }

    Status UnsubscribeProp(std::string_view key, FeatureInstanceHandle handle) override
    {
        int ret = miwear_settings_unsubscribe_prop(miwear_settings_, key.data());
        if (ret != 0) {
            return Status::GeneralError("miwear_settings_unsubscribe failed");
        }

        std::string key_str(key);
        auto it = sub_set_.begin();
        while (it != sub_set_.end()) {
            auto sub_info = reinterpret_cast<SubInfo*>(*it);
            if (sub_info->key() == key_str && sub_info->handle() == handle) {
                it = sub_set_.erase(it);
                delete sub_info;
            } else {
                it++;
            }
        }
        return Status::Ok();
    }

    void Destroy() override
    {
        if (miwear_settings_) {
            miwear_settings_->user_data = nullptr;
            miwear_settings_destroy(miwear_settings_, MiwearSettingsDestroyCallback);
            miwear_settings_ = nullptr;
        }

        uv_close((uv_handle_t*)&async_, AsyncCloseCallback);
    }

    // process message from miwear_settings, assume it will be called in another thread, so it should be thread safe
    void ProcessMessageAsync(miwear_settings_message_t* msg, void* arg) override
    {
        uv_mutex_lock(&queue_mutex_);
        msg_queue_.emplace_back(msg, static_cast<RequestCallback*>(arg));
        uv_async_send(&async_);
        uv_mutex_unlock(&queue_mutex_);
    }

    std::set<uintptr_t>& request_set()
    {
        return request_set_;
    }

    std::set<uintptr_t>& sub_set()
    {
        return sub_set_;
    }

private:
    ~MiwearSettings()
    {
        for (auto sub_id : sub_set_) {
            auto sub_info = reinterpret_cast<SubInfo*>(sub_id);
            delete sub_info;
        }
        for (auto req_id : request_set_) {
            auto req_info = reinterpret_cast<RequestCallback*>(req_id);
            delete req_info;
        }
        for (auto& [msg, req] : msg_queue_) {
            DestroyMiwearSettingsMessage(msg);
            delete req;
        }
    }
    // Destroy callback for miwear_settings, it's do nothing.
    static void MiwearSettingsDestroyCallback(miwear_settings_t* settings, miwear_settings_message_t* msg, void* arg);

    // callback for miwear_settings, assume it will be called in another thread, so it should be thread safe
    static void MiwearSettingsCallback(miwear_settings_t* settings, miwear_settings_message_t* msg, void* arg);

    // callback for async_, it will be called in main loop
    static void ProcessMessageAsyncCallback(uv_async_t* handle);

    // close callback for async_
    static void AsyncCloseCallback(uv_handle_t* handle);

    // instance for miwear_settings
    miwear_settings_t* miwear_settings_;

    // set for get/set req, used for valid req, it is a pointer to req
    std::set<uintptr_t> request_set_;
    // set for subscriptions, used for valid SubInfo, it is a pointer to SubInfo
    std::set<uintptr_t> sub_set_;

    uv_loop_t* loop_;
    // used to accept message from miwear_settings thread
    uv_async_t async_;

    // protect msg_queue_
    uv_mutex_t queue_mutex_;
    std::vector<std::pair<miwear_settings_message_t*, RequestCallback*>> msg_queue_;
};

void MiwearSettings::AsyncCloseCallback(uv_handle_t* handle)
{
    auto settings = static_cast<MiwearSettings*>(handle->data);
    delete settings;
}

void MiwearSettings::ProcessMessageAsyncCallback(uv_async_t* handle)
{
    auto settings = static_cast<MiwearSettings*>(handle->data);
    if (!settings) {
        SETTINGS_WARN("Settings is nullptr");
        return;
    }
    uv_mutex_lock(&settings->queue_mutex_);
    auto msg_queue_ = std::move(settings->msg_queue_);
    uv_mutex_unlock(&settings->queue_mutex_);

    for (const auto& [msg, req] : msg_queue_) {
        SETTINGS_INFO("process message from miwear_settings type <%d> status:<%d>", msg->type, msg->status);
        auto& req_set = settings->request_set();
        uintptr_t id = reinterpret_cast<uintptr_t>(req);
        if (req_set.count(id)) {
            req->Call(msg->status, (void*)msg->data);
            req_set.erase(id);
            delete req;
        }
        auto& sub_set = settings->sub_set();
        if (sub_set.count(id)) {
            req->Call(msg->status, (void*)msg->data);
        }
        DestroyMiwearSettingsMessage(msg);
    }
}
void MiwearSettings::MiwearSettingsDestroyCallback(miwear_settings_t* miwear_settings, miwear_settings_message_t* msg, void* arg)
{
    auto settings = static_cast<Settings*>(miwear_settings->user_data);
    assert(settings == nullptr);
    SETTINGS_INFO("miwear_settings destroyed");
}

void MiwearSettings::MiwearSettingsCallback(miwear_settings_t* miwear_settings, miwear_settings_message_t* msg, void* arg)
{
    auto settings = static_cast<Settings*>(miwear_settings->user_data);
    if (!settings) {
        SETTINGS_WARN("Settings is nullptr");
        return;
    }

    miwear_settings_message_t* new_msg = CloneMiwearSettingsMessage(msg);
    settings->ProcessMessageAsync(new_msg, arg);
}

} // namespace system_settings

void system_settings_onRegister(const char* feature_name)
{
}

void system_settings_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    uv_loop_t* loop = FeatureGetUVLoop(manager);

    system_settings::Settings* settings = new system_settings::MiwearSettings(loop);
    system_settings::Status s = settings->Init();
    if (!s.ok()) {
        SETTINGS_WARN("system_settings onCreate, init failed: %s", s.err_msg().c_str());
        return;
    }

    FeatureSetProtoData(handle, settings);
}

void system_settings_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    SETTINGS_INFO("system_settings onRequired");
    FeatureProtoHandle proto = FeatureGetProtoHandle(handle);
    auto settings = static_cast<system_settings::Settings*>(
        FeatureGetProtoData(proto));
    FeatureSetObjectData(handle, settings);
}
void system_settings_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    SETTINGS_INFO("system_settings onDetached");
    auto settings = static_cast<system_settings::Settings*>(
        FeatureGetObjectData(handle));
    settings->Detach(handle);
    FeatureSetObjectData(handle, nullptr);
}

void system_settings_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    auto* settings = static_cast<system_settings::Settings*>(FeatureGetProtoData(handle));
    FeatureSetProtoData(handle, nullptr);
    settings->Destroy();
}

void system_settings_onUnregister(const char* feature_name)
{
}

void system_settings_wrap_getProp(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_settings_Key* key)
{
    auto* settings = static_cast<system_settings::Settings*>(
        FeatureGetObjectData(feature));
    if (!settings) {
        SETTINGS_WARN("Settings is nullptr");
        FeaturePromiseReject(feature, pid, FT_ERR_GENERAL, "Settings is nullptr");
        return;
    }

    if (!key->key) {
        FeaturePromiseReject(feature, pid, FT_ERR_ARGS, "key is nullptr");
        return;
    }
    settings->GetProp(key->key, feature, pid);
}
void system_settings_wrap_setProp(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_settings_SetParm* set_parm)
{
    auto* settings = static_cast<system_settings::Settings*>(
        FeatureGetObjectData(feature));
    if (!settings) {
        SETTINGS_WARN("Settings is nullptr");
        FeaturePromiseReject(feature, pid, FT_ERR_GENERAL, "Settings is nullptr");
        return;
    }

    if (!set_parm->value || !set_parm->key) {
        FeaturePromiseReject(feature, pid, FT_ERR_ARGS, "key or value is nullptr");
        return;
    }

    ft_value_t value = *set_parm->value;

    ft_context_ref ctx = FeatureGetContext(feature);
    int type = ft_get_type(ctx, value);
    if (type != FT_TYPE_STRING && type != FT_TYPE_NUMBER) {
        FeaturePromiseReject(feature, pid, FT_ERR_ARGS, "setprop should be string or number");
        return;
    }

    const char* val_str = ft_to_string(ctx, value);
    if (!val_str) {
        ft_free_string(ctx, val_str);
        FeaturePromiseReject(feature, pid, FT_ERR_ARGS, "conver to string failed");
        return;
    }

    system_settings::Status s = settings->SetProp(set_parm->key, val_str, feature, pid);
    ft_free_string(ctx, val_str);
    if (!s.ok()) {
        FeaturePromiseReject(feature, pid, s.code(), s.err_msg().c_str());
    }
}

void system_settings_wrap_subscribeProp(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_settings_SubScribe* subscribe)
{
    auto* settings = static_cast<system_settings::Settings*>(
        FeatureGetObjectData(feature));
    if (!settings) {
        SETTINGS_WARN("Settings is nullptr");
        FeaturePromiseReject(feature, pid, FT_ERR_GENERAL, "Settings is nullptr");
        return;
    }
    if (!subscribe->key) {
        FeaturePromiseReject(feature, pid, FT_ERR_ARGS, "key is nullptr");
        return;
    }

    auto s = settings->SubscribeProp(subscribe->key, feature, subscribe->callback);
    if (!s.ok()) {
        FeaturePromiseReject(feature, pid, s.code(), s.err_msg().c_str());
    } else {
        FeaturePromiseResolve(feature, pid);
    }
}
void system_settings_wrap_unsubscribeProp(FeatureInstanceHandle feature, AppendData append_data, FtString key)
{
    auto* settings = static_cast<system_settings::Settings*>(
        FeatureGetObjectData(feature));
    if (!settings) {
        SETTINGS_WARN("Settings is nullptr");
        return;
    }

    auto s = settings->UnsubscribeProp(key, feature);
    if (!s.ok()) {
        SETTINGS_WARN("unsubscribeProp failed: %s", s.err_msg().c_str());
    }
}
