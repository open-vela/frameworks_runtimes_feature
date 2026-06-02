/*
 * Copyright (C) 2026 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <mutex>
#include <atomic>

#include "velaclaw.h"
#include "uv_ext.h"
#include "uv_async_queue.h"
#include "velaclaw_quickapp_bridge.h"

#include <netutils/cJSON.h>

#define TAG "[velaclaw_impl] "

#define VELACLAW_DEBUG(fmt, ...) FEATURE_LOG_DEBUG(TAG fmt, ##__VA_ARGS__)
#define VELACLAW_INFO(fmt, ...)  FEATURE_LOG_INFO(TAG fmt, ##__VA_ARGS__)
#define VELACLAW_ERROR(fmt, ...) FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

/* -- Error codes (matching JS API spec) ----------------------- */
enum VelaclawErrorCode {
    VELACLAW_ERR_GENERAL           = 200,
    VELACLAW_ERR_PARAM             = 202,
    VELACLAW_ERR_NOT_SUPPORTED     = 203,
    VELACLAW_ERR_TIMEOUT           = 204,
    VELACLAW_ERR_SERVICE_UNAVAIL   = 1000,
    VELACLAW_ERR_CONTENT_REJECTED  = 1001,
};

/* -- Pending request tracking --------------------------------- */

static std::atomic<uint32_t> s_req_id{0};

struct PendingAsk {
    FeatureInstanceHandle feature;
    FtPromiseId pid;
    ft_context_ref ft_ctx;
};

/* Maps chat_id -> pending request info */
static std::mutex s_ask_mutex;
static std::map<std::string, PendingAsk> s_pending_asks;

/* -- VelaclawContext (per-proto) ------------------------------- */

struct VelaclawContext {
    uv_async_queue_t async_queue;
    bool initialized;
};

/* -- Async reply data (passed via uv_async_queue) ------------- */

struct AsyncReply {
    char *chat_id;
    char *content;
    char *extra_info;
    char *tool_calls;
};

/* -- Helper: allocate a FeatureMalloc string ------------------ */

static char* ft_strdup(const char* src)
{
    if (!src) return nullptr;
    size_t len = strlen(src);
    char* s = (char*)FeatureMalloc(len + 1, FT_STRING);
    if (s) {
        memcpy(s, src, len);
        s[len] = '\0';
    }
    return s;
}

/* -- Async queue callback (runs on JS/UV loop thread) --------- */

static void async_reply_cb(uv_async_queue_t* queue, void* data)
{
    AsyncReply* reply = static_cast<AsyncReply*>(data);
    if (!reply) return;

    VELACLAW_INFO("async_reply_cb: chat_id=%s content_len=%d tc=%s",
                  reply->chat_id ? reply->chat_id : "null",
                  reply->content ? (int)strlen(reply->content) : 0,
                  reply->tool_calls ? "yes" : "no");

    {
        std::lock_guard<std::mutex> lock(s_ask_mutex);
        auto it = s_pending_asks.find(reply->chat_id ? reply->chat_id : "");
        if (it != s_pending_asks.end()) {
            PendingAsk ask = it->second;
            s_pending_asks.erase(it);

            if (!FeatureInstanceIsDetached(ask.feature)) {
                system_velaclaw_AskResponse* response = system_velaclawMallocAskResponse();
                if (response) {
                    response->reply = ft_strdup(reply->content ? reply->content : "");

                    /* Fill extra_info if present */
                    if (reply->extra_info && reply->extra_info[0]) {
                        response->extra_info = ft_strdup(reply->extra_info);
                    } else {
                        response->extra_info = NULL;
                    }

                    /* Parse tool_calls JSON array into ToolCallResult[] */
                    int tc_count = 0;
                    cJSON* tc_arr = nullptr;
                    if (reply->tool_calls && reply->tool_calls[0]) {
                        tc_arr = cJSON_Parse(reply->tool_calls);
                        if (tc_arr && cJSON_IsArray(tc_arr)) {
                            tc_count = cJSON_GetArraySize(tc_arr);
                        }
                    }

                    if (tc_count > 0) {
                        FtArray* arr = system_velaclaw_malloc_ToolCallResult_struct_type_array();
                        if (arr) {
                            for (int i = 0; i < tc_count; i++) {
                                cJSON* tc = cJSON_GetArrayItem(tc_arr, i);
                                system_velaclaw_ToolCallResult* item =
                                    system_velaclawMallocToolCallResult();
                                if (!item) continue;

                                cJSON* name_item = cJSON_GetObjectItem(tc, "name");
                                item->name = ft_strdup(
                                    (name_item && cJSON_IsString(name_item))
                                    ? name_item->valuestring : "");

                                /* result is a JSON object - serialize to string */
                                cJSON* result_item = cJSON_GetObjectItem(tc, "result");
                                if (result_item) {
                                    char* rs = cJSON_PrintUnformatted(result_item);
                                    item->result = ft_strdup(rs ? rs : "");
                                    free(rs);
                                } else {
                                    item->result = ft_strdup("");
                                }

                                FeatureArrayAppend(arr, item);
                            }
                            response->tool_calls = arr;
                        }
                    } else {
                        /* Default to empty array so JS always gets [] */
                        response->tool_calls =
                            system_velaclaw_malloc_ToolCallResult_struct_type_array();
                    }

                    if (tc_arr) cJSON_Delete(tc_arr);

                    FeaturePromiseResolve(ask.feature, ask.pid, response);
                    FeatureFreeValue(response);
                } else {
                    FeaturePromiseReject(ask.feature, ask.pid,
                        VELACLAW_ERR_GENERAL, "Failed to allocate response");
                }
            }
            FeatureFreeInstanceHandle(ask.feature);
            free(reply->chat_id);
            free(reply->content);
            free(reply->extra_info);
            free(reply->tool_calls);
            free(reply);
            return;
        }
    }

    free(reply->chat_id);
    free(reply->content);
    free(reply->extra_info);
    free(reply->tool_calls);
    free(reply);
}

/* -- Bridge callback (called from bridge receiver thread) ----- */

static VelaclawContext* s_proto_ctx = nullptr;

static void quickapp_reply_handler(const char* chat_id, const char* content,
                                   const char* extra_info,
                                   const char* tool_calls, void* userdata)
{
    VELACLAW_INFO("reply_handler: chat_id=%s content_len=%d tc=%s",
                  chat_id ? chat_id : "null",
                  content ? (int)strlen(content) : 0,
                  tool_calls ? "yes" : "no");

    AsyncReply* reply = static_cast<AsyncReply*>(malloc(sizeof(AsyncReply)));
    if (!reply) {
        VELACLAW_ERROR("reply_handler: failed to allocate AsyncReply");
        return;
    }
    reply->chat_id = strdup(chat_id ? chat_id : "");
    reply->content = strdup(content ? content : "");
    reply->extra_info = extra_info && extra_info[0] ? strdup(extra_info) : NULL;
    reply->tool_calls = (tool_calls && tool_calls[0]) ? strdup(tool_calls) : nullptr;

    VelaclawContext* ctx = static_cast<VelaclawContext*>(userdata);
    if (ctx && ctx->initialized) {
        uv_async_queue_send(&ctx->async_queue, reply);
    } else {
        VELACLAW_ERROR("reply_handler: context not initialized, dropping reply");
        free(reply->chat_id);
        free(reply->content);
        free(reply->extra_info);
        free(reply->tool_calls);
        free(reply);
    }
}

/* -- Lifecycle callbacks --------------------------------------- */

void system_velaclaw_onRegister(const char* feature_name)
{
    VELACLAW_DEBUG("onRegister: %s", feature_name);
}

void system_velaclaw_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    VELACLAW_DEBUG("onCreate");
    VelaclawContext* context = new VelaclawContext();
    if (!context) {
        VELACLAW_ERROR("Failed to allocate VelaclawContext");
        return;
    }
    context->initialized = false;
    context->async_queue.data = context;

    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    int ret = uv_async_queue_init(FeatureGetUVLoop(manager),
                                  &context->async_queue, async_reply_cb);
    if (ret != 0) {
        VELACLAW_ERROR("uv_async_queue_init failed: %d", ret);
        delete context;
        return;
    }
    context->initialized = true;

    s_proto_ctx = context;
    velaclaw_quickapp_bridge_register(quickapp_reply_handler, context);

    FeatureSetProtoData(handle, context);
}

void system_velaclaw_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    VELACLAW_DEBUG("onRequired");
}

void system_velaclaw_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    VELACLAW_DEBUG("onDetached");

    /* Clean up pending asks for this instance */
    {
        std::lock_guard<std::mutex> lock(s_ask_mutex);
        for (auto it = s_pending_asks.begin(); it != s_pending_asks.end(); ) {
            if (it->second.feature == handle) {
                FeatureFreeInstanceHandle(it->second.feature);
                it = s_pending_asks.erase(it);
            } else {
                ++it;
            }
        }
    }
}

static void async_close_cb(uv_handle_t* handle)
{
    uv_async_queue_t* queue = reinterpret_cast<uv_async_queue_t*>(handle);
    VelaclawContext* ctx = static_cast<VelaclawContext*>(queue->data);
    if (ctx) {
        delete ctx;
    }
}

void system_velaclaw_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    VELACLAW_DEBUG("onDestroy");
    VelaclawContext* context = static_cast<VelaclawContext*>(FeatureGetProtoData(handle));
    if (context) {
        velaclaw_quickapp_bridge_unregister();
        s_proto_ctx = nullptr;
        context->initialized = false;
        uv_async_queue_close(&context->async_queue, async_close_cb);
        FeatureSetProtoData(handle, nullptr);
    }
}

void system_velaclaw_onUnregister(const char* feature_name)
{
    VELACLAW_DEBUG("onUnregister: %s", feature_name);
}

/* -- API: ask ------------------------------------------------- */

void system_velaclaw_wrap_ask(FeatureInstanceHandle feature, AppendData append_data,
                               FtPromiseId pid, system_velaclaw_AskParam* param)
{
    VELACLAW_INFO("ask: query=%s", param->query ? param->query : "null");

    ft_context_ref ft_ctx = FeatureGetContext(feature);

    /* Validate query */
    if (!param->query || strlen(param->query) == 0) {
        FeaturePromiseReject(feature, pid, VELACLAW_ERR_PARAM, "query is required");
        return;
    }

    /* Generate unique chat_id for this request */
    uint32_t id = s_req_id.fetch_add(1);
    char chat_id[64];
    snprintf(chat_id, sizeof(chat_id), "qapp_ask_%u", id);

    /* Store pending request */
    {
        std::lock_guard<std::mutex> lock(s_ask_mutex);
        PendingAsk ask;
        ask.feature = FeatureDupInstanceHandle(feature);
        ask.pid = pid;
        ask.ft_ctx = ft_ctx;
        s_pending_asks[chat_id] = ask;
        VELACLAW_INFO("ask: pending count=%d chat_id=%s",
                      (int)s_pending_asks.size(), chat_id);
    }

    /* Send to agent via bridge */
    int ret = velaclaw_quickapp_bridge_ask(chat_id, param->query);
    VELACLAW_INFO("ask: bridge_ask returned %d", ret);
    if (ret != 0) {
        std::lock_guard<std::mutex> lock(s_ask_mutex);
        auto it = s_pending_asks.find(chat_id);
        if (it != s_pending_asks.end()) {
            FeatureFreeInstanceHandle(it->second.feature);
            s_pending_asks.erase(it);
        }
        FeaturePromiseReject(feature, pid, VELACLAW_ERR_SERVICE_UNAVAIL,
                            "Failed to send query to AI agent");
    }
}
