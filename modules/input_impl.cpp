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

#include "input.h"
#include <algorithm>
#include <cctype>
#include <feature_config.h>
#include <feature_exports.h>
#include <graphics/input_gen.h>
#include <string>
#include <unordered_map>
#include <uv.h>

static const char* file_tag = "[jidl_feature] input_impl";

#define CHECK_INPUT_CTX(ctx)                                                           \
    do {                                                                               \
        if (!ctx) {                                                                    \
            FEATURE_LOG_ERROR("%s: failed to get input generator context.", file_tag); \
            return;                                                                    \
        }                                                                              \
    } while (0)

struct InputContext {
    uv_loop_t* loop;
    input_gen_ctx_t ctx;
};

static InputContext* get_input_ctx(FeatureInstanceHandle handle)
{
    return static_cast<InputContext*>(FeatureGetProtoData(FeatureGetProtoHandle(handle)));
}

static int get_button_value(std::string button)
{
    static const std::unordered_map<std::string, int> button_map = {
        { "HOME", 0x01 },
        { "DOWN", 0x02 },
        { "THIRD", 0x04 }
    };
    std::transform(button.begin(), button.end(), button.begin(), ::toupper);
    auto it = button_map.find(button);
    if (it == button_map.end()) {
        FEATURE_LOG_ERROR("%s : Invalid button value '%s'", file_tag, button.c_str());
        return 0;
    }
    return it->second;
}

void system_test_input_onRegister(const char* feature_name)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_test_input_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    auto* ic = static_cast<InputContext*>(FeatureGetProtoData(handle));

    if (ic == nullptr) {
        ic = static_cast<InputContext*>(malloc(sizeof(InputContext)));

        if (!ic) {
            FEATURE_LOG_ERROR("%s : Failed to allocate memory for input context.", file_tag);
            return;
        }

        FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
        ic->loop = FeatureGetUVLoop(manager);

        if (input_gen_create(&ic->ctx, INPUT_GEN_DEV_ALL) < 0) {
            FEATURE_LOG_ERROR("%s : Input generator create failed.", file_tag);
            free(ic);
            return;
        }

        FeatureSetProtoData(handle, ic);
    }
}

void system_test_input_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_test_input_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_test_input_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);

    auto* ic = static_cast<InputContext*>(FeatureGetProtoData(handle));
    if (ic) {
        input_gen_destroy(ic->ctx);
        free(ic);
    }
}

void system_test_input_onUnregister(const char* feature_name)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

template <typename ParamsT>
struct InputReq {
    uv_work_t req;
    FeatureInstanceHandle handle;
    FtPromiseId pid;
    InputContext* ic;
    ParamsT* params;
    int result = -1;
};

template <typename ParamsT>
static void freeInputReq(InputReq<ParamsT>* ir)
{
    FeatureFreeValue(ir->params);
    FeatureFreeInstanceHandle(ir->handle);
    free(ir);
}

template <typename ParamsT>
static int queue_async_work(
    FeatureInstanceHandle feature,
    FtPromiseId pid,
    ParamsT* setup_data,
    const char* operation_name,
    uv_work_cb work_cb)
{
    auto* ic = get_input_ctx(feature);
    if (!ic) {
        FEATURE_LOG_ERROR("%s : Failed to get input generator context.", file_tag);
        FeaturePromiseReject(feature, pid, GENERAL, "Failed to get input generator context");
        return -1;
    }

    auto* req = static_cast<InputReq<ParamsT>*>(malloc(sizeof(InputReq<ParamsT>)));
    if (!req) {
        FEATURE_LOG_ERROR("%s : memory allocation failed.", file_tag);
        FeaturePromiseReject(feature, pid, GENERAL, "malloc fail");
        return -1;
    }

    req->req.data = req;
    req->handle = FeatureDupInstanceHandle(feature);
    req->pid = pid;
    req->ic = ic;
    req->params = (ParamsT*)FeatureDupValue(setup_data);

    int res = uv_queue_work(ic->loop, &req->req, work_cb, [](uv_work_t* _req, int status) {
        auto* ir = static_cast<InputReq<ParamsT>*>(_req->data);

        if (!FeatureInstanceIsDetached(ir->handle)) {
            if (status != 0 || ir->result != 0) {
                FeaturePromiseReject(ir->handle, ir->pid, TASK_FAILED, "Perform error");
            } else {
                FeaturePromiseResolve(ir->handle, ir->pid);
            }
        }

        freeInputReq(ir);
    });

    if (res != 0) {
        FEATURE_LOG_ERROR("%s: execute uv_queue_work fail for %s", file_tag, operation_name);
        FeaturePromiseReject(feature, pid, TASK_FAILED, "uv_queue_work fail");
        freeInputReq(req);
        return -1;
    }

    return 0;
}

void system_test_input_wrap_tap(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_test_input_TapParams* data)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag);

    if (!data) {
        FEATURE_LOG_ERROR("%s : Invalid parameters", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid parameters");
        return;
    }

    if (data->x < 0 || data->y < 0) {
        FEATURE_LOG_ERROR("%s : Invalid parameters", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid parameters");
        return;
    }

    queue_async_work<system_test_input_TapParams>(
        feature,
        pid,
        data,
        "tap",
        [](uv_work_t* req) {
            auto* ir = static_cast<InputReq<system_test_input_TapParams>*>(req->data);
            CHECK_INPUT_CTX(ir->ic->ctx);
            ir->result = input_gen_tap(ir->ic->ctx, ir->params->x, ir->params->y);
        });
}

void system_test_input_wrap_drag(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_test_input_DragParams* data)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s : Invalid parameters.", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid parameters");
        return;
    }

    if (data->startX < 0 || data->startY < 0 || data->endX < 0 || data->endY < 0 || data->duration <= 0) {
        FEATURE_LOG_ERROR("%s : Invalid parameters.", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid parameters");
        return;
    }

    queue_async_work<system_test_input_DragParams>(
        feature,
        pid,
        data,
        "drag",
        [](uv_work_t* req) {
            auto* ir = static_cast<InputReq<system_test_input_DragParams>*>(req->data);
            CHECK_INPUT_CTX(ir->ic->ctx);
            ir->result = input_gen_drag(ir->ic->ctx, ir->params->startX, ir->params->startY, ir->params->endX, ir->params->endY, ir->params->duration);
        });
}

void system_test_input_wrap_swipe(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_test_input_SwipeParams* data)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s : Invalid parameters.", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid parameters");
        return;
    }

    if (data->startX < 0 || data->startY < 0 || data->endX < 0 || data->endY < 0 || data->duration <= 0) {
        FEATURE_LOG_ERROR("%s : Invalid parameters.", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid parameters");
        return;
    }

    queue_async_work<system_test_input_SwipeParams>(
        feature,
        pid,
        data,
        "drag",
        [](uv_work_t* req) {
            auto* ir = static_cast<InputReq<system_test_input_SwipeParams>*>(req->data);
            CHECK_INPUT_CTX(ir->ic->ctx);
            ir->result = input_gen_swipe(ir->ic->ctx, ir->params->startX, ir->params->startY, ir->params->endX, ir->params->endY, ir->params->duration);
        });
}

void system_test_input_wrap_pressSystemButton(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_test_input_PressButtonParams* data)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s : Invalid pressSystemButton parameters", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid pressSystemButton parameters");
        return;
    }

    if (!data->button) {
        FEATURE_LOG_ERROR("%s : Invalid pressSystemButton parameters", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid pressSystemButton parameters");
        return;
    }

    queue_async_work<system_test_input_PressButtonParams>(
        feature,
        pid,
        data,
        "pressSystemButton",
        [](uv_work_t* req) {
            auto* ir = static_cast<InputReq<system_test_input_PressButtonParams>*>(req->data);
            CHECK_INPUT_CTX(ir->ic->ctx);
            ir->result = input_gen_button_click(ir->ic->ctx, get_button_value(ir->params->button));
        });
}

void system_test_input_wrap_longPressSystemButton(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_test_input_LongPressButtonParams* data)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s : Invalid longPressSystemButton parameters", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid longPressSystemButton parameters");
        return;
    }

    if (!data->button || data->duration <= 0) {
        FEATURE_LOG_ERROR("%s : Invalid longPressSystemButton parameters", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid longPressSystemButton parameters");
        return;
    }

    queue_async_work<system_test_input_LongPressButtonParams>(
        feature,
        pid,
        data,
        "longPressSystemButton",
        [](uv_work_t* req) {
            auto* ir = static_cast<InputReq<system_test_input_LongPressButtonParams>*>(req->data);
            CHECK_INPUT_CTX(ir->ic->ctx);
            ir->result = input_gen_button_longpress(ir->ic->ctx, get_button_value(ir->params->button), ir->params->duration);
        });
}

void system_test_input_wrap_wheel(FeatureInstanceHandle feature, AppendData append_data, FtPromiseId pid, system_test_input_WheelParams* data)
{
    if (!data) {
        FEATURE_LOG_ERROR("%s : Invalid wheel parameters", file_tag);
        FeaturePromiseReject(feature, pid, ARGSERROR, "Invalid wheel parameters");
        return;
    }

    // TODO: check data->value

    queue_async_work<system_test_input_WheelParams>(
        feature,
        pid,
        data,
        "wheel",
        [](uv_work_t* req) {
            auto* ir = static_cast<InputReq<system_test_input_WheelParams>*>(req->data);
            CHECK_INPUT_CTX(ir->ic->ctx);
            ir->result = input_gen_mouse_wheel(ir->ic->ctx, ir->params->value);
        });
}
