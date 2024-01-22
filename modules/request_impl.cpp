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

#include "app_path.h"
#include "feature.h"
#include "feature_config.h"
#include "feature_context_qjs.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "request.h"
#include "uv_ext.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <map>
#include <time.h>
#include <type_traits>

#define REQUEST_CANCEL 2
#define check_any(ptr) ((ptr) && (ft_get_type(ft_ctx, *ptr) >= 0))
#define INVOKE_SUCCESS_CB(cb, ...)                                 \
    do {                                                           \
        if (!FeatureInvokeCallback(feature, cb, ##__VA_ARGS__)) {  \
            FEATURE_LOG_ERROR("invoke success callback failed !"); \
        }                                                          \
        FeatureRemoveCallback(feature, cb);                        \
    } while (0)

#define INVOKE_FAIL_CB(cb, msg, code)                           \
    do {                                                        \
        if (!FeatureInvokeCallback(feature, cb, msg, code)) {   \
            FEATURE_LOG_ERROR("invoke fail callback failed !"); \
        }                                                       \
        FeatureRemoveCallback(feature, cb);                     \
    } while (0)

#define INVOKE_COMPLET_CB(cb)                                       \
    do {                                                            \
        if (!FeatureInvokeCallback(feature, cb)) {                  \
            FEATURE_LOG_ERROR("invoke complete callback failed !"); \
        }                                                           \
        FeatureRemoveCallback(feature, cb);                         \
    } while (0)

static const char* file_tag = "[jidl_feature] Request_impl";
#define REQUEST_INFO(fmt, ...) \
    FEATURE_LOG_INFO("[feature_request] " fmt, ##__VA_ARGS__)

#define REQUEST_ERROR(fmt, ...) \
    FEATURE_LOG_ERROR("[feature_request] " fmt, ##__VA_ARGS__)

typedef struct
{
    uv_request_session_t* handle;
    struct weakref_list_node linklist;
    const char* pkg_name;
    int exit;
} RequestContext;

RequestContext* getRequestContext(FeatureInstanceHandle handle)
{
    return (RequestContext*)FeatureInstanceGetUserData(handle, "request_context");
}

typedef struct {
    int success = -1;
    int fail = -1;
    int complete = -1;
    bool isGlobal = true;
    uv_request_t* request = NULL;
    int request_type;
    char* filename = NULL;
    char* uuid = NULL;
    int notify_func = -1;
    FeatureInstanceHandle feature_handle;
    struct weakref_list_node node;
    off_t pre = -1;
} RequestInfo;

void freeRequestInfo(RequestInfo* info);

void __request_cancel(RequestInfo* info);

void system_request_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_request_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    RequestContext* th = static_cast<RequestContext*>(malloc(sizeof(*th)));
    if (!th) {
        REQUEST_ERROR("malloc fail");
        return;
    }
    th->exit = false;
    th->pkg_name = FeatureGetPackageName(handle);
    if (!th->pkg_name || strlen(th->pkg_name) == 0) {
        REQUEST_ERROR("package name is null!");
        th->pkg_name = "request_test";
    }
    weakref_list_initialize(&th->linklist);
    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    assert(uv_request_init(FeatureGetUVLoop(manager), &th->handle) == 0);
    FeatureSetUserData(manager, "request_context", th);
}
void system_request_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    // 创建一个request实例
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_request_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    // 退出页面，取消挂载在该instancehandle上的request请求
    RequestContext* th = getRequestContext(handle);
    RequestInfo *info, *temp;
    weakref_list_for_every_entry_safe(&th->linklist, info, temp, RequestInfo, node)
    {
        if (info->feature_handle == handle) {
            if (info->isGlobal) {
                // 阻止__request_cb回调流程
                uv_request_set_userp(info->request, NULL);
            } else {
                __request_cancel(info);
            }
        }
    }
}
void system_request_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    RequestContext* th = getRequestContext(handle);
    if (!th)
        return;
    // app 退出，cancel掉所有请求
    RequestInfo *info, *temp;
    weakref_list_for_every_entry_safe(&th->linklist, info, temp, RequestInfo, node)
    {
        uv_request_delete(info->request);
        freeRequestInfo(info);
    }

    if (th->exit == false) {
        uv_request_close(th->handle);
        th->exit = true;
    }

    free(th);
}
void system_request_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void freeRequestInfo(RequestInfo* info)
{
    REQUEST_INFO("free RequestInfo %p", info);
    if (info != NULL) {
        if (info->filename)
            free(info->filename);
        if (info->uuid) {
            free(info->uuid);
        }
        free(info);
        info = NULL;
    }
}

static RequestInfo* shareInfo = NULL;

// generate token
const char* uuid()
{
    char buf[100] = "";
    time_t now = time(NULL);
    srand((unsigned int)now);
    sprintf(buf, "%ld-%d", (long int)now, rand());

    char* token = static_cast<char*>(FeatureMalloc(strlen(buf) + 1, FT_CHAR));
    strncpy(token, buf, strlen(buf));
    return token;
}

static void __request_cb(int state, uv_response_t* response)
{
    REQUEST_INFO("==========> __request_cb, state = %d", state);
    if (shareInfo)
        shareInfo = NULL;
    RequestInfo* info = static_cast<RequestInfo*>(response->userp);
    REQUEST_INFO("info = %p", info);
    if (!info)
        return;
    FeatureInstanceHandle feature = info->feature_handle;
    RequestContext* th = getRequestContext(feature);
    if (state == UV_REQUEST_DONE) {
        if (info->request_type == UV_DOWNLOAD) {
            // 返回文件绝对地址
            // REQUEST_INFO("==========> success = %d", info->success);
            system_request_dl_cmpl_succ_t* param = system_requestMallocdl_cmpl_succ_t();
            // REQUEST_INFO("==========> response->body = %s", response->body);
            char* body = app_absolute_to_relative_path(th->pkg_name, response->body);
            char* uri = static_cast<char*>(FeatureMalloc(strlen(body) + 1, FT_CHAR));
            memcpy(uri, body, strlen(body));
            param->uri = uri;
            INVOKE_SUCCESS_CB(info->success, param);
            free(body);
        }
    } else if (state == UV_REQUEST_ERROR) {
        // body内存的是绝对路径的file位置
        // REQUEST_INFO("==========> body = %s", response->body);
        // REQUEST_INFO("==========> fail = %d", info->fail);
        INVOKE_FAIL_CB(info->fail, response->body, response->httpcode);
    } else if (state == REQUEST_CANCEL) {
        INVOKE_FAIL_CB(info->fail, "user cancel request", state);
    }

    // REQUEST_INFO("==========> complete = %d", info->complete);
    INVOKE_COMPLET_CB(info->complete);

    weakref_list_delete(&info->node);
    freeRequestInfo(info);
}

void __progress_cb(uv_request_t* request, off_t total, off_t now)
{
    // REQUEST_INFO("=== in __progress_cb, total = %ld, now = %ld", total, now);
    RequestInfo* info = (RequestInfo*)uv_request_get_userp(request);
    if (FeatureCheckCallbackId(info->feature_handle, info->notify_func)) {
        system_request_notify_data_t* data = system_requestMallocnotify_data_t();
        if (now != 0 && total == 0) {
            data->result = -1;
            data->percent = 0;
        } else if (total != 0) {
            data->result = 0;
            data->percent = 100 * now / total;
        }
        if (now != info->pre) {
            info->pre = now;
            FeatureInvokeCallback(info->feature_handle, info->notify_func, data);
        }
    }
}

void __request_cancel(RequestInfo* info)
{
    REQUEST_INFO("__request_cancel %p", info);
    if (info->request) {
        uv_request_delete(info->request);
        uv_response_t response;
        response.userp = info;
        __request_cb(REQUEST_CANCEL, &response);
    }
}

void initInfo(RequestInfo* info)
{
    info->notify_func = -1;
    info->success = -1;
    info->fail = -1;
    info->complete = -1;
    info->isGlobal = true;
    info->filename = NULL;
    info->uuid = NULL;
    info->pre = -1;
    info->request = NULL;
}

void system_request_wrap_download(FeatureInstanceHandle feature, AppendData append_data, system_request_download_t* param)
{
    const char *header, *filename, *msg;
    char *header_value, *kv, *pos_1, *pos_2, *absolute_path;
    size_t i = 1, j = 0, code;
    system_request_download_succ_t* suc_param;
    RequestContext* th = getRequestContext(feature);

    ft_context_ref ft_ctx = FeatureGetContext(feature);
    RequestInfo* info = static_cast<RequestInfo*>(malloc(sizeof(RequestInfo)));
    if (!info) {
        REQUEST_ERROR("malloc fail");
        code = GENERAL;
        msg = "malloc fail";
        goto callFail;
    }
    initInfo(info);

    if (!param->share && shareInfo) {
        __request_cancel(shareInfo);
        shareInfo = info;
    }

    // 参数检查
    if (param->url == NULL || strlen(param->url) == 0) {
        code = ARGSERROR;
        msg = "invalid url";
        goto callFail;
    }

    // REQUEST_INFO("get url = %s", param->url);
    // REQUEST_INFO("onDownLoadNotify = %d, suc = %d, fail = %d, compl = %d", param->onDownLoadNotify, param->success, param->fail, param->complete);

    info->request_type = UV_DOWNLOAD;
    info->feature_handle = feature;

    assert(uv_request_create(&info->request) == 0);
    assert(uv_request_set_url(info->request, param->url) == 0);
    assert(uv_request_set_method(info->request, "GET") == 0);

    weakref_list_initialize(&info->node);
    weakref_list_add_tail(&th->linklist, &info->node);

    if (check_any(param->header)) {
        header = ft_to_string(ft_ctx, *(param->header));
        REQUEST_INFO("header = %s", header);
        if (header == NULL || header[0] != '{' || header[strlen(header) - 1] != '}') {
            code = ARGSERROR;
            msg = "invalid header";
            goto callFail;
        }
        // header format {"test":"abc","test2":"ddd"}

        header_value = (char*)malloc(strlen(header) - 2);
        if (!header_value) {
            REQUEST_ERROR("malloc fail");
            code = GENERAL;
            msg = "malloc fail";
            ft_free_string(ft_ctx, header);
            goto callFail;
        }
        while (i < strlen(header) - 1) {
            if (header[i] != '"')
                header_value[j++] = header[i];
            i++;
        }
        header_value[j] = '\0';
        // REQUEST_INFO("header_value = %s", header_value);
        kv = strtok(header_value, ",");
        while (kv != NULL) {
            // REQUEST_INFO("append header = %s", kv);
            assert(uv_request_append_header(info->request, kv) == 0);
            kv = strtok(NULL, ",");
        }
        ft_free_string(ft_ctx, header);
        free(header_value);
    }

    if (!check_any(param->filename)) {
        pos_1 = strrchr(param->url, '?');
        pos_2 = strrchr(param->url, '/');
        if (pos_1 == NULL) {
            info->filename = strdup(pos_2 + 1);
        } else {
            info->filename = strndup(pos_2 + 1, pos_1 - pos_2 - 1);
        }
    } else {
        filename = ft_to_string(ft_ctx, *(param->filename));
        if (filename == NULL || strlen(filename) == 0) {
            code = ARGSERROR;
            msg = "invalid filename";
            goto callFail;
        }
        info->filename = strdup(filename);
        ft_free_string(ft_ctx, filename);
    }
    // REQUEST_INFO("info->filename = %s", info->filename);

    absolute_path = app_relative_to_absolute_path(th->pkg_name, info->filename);
    if (!absolute_path) {
        absolute_path = app_absolute_path_generator(th->pkg_name, "files", info->filename);
        assert(absolute_path);
    }
    // REQUEST_INFO("absolute_path = %s", absolute_path);
    assert(uv_request_set_atrribute(info->request, info->request_type, (void*)absolute_path) == 0);

    if (FeatureCheckCallbackId(feature, param->onDownLoadNotify)) {
        info->notify_func = param->onDownLoadNotify;
        assert(uv_request_set_atrribute(info->request, UV_DOWNLOAD_PROGRESS, (void*)__progress_cb) == 0);
    }

    if (!check_disk_limit()) {
        FEATURE_LOG_ERROR("insufficient memory to download file");
        code = GENERAL;
        msg = "no space to download file";
        goto callFail;
    }
    assert(uv_request_set_userp(info->request, info) == 0);
    assert(uv_request_commit(th->handle, info->request, __request_cb) == 0);
    suc_param = system_requestMallocdownload_succ_t();

    suc_param->token = uuid();
    info->uuid = strdup(suc_param->token);
    // REQUEST_INFO("suc_param._token = %s, info = %p", suc_param->token, info);
    INVOKE_SUCCESS_CB(param->success, suc_param);
    INVOKE_COMPLET_CB(param->complete);
    return;
callFail:
    REQUEST_INFO("code = %d, msg = %s", code, msg);
    INVOKE_FAIL_CB(param->fail, msg, code);
    INVOKE_COMPLET_CB(param->complete);
    if (info) {
        if (info->request)
            uv_request_delete(info->request);
        freeRequestInfo(info);
    }
}

void system_request_wrap_onDownloadComplete(FeatureInstanceHandle feature, AppendData append_data, system_request_dl_cmpl_t* param)
{
    REQUEST_INFO("onDownloadComplete token = %s", param->token);
    int code;
    const char* msg;
    RequestContext* th = getRequestContext(feature);

    if (param->token == NULL) {
        code = ARGSERROR;
        msg = "token is missing";
        goto fail;
    } else {
        RequestInfo *info, *temp, *res = NULL;
        weakref_list_for_every_entry_safe(&th->linklist, info, temp, RequestInfo, node)
        {
            if (strcmp(info->uuid, param->token) == 0) {
                res = info;
                break;
            }
        }
        if (res) {
            // REQUEST_INFO("info = %p", res);
            res->success = param->success;
            res->fail = param->fail;
            res->complete = param->complete;
        } else {
            code = TASK_NOT_EXISTS;
            msg = "task not exist";
            goto fail;
        }
        return;
    }
fail:
    FeatureInvokeCallback(feature, param->fail, msg, code);
    FeatureInvokeCallback(feature, param->complete);
}

void system_request_wrap_print(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params)
{
    printf("========== js print ==========> [jidl_feature] ");
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    for (int i = 0; i < vari_params.vari_count; i++) {
        ft_value_t param = vari_params.vari_args[i];
        ft_type param_type = ft_get_type(ft_ctx, param);
        if (param_type == FT_TYPE_OBJECT) {
            const char* param_obj = ft_to_string(ft_ctx, param);
            printf("%s ", param_obj);
            ft_free_string(ft_ctx, param_obj);
        } else if (param_type == FT_TYPE_ARRAY) {
            uint32_t array_size = ft_array_size(ft_ctx, param);
            printf("[");
            for (uint32_t j = 0; j < array_size; ++j) {
                ft_value_t elem = ft_array_at(ft_ctx, param, j);
                ft_type elem_type = ft_get_type(ft_ctx, elem);
                if (elem_type == FT_TYPE_NUMBER) {
                    double param_num;
                    if (ft_to_double(ft_ctx, elem, &param_num))
                        printf("%lf ", param_num);
                } else if (elem_type == FT_TYPE_STRING) {
                    const char* param_str = ft_to_string(ft_ctx, elem);
                    printf("%s ", param_str);
                    ft_free_string(ft_ctx, param_str);
                } else if (elem_type == FT_TYPE_BOOL) {
                    bool param_bool;
                    ft_to_bool(ft_ctx, param, &param_bool);
                    printf("%d ", param_bool);
                } else {
                    printf("invalid array element type!");
                    return;
                }
            }
            printf("] ");
        } else if (param_type == FT_TYPE_STRING) {
            const char* param_str = ft_to_string(ft_ctx, param);
            printf("%s ", param_str);
            ft_free_string(ft_ctx, param_str);
        } else if (param_type == FT_TYPE_NUMBER) {
            double param_num;
            ft_to_double(ft_ctx, param, &param_num);
            printf("%lf ", param_num);
        } else if (param_type == FT_TYPE_BOOL) {
            bool param_bool;
            ft_to_bool(ft_ctx, param, &param_bool);
            printf("%d ", param_bool);
        } else {
            printf("invalid param type!");
            return;
        }
    }
    printf("\n");
}
