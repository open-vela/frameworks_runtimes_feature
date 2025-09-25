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

#include <list>
#include <nuttx/nuttx.h>

#include "cJSON.h"
#include "topics/algo_heartrate.h"

#include "health.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] health_impl";
#define INVOKE_SUCCESS_CB(feature, cb, ...)                        \
    do {                                                           \
        if (!FeatureInvokeCallback(feature, cb, ##__VA_ARGS__)) {  \
            FEATURE_LOG_ERROR("invoke success callback failed !"); \
        }                                                          \
    } while (0)

#define INVOKE_FAIL_CB(feature, cb, msg, code)                  \
    do {                                                        \
        if (!FeatureInvokeCallback(feature, cb, msg, code)) {   \
            FEATURE_LOG_ERROR("invoke fail callback failed !"); \
        }                                                       \
    } while (0)

#define REMOVE_ALL_CALLBACK(feature, __succ__, __fail__) \
    do {                                                 \
        FeatureRemoveCallback(feature, __succ__);        \
        FeatureRemoveCallback(feature, __fail__);        \
    } while (0)

#define HEALTH_INFO_INIT(magic_, index_, name_, meta_) \
    [magic_] = { .index = index_, .name = name_, .meta = meta_ }

typedef enum HealthMagic {
    HEALTH_MAGIC_HEARTRATE,
    HEALTH_MAGIC_NUM,
} HealthMagic;

struct HealthOrb {
    int index;
    const char* name;
    orb_id_t meta;
};

struct HealthGetMeta {
    FtPromiseId pid = 0;
    int num;
    FtArray* array;
};

struct HealthMetaData {
    FeatureInstanceHandle instance = 0;
    FtCallbackId callback = 0;
    FtCallbackId fail = 0;
    HealthGetMeta* getMeta = nullptr;
};

struct HealthContext;

struct HealthTopic {
    HealthContext* context;
    HealthMagic magic;
    std::list<uv_topic_t*> topic_list;
    std::list<HealthMetaData> getList;
    std::list<HealthMetaData> subList;
};
struct HealthContext {
    HealthTopic* event[HEALTH_MAGIC_NUM];
    ft_context_ref ft_ctx;
    int ref_count;
    bool isdestory;
};

const static HealthOrb health_orb_table[HEALTH_MAGIC_NUM] = {
    HEALTH_INFO_INIT(HEALTH_MAGIC_HEARTRATE, 0, "HEART_RATE", ORB_ID(algo_heartrate)),
};

static HealthMagic getHealthMagic(int type)
{
    for (int i = 0; i < HEALTH_MAGIC_NUM; i++) {
        if (health_orb_table[i].index == type) {
            return (HealthMagic)i;
        }
    }

    return HEALTH_MAGIC_NUM;
}

static bool health_sensor_is_active(HealthTopic* topic)
{
    return !topic->getList.empty() || !topic->subList.empty();
}

static std::list<HealthMetaData>::iterator health_find_meta(HealthTopic* event, FeatureInstanceHandle instance)
{
    for (auto it = event->subList.begin(); it != event->subList.end(); ++it) {
        if (it->instance == instance) {
            return it;
        }
    }
    return event->subList.end();
}

static void health_topic_close_cb(uv_handle_t* handle)
{
    uv_topic_t* topic = (uv_topic_t*)handle;
    HealthTopic* event = static_cast<HealthTopic*>(topic->user_data);
    HealthContext* context = event->context;
    context->ref_count--;
    delete (topic);
    if (!context->ref_count && context->isdestory) {
        FEATURE_LOG_ERROR("%s::%s() delete context", file_tag, __FUNCTION__);
        delete context;
    }
}

static void health_topic_cb(uv_topic_t* topic, int status, void* data, size_t datalen)
{
    int ret;
    HealthTopic* event = static_cast<HealthTopic*>(topic->user_data);
    cJSON* root = nullptr;
    cJSON* root_ = nullptr;
    char* json_str = nullptr;
    FtJsonObject rs = nullptr;

    switch (event->magic) {
    case HEALTH_MAGIC_HEARTRATE: {
        size_t cnt = datalen / sizeof(algo_heartrate);
        for (size_t i = 0; i < cnt; i++) {
            struct algo_heartrate* hr_data = (struct algo_heartrate*)data + i;

            root = cJSON_CreateObject();
            root_ = cJSON_CreateObject();

            cJSON_AddNumberToObject(root_, "value", hr_data->bpm);
            cJSON_AddNumberToObject(root_, "timeStamp", hr_data->timestamp_us / 1000);
            cJSON_AddItemToObject(root, "data", root_);
            json_str = cJSON_Print(root);
            rs = FeatureNewJsonObject(json_str);
            for (auto it = event->subList.begin(); it != event->subList.end(); it++) {
                INVOKE_SUCCESS_CB(it->instance, it->callback, rs);
            }

            if (i != cnt - 1 || event->getList.empty()) {
                FeatureFreeValue(rs);
                cJSON_Delete(root);
                rs = nullptr;
                json_str = nullptr;
                root = nullptr;
            }

            free(json_str);
        }

        break;
    }
    default:
        break;
    }

    if (!event->getList.empty()) {
        HealthMetaData meta = event->getList.front();
        HealthGetMeta* getMeta = meta.getMeta;

        cJSON_AddNumberToObject(root, "dataType", health_orb_table[event->magic].index);
        json_str = cJSON_Print(root);
        rs = FeatureNewJsonObject(json_str);
        FeatureArrayAppend(getMeta->array, rs);
        if (!--getMeta->num) {
            FeaturePromiseResolve(meta.instance, getMeta->pid, getMeta->array);
            FeatureFreeValue(getMeta->array);
            delete (getMeta);
        }
        event->getList.pop_front();
        FeatureFreeValue(rs);
        free(json_str);
        cJSON_Delete(root);
    }

    if (!health_sensor_is_active(event) && !event->topic_list.empty()) {
        uv_topic_t* free_topic = event->topic_list.front();
        event->topic_list.pop_front();
        ret = uv_topic_unsubscribe(free_topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_unsubscribe fail", file_tag, __FUNCTION__);
        }

        ret = uv_topic_close(free_topic, health_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
        }
        return;
    }
}

void service_health_wrap_getRecentSamples(FeatureInstanceHandle handle, AppendData append_data, FtPromiseId pid, service_health_getRecentSamplesParam* param)
{
    int ret;
    int code;
    const char* msg = "";
    HealthMagic magic;
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    HealthContext* context = static_cast<HealthContext*>(FeatureGetProtoData(proto_handle));
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(handle);
    FtArray* types = param->dataTypes;
    int len = FeatureArrayGetLength(types);
    HealthGetMeta* getMeta;
    std::vector<std::pair<HealthTopic*, HealthMetaData>> tmpList;

    if (!len) {
        FeaturePromiseReject(handle, pid, FT_ERR_ARGS, "dataTypes is empty");
        return;
    }

    getMeta = new HealthGetMeta();
    if (!getMeta) {
        FeaturePromiseReject(handle, pid, FT_ERR_GENERAL, "malloc getMeta failed");
        return;
    }

    getMeta->num = len;
    getMeta->pid = pid;
    for (int i = 0; i < len; i++) {
        HealthTopic* event;
        HealthMetaData meta;
        int type = *(int*)FeatureArrayGetData(types, i);

        magic = getHealthMagic(type);
        if (magic == HEALTH_MAGIC_NUM) {
            getMeta->num--;
            continue;
        }

        ret = orb_exists(health_orb_table[magic].meta, 0);
        if (ret < 0) {
            getMeta->num--;
            continue;
        }

        event = context->event[magic];
        if (!event) {
            event = new HealthTopic();
            if (!event) {
                code = FT_ERR_GENERAL;
                msg = "malloc event failed";
                goto errout;
            }

            event->context = context;
            event->magic = magic;
        }

        if (!health_sensor_is_active(event)) {
            uv_topic_t* topic = new uv_topic_t();
            if (!topic) {
                code = FT_ERR_GENERAL;
                msg = "malloc topic failed";
                goto errout;
            }

            ret = uv_topic_subscribe(FeatureGetUVLoop(manager), topic,
                health_orb_table[magic].meta,
                health_topic_cb);
            if (ret < 0) {
                delete (topic);
                code = FT_ERR_GENERAL;
                msg = "subscribe error";
                goto errout;
            }

            topic->user_data = (void*)event;
            event->topic_list.push_back(topic);
            context->ref_count++;
        }

        meta.instance = handle;
        meta.getMeta = getMeta;
        tmpList.emplace_back(event, meta);
    }

    if (getMeta->num == 0) {
        code = FT_ERR_ARGS;
        msg = "no valid dataTypes to subscribe";
        goto errout;
    }

    for (auto& p : tmpList) {
        p.first->getList.push_back(p.second);
    }
    getMeta->array = FeatureCreateArray(handle, getMeta->num, FT_JSON_OBJ);

    return;

errout:
    FeatureFreeValue(getMeta->array);
    delete (getMeta);
    FeaturePromiseReject(handle, pid, code, msg);
}

void service_health_wrap_subscribeSample(FeatureInstanceHandle handle,
    AppendData adata,
    service_health_subscribeParam* param)
{
    int ret;
    int code;
    const char* msg = "";
    HealthMagic magic;
    HealthMetaData meta = {};
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(handle);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    HealthContext* context = static_cast<HealthContext*>(FeatureGetProtoData(proto_handle));

    if (!FeatureCheckCallbackId(handle, param->callback)) {
        code = FT_ERR_ARGS;
        msg = "callback id is invalid";
        if (param->fail) {
            INVOKE_FAIL_CB(handle, param->fail, msg, code);
        }
        REMOVE_ALL_CALLBACK(handle, param->callback, param->fail);
        return;
    }

    magic = getHealthMagic(param->dataType);
    if (magic == HEALTH_MAGIC_NUM) {
        code = FT_ERR_ARGS;
        msg = "data type is invalid";
        if (param->fail) {
            INVOKE_FAIL_CB(handle, param->fail, msg, code);
        }
        REMOVE_ALL_CALLBACK(handle, param->callback, param->fail);
        return;
    }

    if (!context->event[magic]) {
        context->event[magic] = new HealthTopic();
        if (!context->event[magic]) {
            code = FT_ERR_GENERAL;
            msg = "malloc event failed";
            if (param->fail) {
                INVOKE_FAIL_CB(handle, param->fail, msg, code);
            }
            REMOVE_ALL_CALLBACK(handle, param->callback, param->fail);
            return;
        }

        context->event[magic]->context = context;
        context->event[magic]->magic = magic;
    }

    std::list<HealthMetaData>::iterator it = health_find_meta(context->event[magic], handle);
    if (it != context->event[magic]->subList.end()) {
        REMOVE_ALL_CALLBACK(handle, it->callback, it->fail);
        it->callback = param->callback;
        it->fail = param->fail;
        return;
    }

    if (!health_sensor_is_active(context->event[magic])) {
        uv_topic_t* topic = new uv_topic_t();
        if (!topic) {
            code = FT_ERR_GENERAL;
            msg = "malloc topic failed";
            if (param->fail) {
                INVOKE_FAIL_CB(handle, param->fail, msg, code);
            }
            REMOVE_ALL_CALLBACK(handle, param->callback, param->fail);
            return;
        }

        ret = uv_topic_subscribe(FeatureGetUVLoop(manager),
            topic,
            health_orb_table[magic].meta,
            health_topic_cb);
        if (ret < 0) {
            delete topic;
            code = FT_ERR_GENERAL;
            msg = "subscribe error";
            if (param->fail) {
                INVOKE_FAIL_CB(handle, param->fail, msg, code);
            }
            REMOVE_ALL_CALLBACK(handle, param->callback, param->fail);
            return;
        }

        topic->user_data = (void*)context->event[magic];
        context->event[magic]->topic_list.push_back(topic);
        context->ref_count++;
    }

    meta.instance = handle;
    meta.callback = param->callback;
    meta.fail = param->fail;
    context->event[magic]->subList.push_back(meta);
}

static void unsubscribe(HealthTopic* event, FeatureInstanceHandle handle)
{
    int ret;
    auto it = health_find_meta(event, handle);
    if (it == event->subList.end()) {
        return;
    }

    REMOVE_ALL_CALLBACK(handle, it->callback, it->fail);
    event->subList.erase(it);
    if (!health_sensor_is_active(event) && !event->topic_list.empty()) {
        uv_topic_t* free_topic = event->topic_list.front();
        event->topic_list.pop_front();
        ret = uv_topic_unsubscribe(free_topic);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s topic unsubscribe failed", __FUNCTION__);
        }

        ret = uv_topic_close(free_topic, health_topic_close_cb);
        if (ret < 0) {
            FEATURE_LOG_ERROR("%s::%s() uv_topic_close fail", file_tag, __FUNCTION__);
        }
    }
}

void service_health_wrap_unsubscribeSample(FeatureInstanceHandle handle, AppendData adata, FtJsonObject dataType)
{
    HealthMagic magic;
    HealthTopic* event;
    int datatype;
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    HealthContext* context = (HealthContext*)FeatureGetProtoData(proto_handle);
    const char* str = FeatureGetJsonString(dataType);
    cJSON* json_str = cJSON_Parse(str);
    cJSON* json_num = cJSON_GetObjectItem(json_str, "dataType");
    if (!cJSON_IsNumber(json_num)) {
        return;
    }

    datatype = json_num->valueint;
    magic = getHealthMagic(datatype);
    if (magic == HEALTH_MAGIC_NUM) {
        return;
    }

    event = context->event[magic];
    if (event) {
        unsubscribe(event, handle);
    }
}

void service_health_wrap_getHr(FeatureInstanceHandle handle, AppendData adata)
{
}

void service_health_wrap_getTodayCalorie(FeatureInstanceHandle handle, AppendData adata)
{
}

void service_health_onRegister(const char* feature_name)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void service_health_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    HealthContext* context = new HealthContext();
    if (!context) {
        FEATURE_LOG_ERROR("malloc failed !");
        return;
    }

    for (int i = 0; i < HEALTH_MAGIC_NUM; i++) {
        context->event[i] = NULL;
    }

    FeatureSetProtoData(handle, context);
}

void service_health_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FeatureProtoHandle protohandle = FeatureGetProtoHandle(handle);
    HealthContext* context = static_cast<HealthContext*>(FeatureGetProtoData(protohandle));
    if (!context->ft_ctx) {
        context->ft_ctx = FeatureGetContext(handle);
    }

    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void service_health_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(handle);
    HealthContext* context = (HealthContext*)FeatureGetProtoData(proto_handle);
    for (int i = 0; i < HEALTH_MAGIC_NUM; i++) {
        HealthTopic* event = context->event[i];
        if (event) {
            event->getList.remove_if([&](const auto& elem) {
                return elem.instance == handle;
            });

            unsubscribe(event, handle);
        }
    }
}

void service_health_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    int ret;
    HealthContext* context = (HealthContext*)FeatureGetProtoData(handle);
    context->isdestory = true;
    for (int i = 0; i < HEALTH_MAGIC_NUM; i++) {
        HealthTopic* event = context->event[i];
        if (event) {
            event->getList.clear();
            while (!event->topic_list.empty()) {
                uv_topic_t* topic = event->topic_list.front();
                event->topic_list.pop_front();
                ret = uv_topic_unsubscribe(topic);
                if (ret < 0) {
                    FEATURE_LOG_ERROR("Failed to unsubscribe topic\n");
                }

                ret = uv_topic_close(topic, health_topic_close_cb);
                if (ret < 0) {
                    FEATURE_LOG_ERROR("Failed to close topic\n");
                }
            }
        }
    }
}

void service_health_onUnregister(const char* feature_name)
{
}