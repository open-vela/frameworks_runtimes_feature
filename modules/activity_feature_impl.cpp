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

#include <map>

#include "activity.h"
#include "app/Activity.h"
#include "app/ServiceConnection.h"

using android::sp;
using os::app::Context;
using os::app::Intent;

static const char* file_tag = "[jidl_feature] activity_feature_impl";

typedef struct CallData {
    FeatureInstanceHandle mHandle;
    FtCallbackId mId;
} CallData;

class FtServiceConnection : public os::app::ServiceConnection {
public:
    FtServiceConnection(FeatureInstanceHandle handler, FtCallbackId onConnectedCb,
        FtCallbackId onDisconnectedCb)
        : mHandler(FeatureDupInstanceHandle(handler))
        , mOnConnectedCb(onConnectedCb)
        , mOnDisConnectedCb(onDisconnectedCb)
    {
    }

    // FtServiceConnection is sp<IBinder>, it will be destroy after app exit!!!
    // we must clear feature ref by manual
    void clearFeatureRef()
    {
        if (FeatureCheckCallbackId(mHandler, mOnConnectedCb)) {
            FeatureRemoveCallback(mHandler, mOnConnectedCb);
        }
        if (FeatureCheckCallbackId(mHandler, mOnDisConnectedCb)) {
            FeatureRemoveCallback(mHandler, mOnDisConnectedCb);
        }
        FeatureFreeInstanceHandle(mHandler);
        mHandler = nullptr;
    }

    void onConnected(const sp<android::IBinder>& server)
    {
        if (mHandler && !FeatureInstanceIsDetached(mHandler)) {
#ifdef CONFIG_QUICKAPP_ACTIVITY_ASYNC
            if (isFeatureLoopValid()) {
                CallData* data = new CallData;
                data->mHandle = mHandler;
                data->mId = mOnConnectedCb;
                FeaturePost(
                    mHandler, [](int mode, void* callbackData) {
                        CallData* dataPtr = (CallData*)callbackData;
                        FeatureInvokeCallback(dataPtr->mHandle, dataPtr->mId);
                        delete dataPtr;
                    },
                    data);
            } else {
                FeatureInvokeCallback(mHandler, mOnConnectedCb);
            }
#else
            FeatureInvokeCallback(mHandler, mOnConnectedCb);
#endif
        }
    }

    void onDisconnected(const sp<android::IBinder>& server)
    {
        if (mHandler && !FeatureInstanceIsDetached(mHandler)) {
#ifdef CONFIG_QUICKAPP_ACTIVITY_ASYNC
            if (isFeatureLoopValid()) {
                CallData* data = new CallData;
                data->mHandle = mHandler;
                data->mId = mOnDisConnectedCb;
                FeaturePost(
                    mHandler, [](int mode, void* callbackData) {
                        CallData* dataPtr = (CallData*)callbackData;
                        FeatureInvokeCallback(dataPtr->mHandle, dataPtr->mId);
                        delete dataPtr;
                    },
                    data);
            } else {
                FeatureInvokeCallback(mHandler, mOnDisConnectedCb);
            }
#else
            FeatureInvokeCallback(mHandler, mOnDisConnectedCb);
#endif
        }
    }

    // 用于判断在初始化feature环境时,是否传入了loop
    bool isFeatureLoopValid()
    {
        FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(mHandler);
        uv_loop_t* loop = FeatureGetUVLoop(manager);
        if (!loop) {
            FEATURE_LOG_ERROR("loop is null !");
            return false;
        }
        return true;
    }

private:
    FeatureInstanceHandle mHandler;
    FtCallbackId mOnConnectedCb;
    FtCallbackId mOnDisConnectedCb;
};

class ServiceConnectManager {
public:
    ServiceConnectManager(Context* ctx)
        : mBindId(0)
        , mCtx(ctx)
    {
    }
    ~ServiceConnectManager()
    {
        clearServiceConnect();
    }

    int addServiceConnect(const sp<FtServiceConnection>& conn)
    {
        if (conn) {
            mServiceConns.emplace(++mBindId, conn);
            return mBindId;
        }
        return -1;
    }

    sp<FtServiceConnection> removeServiceConnect(const int bindId)
    {
        auto iter = mServiceConns.find(bindId);
        if (iter != mServiceConns.end()) {
            auto serviceConn = iter->second;
            serviceConn->clearFeatureRef();
            mServiceConns.erase(iter);
            return serviceConn;
        }
        return nullptr;
    }

    void clearServiceConnect()
    {
        for (auto it = mServiceConns.begin(); it != mServiceConns.end(); ++it) {
            if (mCtx) {
                mCtx->unbindService(it->second);
                it->second->clearFeatureRef();
            }
        }
        mServiceConns.clear();
    }

private:
    int mBindId;
    std::map<int, sp<FtServiceConnection>> mServiceConns;
    Context* mCtx;
};

// FeatureCallbacks to be implemented
void system_internal_activity_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_internal_activity_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_internal_activity_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    Context* nativeContext = static_cast<Context*>(FeatureInstanceGetManagerUserData(handle, "nativeContext"));
    auto serviceManager = new ServiceConnectManager(nativeContext);
    FeatureSetObjectData(handle, serviceManager);
}

void system_internal_activity_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    ServiceConnectManager* serviceManager = (ServiceConnectManager*)FeatureGetObjectData(handle);
    if (serviceManager) {
        delete serviceManager;
    }
}

void system_internal_activity_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_internal_activity_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented
FtInt system_internal_activity_wrap_startActivity(FeatureInstanceHandle feature,
    AppendData append_data, FtString target,
    FtAny params)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    int ret = -1;
    Context* nativeContext = static_cast<Context*>(FeatureInstanceGetManagerUserData(feature, "nativeContext"));
    if (nativeContext) {
        Intent intent(target);
        ft_context_ref ftCtx = FeatureGetContext(feature);
        const char* jsonStr = ft_to_string(ftCtx, *params);
        intent.setData(jsonStr);
        ret = nativeContext->startActivity(intent);
        ft_free_string(ftCtx, jsonStr);
    } else {
        FEATURE_LOG_ERROR("Can't get nativeContext in Feature user data");
    }
    return ret;
}

FtInt system_internal_activity_wrap_stopActivity(FeatureInstanceHandle feature,
    AppendData append_data, FtString target)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);

    int ret = -1;
    Context* nativeContext = static_cast<Context*>(FeatureInstanceGetManagerUserData(feature, "nativeContext"));
    if (nativeContext) {
        Intent intent(target);
        ret = nativeContext->stopActivity(intent);
    } else {
        FEATURE_LOG_ERROR("Can't get nativeContext in Feature user data");
    }
    return ret;
}

FtInt system_internal_activity_wrap_startService(FeatureInstanceHandle feature,
    AppendData append_data, FtString target,
    FtAny params)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    int ret = -1;
    Context* nativeContext = static_cast<Context*>(FeatureInstanceGetManagerUserData(feature, "nativeContext"));
    if (nativeContext) {
        Intent intent(target);
        ft_context_ref ftCtx = FeatureGetContext(feature);
        const char* jsonStr = ft_to_string(ftCtx, *params);
        intent.setData(jsonStr);
        ret = nativeContext->startService(intent);
        ft_free_string(ftCtx, jsonStr);
    } else {
        FEATURE_LOG_ERROR("Can't get nativeContext in Feature user data");
    }
    return ret;
}
FtInt system_internal_activity_wrap_stopService(FeatureInstanceHandle feature,
    AppendData append_data, FtString target)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    int ret = -1;
    Context* nativeContext = static_cast<Context*>(FeatureInstanceGetManagerUserData(feature, "nativeContext"));
    if (nativeContext) {
        Intent intent(target);
        ret = nativeContext->stopService(intent);
    } else {
        FEATURE_LOG_ERROR("Can't get nativeContext in Feature user data");
    }
    return ret;
}

FtInt system_internal_activity_wrap_bindService(FeatureInstanceHandle feature,
    AppendData append_data, FtString target,
    FtAny params,
    system_internal_activity_ServiceConnection* conn)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    int bindId = -1;
    Context* nativeContext = static_cast<Context*>(FeatureInstanceGetManagerUserData(feature, "nativeContext"));
    if (nativeContext) {
        auto manager = (ServiceConnectManager*)FeatureGetObjectData(feature);
        sp<FtServiceConnection> ftConn = new FtServiceConnection(feature, conn->connectCallBack, conn->disconnectCallBack);
        bindId = manager->addServiceConnect(ftConn);
        if (bindId > 0) {
            Intent intent(target);
            ft_context_ref ftCtx = FeatureGetContext(feature);
            const char* jsonStr = ft_to_string(ftCtx, *params);
            intent.setData(jsonStr);
            nativeContext->bindService(intent, ftConn);
            ft_free_string(ftCtx, jsonStr);
        }
    } else {
        FEATURE_LOG_ERROR("Can't get nativeContext in Feature user data");
    }
    return bindId;
}

void system_internal_activity_wrap_unbindService(FeatureInstanceHandle feature,
    AppendData append_data, FtInt bindId)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    Context* nativeContext = static_cast<Context*>(FeatureInstanceGetManagerUserData(feature, "nativeContext"));
    if (nativeContext) {
        auto manager = (ServiceConnectManager*)FeatureGetObjectData(feature);
        auto ftconn = manager->removeServiceConnect(bindId);
        if (ftconn) {
            nativeContext->unbindService(ftconn);
        }
    } else {
        FEATURE_LOG_ERROR("Can't get nativeContext in Feature user data");
    }
}

FtBool system_internal_activity_wrap_moveToBackground(FeatureInstanceHandle feature,
    AppendData append_data, FtBool nonRoot)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    os::app::Activity* nativeContext = static_cast<os::app::Activity*>(FeatureInstanceGetManagerUserData(feature, "nativeContext"));
    if (nativeContext) {
        return nativeContext->moveToBackground(nonRoot);
    } else {
        FEATURE_LOG_ERROR("Can't get nativeContext in Feature user data");
        return false;
    }
}
