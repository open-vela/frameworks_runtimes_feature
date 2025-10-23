/*
 * Copyright (C) 2024 Xiaomi Corporation
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

#include "wechat_impl.h"
#include "feature.h"
#include "uv_ext.h"
#include "wechat.h"
#include <cstddef>
#include <unistd.h>

static const char* file_tag = "[jidl_feature] wechat_impl";
#define COPYSTR(dst, src)                                             \
    do {                                                              \
        char* tmp = (char*)FeatureMalloc(strlen(src) + 1, FT_STRING); \
        strcpy(tmp, src);                                             \
        dst = tmp;                                                    \
    } while (0)

struct WechatTask {
    char* resp_body;
    double task_id;
    double error_code;
};

struct WechatEvent {
    char* event;
    char* event_body;
};

struct WechatHandle {
    FeatureInstanceHandle feature;
    FtCallbackId event_cb;
    FtCallbackId task_cb;
    uv_async_queue_t task_async;
    uv_async_queue_t event_async;
};
static WechatHandle* wechat_handle;
static void wechat_free(WechatHandle* handle);
static void async_js_task_callback(uv_async_queue_t* async, void* data);
static void async_js_event_callback(uv_async_queue_t* async, void* data);

static void uv_async_close_cb(uv_handle_t* handle)
{
    uv_async_queue_t* async_queue = (uv_async_queue_t*)handle;
    WechatHandle* wechat = (WechatHandle*)async_queue->data;
    if (wechat) {
        free(wechat);
        wechat = NULL;
    }
}

static void uv_async_queue_close_cb(uv_handle_t* handle)
{
    uv_async_queue_t* async_queue = (uv_async_queue_t*)handle;
    WechatHandle* wechat = (WechatHandle*)async_queue->data;
    uv_async_queue_close(&wechat->event_async, uv_async_close_cb);
}

void service_wechat_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void service_wechat_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    WechatHandle* wechat = static_cast<WechatHandle*>(calloc(1, sizeof(WechatHandle)));
    wechat->task_async.data = wechat;
    wechat->event_async.data = wechat;
    wechat->event_cb = -1;
    wechat->task_cb = -1;
    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    uv_async_queue_init(FeatureGetUVLoop(manager), &wechat->task_async, async_js_task_callback);
    uv_async_queue_init(FeatureGetUVLoop(manager), &wechat->event_async, async_js_event_callback);
    wechat_handle = wechat;
    FeatureSetProtoData(handle, wechat);
}

void service_wechat_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    WechatHandle* wechat = static_cast<WechatHandle*>(FeatureGetProtoData(proto_handle));
    if (wechat->feature) {
        FEATURE_LOG_ERROR("[Regist Event] wechat feature already exist");
    }
    wechat->feature = handle;
}

void service_wechat_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    wechat_handle->feature = NULL;
    wechat_handle = NULL; // clear global pointer
}

void service_wechat_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    WechatHandle* wechat = static_cast<WechatHandle*>(FeatureGetProtoData(handle));
    FeatureSetProtoData(handle, NULL);
    wechat_free(wechat);
}

void service_wechat_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

static void wechat_free(WechatHandle* handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->feature) {
        FeatureRemoveCallback(handle->feature, handle->event_cb);
        FeatureRemoveCallback(handle->feature, handle->task_cb);
    }

    uv_async_queue_close(&handle->task_async, uv_async_queue_close_cb);
}

static void OnJsEvent(const char* event, const char* event_body)
{
    if (!event || !event_body) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }

    if (wechat_handle == NULL) {
        FEATURE_LOG_INFO("%s wechat handle is null", __FUNCTION__);
        return;
    }

    struct WechatEvent* wechat_event = (struct WechatEvent*)malloc(sizeof(struct WechatEvent));
    wechat_event->event = strdup(event);
    wechat_event->event_body = strdup(event_body);
    uv_async_queue_send(&wechat_handle->event_async, (void*)wechat_event);
}

static void async_js_event_callback(uv_async_queue_t* async, void* data)
{
    struct WechatEvent* wechat_event = (struct WechatEvent*)data;
    if (wechat_event == NULL) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    service_wechat_eventData* event_data = service_wechatMalloceventData();

    COPYSTR(event_data->event, wechat_event->event);
    COPYSTR(event_data->event_body, wechat_event->event_body);

    FeatureInvokeCallback(wechat_handle->feature, wechat_handle->event_cb, event_data);
    FEATURE_LOG_INFO("[wechat] OnJsEvent exit");
    free(wechat_event->event);
    free(wechat_event->event_body);
    free(wechat_event);
    FeatureFreeValue(event_data);
}

static void OnJsTask(double task_id, double error_code, const char* resp_body)
{
    if (resp_body == NULL) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }

    if (wechat_handle == NULL) {
        FEATURE_LOG_INFO("%s wechat handle is null", __FUNCTION__);
        return;
    }

    struct WechatTask* wechat_task = (struct WechatTask*)malloc(sizeof(*wechat_task));
    wechat_task->task_id = task_id;
    wechat_task->error_code = error_code;
    wechat_task->resp_body = strdup(resp_body);
    FEATURE_LOG_INFO("[wechat] OnJsTask call %p, task_id:%d, error_code:%d, resp_body:%s",
        wechat_task, (uint32_t)task_id, (uint32_t)error_code, resp_body);
    uv_async_queue_send(&wechat_handle->task_async, (void*)wechat_task);
}

static void async_js_task_callback(uv_async_queue_t* async, void* data)
{
    struct WechatTask* wechat_task = static_cast<struct WechatTask*>(data);
    if (wechat_task == NULL) {
        FEATURE_LOG_ERROR("%s Invalid arguments", __FUNCTION__);
        return;
    }
    service_wechat_taskData* task_data = service_wechatMalloctaskData();
    task_data->task_id = wechat_task->task_id;
    task_data->error_code = wechat_task->error_code;
    COPYSTR(task_data->resp_body, wechat_task->resp_body);

    FEATURE_LOG_INFO("[wechat] task_callback entry:%p, task_id:%d, error_code:%d, resp_body:%s",
        wechat_task, (uint32_t)wechat_task->task_id, (uint32_t)wechat_task->error_code, wechat_task->resp_body);

    FeatureInvokeCallback(wechat_handle->feature, wechat_handle->task_cb, task_data);
    FEATURE_LOG_INFO("[wechat] OnJsTask exit");
    free(wechat_task->resp_body);
    free(wechat_task);
    FeatureFreeValue(task_data);
}

void service_wechat_wrap_js_invoke_function(FeatureInstanceHandle feature, AppendData append_data, service_wechat_invokeInfo* info)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
    if (!info) {
        FEATURE_LOG_ERROR("[Invoke Function] invaild arguments");
        return;
    }

    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    WechatHandle* wechat = static_cast<WechatHandle*>(FeatureGetProtoData(proto_handle));
    if (wechat == NULL) {
        FEATURE_LOG_ERROR("[Invoke Function] wechat handle malloc failed");
        return;
    }

    double task_id;
    task_id = info->task_id;

    char* func_name = (char*)FeatureMalloc(strlen(info->func_name) + 1, FT_STRING);
    sprintf(func_name, "%s", info->func_name);

    char* request_body = (char*)FeatureMalloc(strlen(info->request_body) + 1, FT_STRING);
    sprintf(request_body, "%s", info->request_body);

    FEATURE_LOG_INFO("[wechat] invoke entry task_id=%d func_name:%s, request_body=%s",
        (uint32_t)task_id, func_name, request_body);
    adam::js_invoke_function(task_id, func_name, request_body);
    FEATURE_LOG_INFO("[wechat] invoke exit");
    FeatureFreeValue(func_name);
    FeatureFreeValue(request_body);
}

void service_wechat_wrap_js_regist_task_callback(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId task_cb)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    WechatHandle* wechat = static_cast<WechatHandle*>(FeatureGetProtoData(proto_handle));
    if (wechat == NULL || task_cb == 0) {
        FEATURE_LOG_ERROR("[Regist Task] wechat handle malloc failed");
        return;
    }

    FeatureRemoveCallback(wechat->feature, wechat->task_cb);
    wechat->task_cb = task_cb;
    FEATURE_LOG_INFO("[wechat] regist task callback");
    adam::js_regist_task_callback(OnJsTask);
}

void service_wechat_wrap_js_regist_event_callback(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId event_cb)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    WechatHandle* wechat = static_cast<WechatHandle*>(FeatureGetProtoData(proto_handle));
    if (wechat == NULL || event_cb == 0) {
        FEATURE_LOG_ERROR("[Regist Event] wechat handle malloc failed");
        return;
    }

    FeatureRemoveCallback(wechat->feature, wechat->event_cb);
    wechat->event_cb = event_cb;
    FEATURE_LOG_INFO("[wechat] regist event callback");
    adam::js_regist_event_callback(OnJsEvent);
}

void service_wechat_wrap_js_unregist_task_callback(FeatureInstanceHandle feature, AppendData append_data)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    WechatHandle* wechat = static_cast<WechatHandle*>(FeatureGetProtoData(proto_handle));
    if (wechat == NULL) {
        FEATURE_LOG_ERROR("[Unregist Task] wechat handle malloc failed");
        return;
    }

    FEATURE_LOG_INFO("[wechat] unregist event callback");
    FeatureRemoveCallback(wechat->feature, wechat->task_cb);
    wechat->task_cb = 0;
    adam::js_unregist_task_callback();
}

void service_wechat_wrap_js_unregist_event_callback(FeatureInstanceHandle feature, AppendData append_data)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    WechatHandle* wechat = static_cast<WechatHandle*>(FeatureGetProtoData(proto_handle));
    if (wechat == NULL) {
        FEATURE_LOG_ERROR("[Unregist Event] wechat handle malloc failed");
        return;
    }

    FeatureRemoveCallback(wechat->feature, wechat->event_cb);
    wechat->event_cb = 0;
    FEATURE_LOG_INFO("[wechat] unregist event callback");
    adam::js_unregist_event_callback();
}