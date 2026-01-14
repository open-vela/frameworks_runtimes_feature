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

#include "interconnect.h"

#include <set>
#include <string>

#include "feature_trace.h"
#include "uv_ext.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define TAG "[interconnect_impl]"

#define INTERCONNECT_DEBUG(fmt, ...) FEATURE_LOG_DEBUG(TAG fmt, ##__VA_ARGS__)
#define INTERCONNECT_INFO(fmt, ...) FEATURE_LOG_INFO(TAG fmt, ##__VA_ARGS__)
#define INTERCONNECT_WARN(fmt, ...) FEATURE_LOG_WARN(TAG fmt, ##__VA_ARGS__)
#define INTERCONNECT_ERROR(fmt, ...) FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

#define CHECK_LOG_RETURN(ptr, info)   \
    do {                              \
        if (!ptr) {                   \
            INTERCONNECT_ERROR(info); \
            return;                   \
        }                             \
    } while (0);

#define CHECK_LOG_RETURN_X(ptr, info, x) \
    do {                                 \
        if (!ptr) {                      \
            INTERCONNECT_ERROR(info);    \
            return x;                    \
        }                                \
    } while (0)

FtString createString(const char* str)
{
    assert(str);
    size_t len = strlen(str);
    char* ret = static_cast<char*>(FeatureMalloc(len + 1, FT_STRING));
    strncpy(ret, str, len + 1);
    return ret;
}

FtString createString(const char* data, size_t len)
{
    assert(data);
    char* ret = static_cast<char*>(FeatureMalloc(len + 1, FT_STRING));
    strncpy(ret, data, len);
    ret[len] = '\0';
    return ret;
}

void system_interconnect_onRegister(const char* feature_name)
{
    INTERCONNECT_DEBUG("system_interconnect_onRegister");
}

void system_interconnect_onUnregister(const char* feature_name)
{
    INTERCONNECT_DEBUG("system_interconnect_onUnregister")
}

namespace system_interconnect {

enum class StatusCode {
    kOk = 0,
    kInvalidArgs = 202,
    kUnsupported = 203,
    kTimeout = 204,
    kUnknown = 1000,
    kAppUninstall = 1001,
    kDisconnect = 1006,
};

constexpr int kInvalidPromiseId = -1;

/**
 * @brief 拓展 MIWEAR_STATUS 的定义
 */
enum MiwearStatus {
    MIWEAR_STATUS_CLIENT_INIT = 0, // client 刚初始化
};

/**
 * @brief 代表与手机的一个连接上下文。生命周期跟随
 * featureProto。在快应用线程内是一个单例。 该单例将在 shutDown 时，调用 destroy() 销毁
 */
class InterconnectContext {
public:
    struct SendTask {
        std::string msg;
        InterconnectContext* conn { nullptr };
        uv_timer_t* timer { nullptr }; // for pending send task
        FtPromiseId pid;
    };

    static const int kDiagnosisTimeout = 10000;

    /**
     * @brief miwear 的回调函数。所有miwear的消息都通过这个函数处理。包括:
     * 连接状态与数据接收。
     *
     * @param miwear miwear 连接实例
     * @param status pipe 状态. 0 表示成功，非 0 表示失败
     * @param msg 消息体
     * @param client 客户端名称
     */
    static void __miwear_connect_cb(uv_miwear_t* miwear,
        int status,
        uv_miwear_message_t* msg,
        const char* client)
    {
        INTERCONNECT_INFO("connect callback %s", client ? client : "nothing");
        auto* conn = static_cast<InterconnectContext*>(miwear->data);
        assert(msg);
        if (!conn) {
            INTERCONNECT_INFO("conn is null, miwear connection has closed");
            if (msg->header.type == MIWEAR_MESSAGE_TYPE_STATUS) {
                auto miwear_status = static_cast<const uv_miwear_status_t*>(msg->data);
                INTERCONNECT_INFO("connect status: %d", miwear_status->status);
                if (miwear_status->status == MIWEAR_STATUS_CONNECTION_CLOSED) {
                    free(miwear);
                }
            }

            return;
        }

        if (unlikely(status != 0)) {
            INTERCONNECT_ERROR("connect failed, status: %d", status);
            conn->ProcessPendingDiagnosis(false);
            return;
        }

        if (msg->header.type == MIWEAR_MESSAGE_TYPE_STATUS) {
            auto miwear_status = static_cast<const uv_miwear_status_t*>(msg->data);
            auto old_status = conn->set_status(miwear_status->status);
            INTERCONNECT_INFO("recv status %s", conn->status_name());
            switch (conn->status()) {
            case MIWEAR_STATUS_CONNECT_FAILED: {
                // 连接失败
                FEATURE_NOTE_MARK("interconnect_failed");
                conn->InvokeError("connect to app failed",
                    static_cast<int>(StatusCode::kUnknown));
                conn->ProcessPendingDiagnosis(false);
                break;
            }
            case MIWEAR_STATUS_PHONE_CONNECTED: {
                // 连接成功
                FEATURE_NOTE_MARK("interconnect_success");
                conn->ProcessPendingDiagnosis(false);
                conn->ProcessPendingSendTasks();
                if (__IsConnecting(old_status)) {
                    conn->InvokeConnect(true); // connected first time
                } else {
                    conn->InvokeConnect(false); // reconnected
                }
                break;
            }
            case MIWEAR_STATUS_PHONE_DISCONNECTED: {
                // 连接断开
                FEATURE_NOTE_MARK("interconnect_disconnected");
                conn->ProcessPendingDiagnosis(false); // 处理诊断 pending
                conn->ClearPendingConnect();
                if (__IsConnecting(old_status)) { // 连接失败
                    conn->InvokeError("connect to miwear server failed",
                        static_cast<int>(StatusCode::kDisconnect));
                } else { // 连接断开
                    conn->InvokeDisconnect("disconnect",
                        static_cast<int>(StatusCode::kUnknown));
                }

                break;
            }
            case MIWEAR_STATUS_PHONE_UNINSTALLED: {
                // 应用未安装
                FEATURE_NOTE_MARK("interconnect_app_uninstalled");
                conn->ProcessPendingDiagnosis(false);
                conn->InvokeDisconnect("phone app uninstalled",
                    static_cast<int>(StatusCode::kAppUninstall));
                break;
            }
            default: {
                INTERCONNECT_ERROR("unknown status %s", conn->status_name());
                break;
            }
            }
        } else if (msg->header.type == MIWEAR_MESSAGE_TYPE_DATA) {
            // 收到来自手机的消息
            FEATURE_NOTE_MARK("interconnect_recv_from_phone");
            conn->InvokeRecv(static_cast<const char*>(msg->data), msg->header.len);
        }
    }

    InterconnectContext(uv_loop_t* loop)
        : loop_(loop)
        , status_(0) // 0 -> connecting
    {
        miwear_ = static_cast<uv_miwear_t*>(malloc(sizeof(uv_miwear_t)));
        memset(miwear_, 0, sizeof(uv_miwear_t));
        miwear_->data = this;
    }

    InterconnectContext(const InterconnectContext&) = delete;
    InterconnectContext& operator=(const InterconnectContext&) = delete;

    bool IsConnecting() const { return __IsConnecting(status_); }

    static bool __IsConnecting(size_t status)
    {
        return status == MIWEAR_STATUS_CLIENT_INIT || status == MIWEAR_STATUS_CLIENT_ID_SENT;
    }

    bool IsConnected() const { return status_ == MIWEAR_STATUS_PHONE_CONNECTED; }

    enum class ConnectResult {
        kOk = 0,
        kConnecting = 1,
        kEmptyPackageName = -1,
        kConnectFailed = -2
    };
    /**
     * @brief 连接到手机，并等待连接结果。需要预先设置 package_name
     *        可以多次调用，但只会建立一次连接。
     *
     * @return ConnectResult
     */
    ConnectResult connect()
    {
        if (connect_send_) {
            INTERCONNECT_INFO("connect request has sent");
            return ConnectResult::kConnecting;
        }
        if (package_name_.empty()) {
            INTERCONNECT_INFO("package_name is empty");
            return ConnectResult::kEmptyPackageName;
        }
        // 开始连接
        FEATURE_NOTE_MARK("interconnect_start");
        int res = 0;
        INTERCONNECT_INFO("connect to %s", package_name_.c_str());
        res = uv_miwear_connect(loop_, miwear_, package_name_.c_str(),
            __miwear_connect_cb);
        if (res != 0) {
            INTERCONNECT_ERROR("connect failed");
            return ConnectResult::kConnectFailed;
        }

        connect_send_ = true;
        return ConnectResult::kOk;
    }

    /**
     * @brief uv_close 的回调函数。只负责释放 handle 内存。
     */
    static void __mm_free_handle(uv_handle_t* handle) { free(handle); }

    /**
     * @brief 诊断超时定时器回调。
     */
    static void __timer_diagnosis_cb(uv_timer_t* handle)
    {
        // diagnosis 超时
        FEATURE_NOTE_MARK("interconnect_diagnosis_timeout");
        auto conn = static_cast<system_interconnect::InterconnectContext*>(handle->data);
        if (!conn) {
            INTERCONNECT_INFO("conn is null");
            return;
        }
        conn->ProcessPendingDiagnosis(true);
    }

    /**
     * @brief 销毁连接上下文。包括: 销毁定时器，销毁 send 任务。
     */
    void destroy()
    {
        // 处理 pending diagnosis call
        if (diagnosis_timer_) {
            uv_timer_stop(diagnosis_timer_);
            uv_close(reinterpret_cast<uv_handle_t*>(diagnosis_timer_),
                __mm_free_handle);
            diagnosis_timer_ = nullptr;
            if (diagnosis_promise_id_ != kInvalidPromiseId) {
                INTERCONNECT_WARN("unrejected diagnosis call");
                diagnosis_promise_id_ = kInvalidPromiseId;
            }
        }

        // 处理 pending send tasks
        for (const auto task : send_tasks_) {
            if (task->timer) { // 清除 pending send task
                task->timer->data = nullptr; // remove ref to task
                uv_close(reinterpret_cast<uv_handle_t*>(task->timer), __mm_free_handle);
            }

            INTERCONNECT_WARN("unrejected send task");
            delete task;
        }

        if (err_info_) {
            FeatureFreeValue(err_info_);
            err_info_ = nullptr;
        }

        delete this;
    }

    /**
     * @brief 连接正在建立，pending Diagnosis 任务，等待 interconnect 连接结果。
     * 同一时间只允许有一个Diagnosis任务， 在上一个任务没有结束前，新的
     * Diagnosis 任务会返回失败。
     *
     * @param id      回调函数 id
     * @param timeout 超时时间
     */
    StatusCode PendingDiagnosis(FtPromiseId id, int timeout)
    {
        if (timeout <= 0) {
            INTERCONNECT_ERROR("timeout should be positive");
            return StatusCode::kInvalidArgs;
        }

        if (diagnosis_promise_id_ != kInvalidPromiseId) {
            INTERCONNECT_INFO("pending diagnosis call already exists");
            return StatusCode::kUnsupported;
        }

        if (!diagnosis_timer_) {
            diagnosis_timer_ = static_cast<uv_timer_t*>(calloc(1, sizeof(uv_timer_t)));
            diagnosis_timer_->data = this;
            int ret = uv_timer_init(loop_, diagnosis_timer_);
            assert(ret == 0);
        }

        diagnosis_promise_id_ = id;
        uv_timer_start(diagnosis_timer_, __timer_diagnosis_cb, timeout, 0);

        return StatusCode::kOk;
    }

    /**
     * @brief 连接建立时，处理未缓存的 send 任务。
     */
    void ProcessPendingSendTasks()
    {
        for (const auto task : send_tasks_) {
            if (!task->timer) { // no timer means send task has been sent
                continue;
            }
            uv_timer_stop(task->timer);
            uv_close(reinterpret_cast<uv_handle_t*>(task->timer), __mm_free_handle);
            task->timer = nullptr;
            SendMsg(task);
        }
    }

    /**
     * @brief 处理 pending diagnosis call。
     *
     * @param is_timeout 是否是因为超时
     */
    void ProcessPendingDiagnosis(bool is_timeout)
    {
        if (diagnosis_promise_id_ == kInvalidPromiseId) {
            INTERCONNECT_INFO("no pending diagnosis call");
            return;
        }

        assert(diagnosis_timer_);
        uv_timer_stop(diagnosis_timer_);
        ProcessDiagnosisResult(diagnosis_promise_id_, is_timeout);
        diagnosis_promise_id_ = kInvalidPromiseId;
    }

    void ProcessDiagnosisResult(FtPromiseId pid, bool is_timeout)
    {
        int status = static_cast<int>(StatusCode::kOk);
        if (is_timeout) {
            status = static_cast<int>(StatusCode::kTimeout);
        } else {
            switch (status_) {
            case MIWEAR_STATUS_CONNECT_FAILED:
                [[fallthrough]];
            case MIWEAR_STATUS_PHONE_DISCONNECTED: { // 连接断开，错误未知
                status = static_cast<int>(StatusCode::kUnknown);
                break;
            }
            case MIWEAR_STATUS_PHONE_CONNECTED: { // 连接成功
                status = static_cast<int>(StatusCode::kOk);
                break;
            }
            case MIWEAR_STATUS_PHONE_UNINSTALLED: { // 应用未安装
                status = static_cast<int>(StatusCode::kAppUninstall);
                break;
            }
            default: { // 未知错误
                INTERCONNECT_ERROR("interconnect status error %s", status_name());
                status = static_cast<int>(StatusCode::kUnknown);
                break;
            }
            }
        }
        // diagnosis 成功
        FEATURE_NOTE_MARK("interconnect_diagnosis_success");
        ft_context_ref ctx = FeatureGetContext(handle_);
        ft_value_t obj = ft_new_object(ctx);
        ft_obj_set_property(ctx, obj, "status", ft_from_int(ctx, status));
        FeaturePromiseResolve(handle_, pid, &obj);
        ft_free_value(ctx, obj);
    }

    /**
     * @brief
     * 发送消息超时回调。在超时时间内，连接还未建立，本次发送消息失败，返回超时。
     */
    static void __timer_send_cb(uv_timer_t* handle)
    {
        // 代码执行到这里，说明在超时时间内，miwear 没有重新建立连接。丢弃本次发送
        SendTask* task = static_cast<SendTask*>(handle->data);
        INTERCONNECT_INFO("send timeout %p", task);
        task->conn->ProcessSendResult(task, SendResult::kTimeout);
        uv_close(reinterpret_cast<uv_handle_t*>(handle), __mm_free_handle);
    }

    /**
     * @brief 发送数据的回调函数，用于 uv_miwear_send(...)
     */
    static void __miwear_send_cb(uv_miwear_t* miwear,
        int status,
        uv_miwear_message_t* msg,
        void* cb_para)
    {
        // send 成功
        FEATURE_NOTE_MARK("interconnect_send_success");
        auto conn = static_cast<system_interconnect::InterconnectContext*>(miwear->data);
        SendTask* task = static_cast<SendTask*>(cb_para);
        if (!conn) {
            INTERCONNECT_INFO("conn is closed");
            return;
        }
        conn->ProcessSendResult(
            task, status == 0 ? SendResult::kOk : SendResult::kFailed);
    }

    enum class SendResult {
        kOk,
        kFailed,
        kTimeout,
    };
    /**
     * @brief 处理发送数据的结果
     *
     * @param task 发送任务
     * @param result 发送结果，Ok 表示成功，Failed 表示失败，Timeout 表示超时
     */
    void ProcessSendResult(SendTask* task, SendResult result)
    {
        if (!send_tasks_.count(task)) {
            return;
        }

        send_tasks_.erase(task);
        switch (result) {
        case SendResult::kOk: {
            FeaturePromiseResolve(handle_, task->pid);
            break;
        }
        case SendResult::kTimeout: {
            FeaturePromiseReject(handle_, task->pid,
                static_cast<int>(StatusCode::kTimeout),
                "send timeout");
            break;
        }
        case SendResult::kFailed: {
            FeaturePromiseReject(handle_, task->pid,
                static_cast<int>(StatusCode::kDisconnect),
                "connection broken");
            break;
        }
        default: {
            assert(false);
        }
        }
        delete task;
    }

    size_t set_status(int status)
    {
        size_t old_status = status_;
        status_ = status;
        return old_status;
    }

    int status() { return status_; }

    const char* status_name()
    {
        assert(status_ < sizeof(status_name_) / sizeof(char*));
        return status_name_[status_];
    }

    /**
     * @brief 关闭 Interconnect 连接。Interconnect 单例使用该接口触发销毁流程。
     */
    void shutdown()
    {
        if (connect_send_) { // 已经开始连接了
            miwear_->data = nullptr; // 清空 conn 指针
            uv_miwear_close(miwear_);
        }
        destroy();
    }

    void set_package_name(const char* package_name)
    {
        package_name_ = package_name;
    }

    FtCallbackId get_recv_func() { return recv_func_; }

    void set_recv_func(FtCallbackId func)
    {
        FeatureRemoveCallback(handle_, recv_func_);
        recv_func_ = func;
        return;
    }

    FtCallbackId get_conn_func() { return conn_func_; }

    void set_conn_func(FtCallbackId func)
    {
        FeatureRemoveCallback(handle_, conn_func_);
        conn_func_ = func;
        if (is_pending_connect_) {
            InvokeConnect(true);
            is_pending_connect_ = false;
        }
        return;
    }

    void ClearPendingConnect() { is_pending_connect_ = false; }

    FtCallbackId get_error_func() { return error_func_; }

    void set_error_func(FtCallbackId func)
    {
        FeatureRemoveCallback(handle_, error_func_);
        error_func_ = func;
        if (err_info_) {
            FeatureInvokeCallback(handle_, error_func_, err_info_);
            FeatureFreeValue(err_info_);
            err_info_ = nullptr;
        }
        return;
    }

    FtCallbackId get_disconn_func() { return disconn_func_; }

    void set_disconn_func(FtCallbackId func)
    {
        FeatureRemoveCallback(handle_, disconn_func_);
        disconn_func_ = func;
        return;
    }

    /**
     * @brief 调用用户注册的错误监听函数
     */
    void InvokeError(std::string err_msg, int err_code)
    {
        system_interconnect_ErrorInfo* errinfo = system_interconnectMallocErrorInfo();
        errinfo->data = createString(err_msg.c_str(), err_msg.size());
        errinfo->code = err_code;
        if (error_func_ == 0) { // save error info， call it when callback is set
            INTERCONNECT_ERROR("error callback is not set yet");
            if (err_info_) // drop old error info
                FeatureFreeValue(err_info_);
            err_info_ = errinfo;
            return;
        }

        FeatureInvokeCallback(handle_, error_func_, errinfo);
        FeatureFreeValue(errinfo);
    }

    /**
     * @brief 从 SendParams 中解析出需要发送的数据 buffer
     */
    static StatusCode getMsg(ft_context_ref ctx,
        ft_value_t obj,
        std::string* value)
    {
        // ft_context_ref ctx = FeatureGetContext(handle_);
        int type = ft_get_type(ctx, obj);
        const char* buffer = nullptr;
        INTERCONNECT_INFO("data type %d", type);

        if (type != FT_TYPE_OBJECT) {
            return StatusCode::kInvalidArgs;
        }

        buffer = ft_to_string(ctx, obj);
        if (!buffer) {
            INTERCONNECT_ERROR("convert obj to string failed");
            return StatusCode::kInvalidArgs;
        }
        *value = std::string(buffer);
        ft_free_string(ctx, buffer);
        if (value->length() == 0) {
            return StatusCode::kInvalidArgs;
        }

        return StatusCode::kOk;
    }

    /**
     * @brief 发送数据到手机
     *
     * @param task 发送任务
     */
    void SendMsg(SendTask* task)
    {
        assert(IsConnected());
        uv_miwear_message_t msg;

        INTERCONNECT_INFO("send data len: %d, %.128s,", task->msg.size(),
            task->msg.c_str());
        msg.data = (void*)task->msg.c_str();
        msg.header.len = task->msg.size();
        msg.header.type = MIWEAR_MESSAGE_TYPE_DATA;

        uv_miwear_send(miwear_, NULL, &msg, __miwear_send_cb,
            static_cast<void*>(task));
        return;
    }

    /**
     * @brief 创建一个 send 任务
     *
     * @param parms 发送参数
     * @param is_pending 是否是 pending 任务
     */
    SendTask* CreateSendTask(std::string msg, FtPromiseId pid)
    {
        SendTask* task = new SendTask();
        task->msg = std::move(msg);
        task->pid = pid;
        task->conn = this;
        send_tasks_.insert(task);
        return task;
    }

    /**
     * @brief 缓存 send 任务，等待连接建立
     */
    StatusCode PendingSendTask(system_interconnect_SendParams* parms,
        FtPromiseId pid)
    {
        std::string buffer;
        if (!parms->data) {
            return StatusCode::kInvalidArgs;
        }

        StatusCode s = getMsg(FeatureGetContext(handle_), *parms->data, &buffer);
        if (s != StatusCode::kOk) {
            return s;
        }

        SendTask* task = CreateSendTask(std::move(buffer), pid);
        uv_timer_t* timer = static_cast<uv_timer_t*>(calloc(1, sizeof(uv_timer_t)));
        INTERCONNECT_INFO("pending send task %p, %dms", task, parms->timeout);

        timer->data = task;
        task->timer = timer;
        int ret = uv_timer_init(loop_, timer);
        assert(ret == 0);

        ret = uv_timer_start(timer, __timer_send_cb, parms->timeout, 0);
        assert(ret == 0);
        return StatusCode::kOk;
    }

    /**
     * @brief 连接建立时，执行的回调。
     *
     * @param is_reconnect 是否是重新连接。第一次连接时，is_reconnect 为 false。
     */
    void InvokeConnect(bool is_reconnect)
    {
        INTERCONNECT_INFO("InvokeConnect");
        if (conn_func_ == 0) {
            is_pending_connect_ = true;
            return;
        }
        if (!handle_) {
            INTERCONNECT_ERROR("handle_ is null");
            return;
        }
        ft_context_ref ctx = FeatureGetContext(handle_);
        ft_value_t obj = ft_new_object(ctx);
        ft_obj_set_property(ctx, obj, "isReconnected",
            ft_from_bool(ctx, is_reconnect));
        FeatureInvokeCallback(handle_, conn_func_, &obj);
        ft_free_value(ctx, obj);
    }

    /**
     * @brief 断开连接时，执行的回调。
     *        该函数在 onclose 注册。可以合并入 onerror 里面
     */
    void InvokeDisconnect(std::string msg, int code)
    {
        INTERCONNECT_INFO("InvokeDisconnect");
        system_interconnect_CloseInfo close_info = { 0 };

        close_info.data = msg.c_str();
        close_info.code = code;
        FeatureInvokeCallback(handle_, disconn_func_, &close_info);
    }

    ///@brief 设置接口句柄,只允许设置一次,
    void set_interface_handle(FeatureInterfaceHandle handle)
    {
        assert(handle_ == 0);
        handle_ = handle;
    }

    FeatureInterfaceHandle get_interface_handle() { return handle_; }

    /**
     * @brief 收到来自手机的数据时，执行的回调。
     */
    void InvokeRecv(const char* data, int len)
    {
        if (recv_func_ == 0) {
            INTERCONNECT_DEBUG("recv_func_ is null");
            return;
        }
        system_interconnect_Message msg = { 0 };
        std::string data_str(data, len);
        msg.data = data_str.c_str();

        // don't output string data in release mode
        INTERCONNECT_INFO("recv data len %d: %.128s", len, msg.data);
        FeatureInvokeCallback(handle_, recv_func_, &msg);
    }

private:
    /**
     * @brief 析构函数为空，主要销毁动作放在 Destroy 函数中
     */
    ~InterconnectContext() { }

    /// @brief status 转换成字符串，用于打印日志对象
    static const char* status_name_[8];

    /// @brief 应用的包名，用于连接手机端app
    std::string package_name_;

    uv_loop_t* loop_ { nullptr };

    /// @brief 连接状态, 对应到 status_name_
    size_t status_ { MIWEAR_STATUS_CLIENT_INIT };

    /// @brief 表示是否已经开始连接
    bool connect_send_ { false };

    bool is_pending_connect_ { false };

    /// @brief miwear 连接实例
    uv_miwear_t* miwear_ { nullptr };

    /// @brief 发送的数据的缓存，主要用于网络断开连接时，缓存数据。
    std::set<SendTask*> send_tasks_;

    /// @brief dignosis 定时器，
    // 如果连接正在建立，PendingDiagnosis 会创建该 timer，用于等待连接结果返回
    // 空值表示没有 pending 的 dignosis 调用。
    uv_timer_t* diagnosis_timer_ { nullptr };
    FtPromiseId diagnosis_promise_id_ { kInvalidPromiseId };

    /// @brief interface create by interconnect.instance()，only one per feature
    FeatureInterfaceHandle handle_ { 0 };

    /// @brief 用户注册的回调函数
    FtCallbackId recv_func_ { 0 }; // onmessage
    FtCallbackId conn_func_ { 0 }; // onopen
    FtCallbackId error_func_ { 0 }; // onerror
    system_interconnect_ErrorInfo* err_info_ { nullptr }; // cache error info for onerror
    FtCallbackId disconn_func_ { 0 }; // onclose
};

/**
 * @brief 连接状态码到字符串的映射 uv_miwear
 *        底层实现使用到了管道。因此有些状态的描述使用了管道一词。
 */
const char* system_interconnect::InterconnectContext::status_name_[8] = {
    "MIWEAR_STATUS_CONNECTING", // state before obtaining the connection
                                // result
    "MIWEAR_STATUS_CLIENT_ID_SENT", //  another kind of connecting state,
                                    //  which is not important
    "MIWEAR_STATUS_CONNECT_FAILED", //  state when pipe connection fails,
                                    //  which is very rare
    "MIWEAR_STATUS_CONNECTION_CLOSED", // state when pipe is closed
    "MIWEAR_STATUS_CLIENT_ONLINE-X", // never use in client side
    "MIWEAR_STATUS_PHONE_CONNECTED", // Connection to app in mobile
                                     // established
    "MIWEAR_STATUS_PHONE_DISCONNECTED", // Connection to app in mobile lost
    "MIWEAR_STATUS_PHONE_UNINSTALLED" //  the app is uninstalled in mobile
};
} // namespace system_interconnect

void system_interconnect_onCreate(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    INTERCONNECT_INFO("onCreate");
    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    uv_loop_t* loop = FeatureGetUVLoop(manager);
    auto conn = new system_interconnect::InterconnectContext(loop);
    FeatureSetProtoData(handle, conn);
}

void system_interconnect_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    INTERCONNECT_INFO("onDestroy");
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetProtoData(handle));

    conn->shutdown();
}

/**
 * @brief 用户调用 require("system.interconnect").
 */
void system_interconnect_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    INTERCONNECT_INFO("onRequired");
    FeatureProtoHandle proto = FeatureGetProtoHandle(handle);
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetProtoData(proto));
    conn->set_package_name(FeatureGetPackageName(proto));
    auto ret = conn->connect();
    INTERCONNECT_INFO("interconnect handle %p connect res %d, conn:%p", handle,
        ret, conn);

    FeatureSetObjectData(handle, conn);
}

void system_interconnect_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    INTERCONNECT_INFO("system_interconnect_onDetached");
}

/**
 * @brief 创建一个 interconnect 单例
 */
FeatureInterfaceHandle system_interconnect_wrap_instance(
    FeatureInstanceHandle feature,
    AppendData append_data,
    system_interconnect_AppInfo* info)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(feature));

    INTERCONNECT_INFO("interconnect.instance %p, conn %p info %p", feature, conn,
        info);

    assert(conn);

    FeatureInterfaceHandle handle = conn->get_interface_handle();
    if (handle != 0) {
        return handle;
    }

    if (info) {
        conn->set_package_name(info->package);
    }

    handle = system_interconnect_instance_instance(feature);
    INTERCONNECT_INFO("create instance %p\n", handle);

    conn->set_interface_handle(handle);
    FeatureSetObjectData(handle, conn);

    auto ret = conn->connect();

    if (static_cast<int>(ret) < 0) {
        INTERCONNECT_ERROR("connect failed %d", ret);
        return handle;
    }

    return handle;
}

void system_interconnect_InterConn_interface_MiwearConnect_finalize(
    FeatureInterfaceHandle handle)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    CHECK_LOG_RETURN(conn, "conn is null");
    FeatureSetObjectData(handle, nullptr);
    return;
}

/**
 * @brief 获取连接状态,目前 [0 表示连接中] 1 表示连接成功，2 表示连接断开
 *
 * @param params 两个 callback id 这两个 id 必须在函数退出之前，被remove掉
 * @return
 */
void system_interconnect_InterConn_interface_MiwearConnect_getReadyState(
    FeatureInterfaceHandle handle,
    AppendData append_data,
    FtPromiseId pid,
    system_interconnect_ReadystateParams* parms)
{
    // get ReadyState 开始
    FEATURE_NOTE_MARK("interconnect_get_ReadyState_start");
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));

    if (!conn) {
        INTERCONNECT_ERROR("conn is null");
        FeaturePromiseReject(
            handle, pid,
            static_cast<int>(system_interconnect::StatusCode::kUnknown),
            "conn has been shoutdown");
        return;
    }

    ft_context_ref ctx = FeatureGetContext(handle);
    ft_value_t obj = ft_new_object(ctx);
    ft_obj_set_property(ctx, obj, "status", ft_from_int(ctx, 2));
    FEATURE_LOG_INFO(" status: %s", conn->status_name());
    switch (conn->status()) {
    case MIWEAR_STATUS_CONNECT_FAILED: // fall through
    case MIWEAR_STATUS_PHONE_DISCONNECTED: // fall through
    case MIWEAR_STATUS_PHONE_UNINSTALLED: // fall through
    case MIWEAR_STATUS_CONNECTION_CLOSED: {
        ft_obj_set_property(ctx, obj, "status", ft_from_int(ctx, 2));
        break;
    }
    case MIWEAR_STATUS_PHONE_CONNECTED: {
        ft_obj_set_property(ctx, obj, "status", ft_from_int(ctx, 1));
        break;
    }
    default: {
        FEATURE_LOG_ERROR("unknown status: %s", conn->status_name());
    }
    }

    FeaturePromiseResolve(handle, pid, &obj);
    ft_free_value(ctx, obj);

    return;
}

/**
 * @brief 发送数据到手机端。连接断开时，缓存消息。
 */
void system_interconnect_InterConn_interface_MiwearConnect_send(
    FeatureInterfaceHandle handle,
    AppendData append_data,
    FtPromiseId pid,
    system_interconnect_SendParams* parms)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    // send
    FEATURE_NOTE_MARK("interconnect_send_msg");
    if (!conn) {
        INTERCONNECT_ERROR("conn is null");
        FeaturePromiseReject(
            handle, pid,
            static_cast<int>(system_interconnect::StatusCode::kDisconnect),
            "conn has been shoutdown");
        return;
    }

    system_interconnect::StatusCode s;
    if (conn->IsConnected()) {
        std::string buffer;
        if (!parms->data) {
            s = system_interconnect::StatusCode::kInvalidArgs;
            goto error;
        }
        s = system_interconnect::InterconnectContext::getMsg(
            FeatureGetContext(handle), *parms->data, &buffer);
        if (s != system_interconnect::StatusCode::kOk) {
            goto error;
        }
        conn->SendMsg(conn->CreateSendTask(std::move(buffer), pid));
    } else if (conn->IsConnecting()) {
        INTERCONNECT_INFO("status is connecting, pending %ds", parms->timeout);

        s = conn->PendingSendTask(parms, pid);
        if (s != system_interconnect::StatusCode::kOk) {
            goto error;
        }
    } else { // status is disconnected, send failed
        FeaturePromiseReject(handle, pid,
            static_cast<int>(system_interconnect::StatusCode::kDisconnect),
            "disconnect");
    }

    return;
error:
    const char* err_msg = nullptr;
    switch (s) {
    case system_interconnect::StatusCode::kInvalidArgs: {
        err_msg = "invalid args";
        break;
    }
    default: {
        err_msg = "pendingSendTask: unknown error";
        break;
    }
    }
    FeaturePromiseReject(handle, pid, static_cast<int>(s), err_msg);
}

void system_interconnect_InterConn_interface_MiwearConnect_diagnosis(
    FeatureInterfaceHandle handle,
    AppendData append_data,
    FtPromiseId pid,
    system_interconnect_DiagnosisParams* param)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    FEATURE_NOTE_MARK("interconnect_diagnosis_start");
    int timeout = system_interconnect::InterconnectContext::kDiagnosisTimeout;
    if (param) {
        timeout = param->timeout;
    }

    if (!conn) {
        INTERCONNECT_ERROR("conn is null %d\n", pid);
        FeaturePromiseReject(
            handle, pid,
            static_cast<int>(system_interconnect::StatusCode::kDisconnect),
            "conn has been shoutdown");
        return;
    }

    if (conn->IsConnecting()) {
        system_interconnect::StatusCode ret = conn->PendingDiagnosis(pid, timeout);
        if (ret != system_interconnect::StatusCode::kOk) {
            INTERCONNECT_ERROR("PendingDiagnosis failed");
            const char* err_msg = nullptr;
            switch (ret) {
            case system_interconnect::StatusCode::kInvalidArgs: {
                err_msg = "timeout should be positive";
                break;
            }
            case system_interconnect::StatusCode::kUnsupported: {
                err_msg = "diag";
                break;
            }
            default: {
                err_msg = "pending diagnosis: unknown error";
                break;
            }
            }
            FeaturePromiseReject(
                handle, pid,
                static_cast<int>(system_interconnect::StatusCode::kUnknown), err_msg);
        }
        return;
    }

    conn->ProcessDiagnosisResult(pid, false);
    return;
}

FtCallbackId
system_interconnect_InterConn_interface_MiwearConnect_get_onmessage(
    FeatureInterfaceHandle handle,
    AppendData append_data)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    CHECK_LOG_RETURN_X(conn, "conn is null", 0);
    return conn->get_recv_func();
}

void system_interconnect_InterConn_interface_MiwearConnect_set_onmessage(
    FeatureInterfaceHandle handle,
    AppendData append_data,
    FtCallbackId onmessage)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    CHECK_LOG_RETURN(conn, "conn is null");

    if (conn->get_interface_handle() != handle) {
        INTERCONNECT_ERROR("interface handle is not match");
        return;
    }
    conn->set_recv_func(onmessage);
    return;
}

FtCallbackId system_interconnect_InterConn_interface_MiwearConnect_get_onopen(
    FeatureInterfaceHandle handle,
    AppendData append_data)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    CHECK_LOG_RETURN_X(conn, "conn is null", 0);
    return conn->get_conn_func();
}

void system_interconnect_InterConn_interface_MiwearConnect_set_onopen(
    FeatureInterfaceHandle handle,
    AppendData append_data,
    FtCallbackId onopen)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));

    CHECK_LOG_RETURN(conn, "conn is null")

    conn->set_conn_func(onopen);
    return;
}

FtCallbackId system_interconnect_InterConn_interface_MiwearConnect_get_onclose(
    FeatureInterfaceHandle handle,
    AppendData append_data)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));

    CHECK_LOG_RETURN_X(conn, "conn is null", 0);
    return conn->get_disconn_func();
}

void system_interconnect_InterConn_interface_MiwearConnect_set_onclose(
    FeatureInterfaceHandle handle,
    AppendData append_data,
    FtCallbackId onclose)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));

    CHECK_LOG_RETURN(conn, "conn is null");
    conn->set_disconn_func(onclose);
    return;
}

FtCallbackId system_interconnect_InterConn_interface_MiwearConnect_get_onerror(
    FeatureInterfaceHandle handle,
    AppendData append_data)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    CHECK_LOG_RETURN_X(conn, "conn is null", 0);
    return conn->get_error_func();
}

void system_interconnect_InterConn_interface_MiwearConnect_set_onerror(
    FeatureInterfaceHandle handle,
    AppendData append_data,
    FtCallbackId onerror)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    CHECK_LOG_RETURN(conn, "conn is null");
    conn->set_error_func(onerror);
    return;
}

FtString system_interconnect_InterConn_interface_MiwearConnect_getApkStatus(
    FeatureInterfaceHandle handle,
    AppendData append_data)
{
    auto conn = static_cast<system_interconnect::InterconnectContext*>(
        FeatureGetObjectData(handle));
    CHECK_LOG_RETURN_X(conn, "conn is null", createString("DISCONNECTED"));

    const char* ret_str = nullptr;
    switch (conn->status()) {
    case MIWEAR_STATUS_PHONE_CONNECTED:
        ret_str = "CONNECTED";
        break;
    case MIWEAR_STATUS_PHONE_UNINSTALLED:
        ret_str = "UNINSTALLED";
        break;
    case MIWEAR_STATUS_PHONE_DISCONNECTED:
        ret_str = "DISCONNECTED";
        break;
    default:
        INTERCONNECT_INFO("status %s", conn->status_name());
        ret_str = "DISCONNECTED";
        break;
    }
    INTERCONNECT_INFO("get apkstatus %s", ret_str);
    return createString(ret_str);
}
