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
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <string>
#include <time.h>
#include <type_traits>

#define REQUEST_CANCEL 2
#define DEFAULT_FILE_NAME "download_file"
#define DEFAULT_FILE_TYPE "txt"
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

#define DOWNLOAD_RESULT_CACHE_SIZE 10
typedef struct
{
    bool success;
    int code;
    const char* data;
} DownloadResult;

typedef struct {
    int success = -1;
    int fail = -1;
    int complete = -1;
    bool isGlobal = true;
    bool share;
    uv_request_t* request = NULL;
    int request_type;
    char* filename = NULL;
    char* uuid = NULL;
    int notify_func = -1;
    FeatureInstanceHandle feature_handle;
    struct weakref_list_node node;
    off_t pre = -1;
} RequestInfo;

typedef struct
{
    uv_request_session_t* handle;
    struct weakref_list_node linklist;
    const char* pkg_name;
    int exit;
    std::map<std::string, DownloadResult*>* download_results;
    RequestInfo* shareInfo = NULL;
} RequestContext;

RequestContext* getRequestContext(FeatureInstanceHandle handle)
{
    void* user_data = FeatureGetProtoData(FeatureGetProtoHandle(handle));
    assert(user_data != nullptr);
    return static_cast<RequestContext*>(user_data);
}

void addResult(FeatureInstanceHandle handle, char* uuid, DownloadResult* result)
{
    REQUEST_INFO("add download result {%s, %s, code: %d, success: %d}", uuid, result->data, result->code, result->success);

    std::map<std::string, DownloadResult*>* downloadResults = getRequestContext(handle)->download_results;
    (*downloadResults)[uuid] = result;
    if ((*downloadResults).size() >= DOWNLOAD_RESULT_CACHE_SIZE) {
        REQUEST_INFO("downloadResults size is out of range, free downloadResults.begin()");
        free((void*)(*downloadResults).begin()->second->data);
        (*downloadResults).erase((*downloadResults).begin());
    }
}

void freeRequestInfo(RequestInfo* info);

void __request_cancel(RequestInfo* info);

void system_request_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_request_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    RequestContext* th = (RequestContext*)FeatureGetProtoData(handle);
    if (th == nullptr) {
        th = static_cast<RequestContext*>(malloc(sizeof(*th)));
        if (!th) {
            REQUEST_ERROR("malloc RequestContext fail");
            return;
        }
        th->exit = false;
        th->pkg_name = FeatureGetPackageName(handle);
        if (!th->pkg_name || strlen(th->pkg_name) == 0) {
            REQUEST_ERROR("package name is null!");
            th->pkg_name = "request_test";
        }
        weakref_list_initialize(&th->linklist);
        uv_request_init(FeatureGetUVLoop(manager), &th->handle);
        th->download_results = new std::map<std::string, DownloadResult*>();
        if (!th->download_results) {
            REQUEST_ERROR("malloc downloadResults fail");
        }
        th->shareInfo = NULL;
        FeatureSetProtoData(handle, th);
    }
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
    RequestContext* th = static_cast<RequestContext*>(FeatureGetProtoData(handle));
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
    // free download_results
    std::map<std::string, DownloadResult*>* downloadResults = th->download_results;
    if (downloadResults != nullptr) {
        for (auto it = downloadResults->begin(); it != downloadResults->end(); ++it) {
            free((void*)it->second->data);
            free(it->second);
        }
        delete downloadResults;
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

// generate token
char* uuid()
{
    char buf[100] = "";
    time_t now = time(NULL);
    srand((unsigned int)now);
    sprintf(buf, "%ld-%d", (long int)now, rand());
    return strdup(buf);
}

static void __request_cb(int state, uv_response_t* response)
{
    REQUEST_INFO("in __request_cb, state = %d", state);
    RequestInfo* info = static_cast<RequestInfo*>(response->userp);
    if (!info)
        return;
    FeatureInstanceHandle feature = info->feature_handle;
    RequestContext* th = getRequestContext(feature);
    if (info->share == false) {
        th->shareInfo = NULL;
    }

    DownloadResult* res = static_cast<DownloadResult*>(malloc(sizeof(DownloadResult)));
    if (state == UV_REQUEST_DONE) {
        if (info->request_type == UV_DOWNLOAD) {
            // 返回文件绝对地址
            system_request_dl_cmpl_succ_t* param = system_requestMallocdl_cmpl_succ_t();
            char* body = app_absolute_to_relative_path(th->pkg_name, response->body);
            char* value = (char*)FeatureMalloc(strlen(body) + 1, FT_CHAR);
            sprintf(value, "%s", body);
            param->uri = value;
            INVOKE_SUCCESS_CB(info->success, param);
            FeatureFreeValue(param);
            FeatureRemoveCallback(feature, info->fail);
            res->success = true;
            res->data = strdup(body);
            res->code = UV_REQUEST_DONE;
            free(body);
        }
    } else if (state == UV_REQUEST_ERROR) {
        // body内存的是绝对路径的file位置
        REQUEST_INFO("request error: %s", response->body);
        INVOKE_FAIL_CB(info->fail, response->body, TASK_FAILED);
        FeatureRemoveCallback(feature, info->success);
        res->success = false;
        res->data = strdup(response->body);
        res->code = TASK_FAILED;
    } else if (state == REQUEST_CANCEL) {
        INVOKE_FAIL_CB(info->fail, "user cancel request", state);
        FeatureRemoveCallback(feature, info->success);
        res->success = false;
        res->data = strdup(response->body);
        res->code = CANCEL_ERROR_CODE;
    }
    addResult(feature, info->uuid, res);
    INVOKE_COMPLET_CB(info->complete);

    weakref_list_delete(&info->node);
    freeRequestInfo(info);
}

int __progress_cb(uv_request_t* request, off_t dltotal, off_t dlnow, off_t ultotal, off_t ulnow)
{
    // REQUEST_INFO("=== in __progress_cb, total = %ld, now = %ld", dltotal, dlnow);
    RequestInfo* info = (RequestInfo*)uv_request_get_userp(request);
    if (FeatureCheckCallbackId(info->feature_handle, info->notify_func)) {
        system_request_notify_data_t* data = system_requestMallocnotify_data_t();
        if (dlnow != 0 && dltotal == 0) {
            data->result = -1;
            data->percent = 0;
        } else if (dltotal != 0) {
            data->result = 0;
            data->percent = 100 * dlnow / dltotal;
        }
        if (dlnow != info->pre) {
            info->pre = dlnow;
            FeatureInvokeCallback(info->feature_handle, info->notify_func, data);
        }
        FeatureFreeValue(data);
    }
    return 0;
}

void __request_cancel(RequestInfo* info)
{
    REQUEST_INFO("__request_cancel %p", info);
    if (info->request) {
        uv_request_delete(info->request);
        uv_response_t response;
        response.userp = info;
        response.body = (char*)"request is canceled";
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
    info->share = true;
    info->filename = NULL;
    info->uuid = NULL;
    info->pre = -1;
    info->request = NULL;
}

bool __is_valid_uri(const char* uri)
{
    std::string uri_str(uri);
    // Needs to start with `http://` or `https://`
    std::regex uri_regex("^https?://[\\S]*$");
    return std::regex_match(uri_str, uri_regex);
}

void __remove_trailing_slash(char* url)
{
    int tail = strlen(url);
    while (tail > 0 && url[tail - 1] == '/') {
        tail--;
    }
    url[tail] = '\0';
}

char* __get_filename_from_url(const char* url)
{
    const char* question_mark = strrchr(url, '?');

    // remove url parameter
    char* url_main;
    if (question_mark == NULL) {
        url_main = strdup(url);
    } else {
        url_main = strndup(url, question_mark - url - 1);
    }
    // remove slash mark in backwards
    __remove_trailing_slash(url_main);
    REQUEST_INFO("url_main = %s", url_main);

    // get last slash mark in url_main
    const char* slash_mark = strrchr(url_main, '/');

    if (slash_mark != NULL) {
        char* filename = strdup(slash_mark + 1);
        REQUEST_INFO("tmp filename = %s", filename);
        const char* dot_mask = strrchr(filename, '.');
        if (dot_mask != NULL && dot_mask == strchr(filename, '.')) {
            // only one dot in filename, its a valid filename
            free(url_main);
            return filename;
        }
        free(filename);
    }
    free(url_main);

    REQUEST_INFO("genarate default filename");
    // cannot get filename from url, create one
    char tmp[64] = "";
    sprintf(tmp, "%s-%d.%s", DEFAULT_FILE_NAME, rand(), DEFAULT_FILE_TYPE);
    return strdup(tmp);
}

void system_request_wrap_download(FeatureInstanceHandle feature, AppendData append_data, system_request_download_t* param)
{
    const char* msg;
    char *filename, *token;
    char kv[1024];
    int code;
    rapidjson::Document doc;
    rapidjson::ParseResult result;
    system_request_download_succ_t* suc_param;
    RequestContext* th = getRequestContext(feature);

    RequestInfo* info = static_cast<RequestInfo*>(malloc(sizeof(RequestInfo)));
    if (!info) {
        REQUEST_ERROR("malloc fail");
        code = GENERAL;
        msg = "malloc fail";
        goto callFail;
    }
    initInfo(info);

    if (param->share == false) {
        info->share = false;
        if (th->shareInfo != NULL) {
            __request_cancel(th->shareInfo);
        }
        th->shareInfo = info;
    }

    // 参数检查
    // REQUEST_INFO("get url = %s", param->url);
    if (param->url == NULL || strlen(param->url) == 0 || !__is_valid_uri(param->url)) {
        code = ARGSERROR;
        msg = "invalid url";
        goto callFail;
    }

    info->request_type = UV_DOWNLOAD;
    info->feature_handle = feature;
    // REQUEST_INFO("header = %s", param->header);
    // header format "{"test":"abc","test2":"ddd"}"

    if (param->header != NULL) {
        result = doc.Parse(param->header);
        if (result.IsError()) {
            REQUEST_ERROR("header json parse error");
            code = ARGSERROR;
            msg = "invalid header";
            goto callFail;
        }
    }

    if (param->filename != NULL && strlen(param->filename) > 0) {
        filename = strdup(param->filename);
    } else {
        filename = __get_filename_from_url(param->url);
    }
    // REQUEST_INFO("filename = %s", filename);

    info->filename = app_relative_to_absolute_path(th->pkg_name, filename);
    if (!info->filename) {
        info->filename = app_absolute_path_generator(th->pkg_name, "files", filename);
        if (!info->filename) {
            REQUEST_ERROR("info->filename is null");
        }
    }
    free(filename);
    REQUEST_INFO("info->filename = %s", info->filename);

    if (!check_disk_limit()) {
        REQUEST_ERROR("insufficient memory to download file");
        code = GENERAL;
        msg = "no space to download file";
        goto callFail;
    }

    uv_request_create(&info->request);
    uv_request_set_url(info->request, param->url);
    uv_request_set_method(info->request, "GET");
    uv_request_set_atrribute(info->request, info->request_type, (void*)info->filename);
    if (FeatureCheckCallbackId(feature, param->onDownLoadNotify)) {
        info->notify_func = param->onDownLoadNotify;
        uv_request_set_atrribute(info->request, UV_DOWNLOAD_PROGRESS, (void*)__progress_cb);
    }
    if (!doc.IsNull()) {
        for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
            memset(kv, 0, sizeof(kv));
            sprintf(kv, "%s: ", itr->name.GetString());
            if (itr->value.IsString()) {
                sprintf(kv, "%s", itr->value.GetString());
            } else if (itr->value.IsInt()) {
                sprintf(kv, "%d", itr->value.GetInt());
            } else if (itr->value.IsDouble()) {
                sprintf(kv, "%f", itr->value.GetDouble());
            } else if (itr->value.IsBool()) {
                sprintf(kv, "%d", itr->value.GetBool());
            } else {
                REQUEST_ERROR("unkown type");
            }
            // REQUEST_INFO("kv = '%s'", kv);
            uv_request_append_header(info->request, kv);
        }
    }
    uv_request_set_userp(info->request, info);
    uv_request_commit(th->handle, info->request, __request_cb);
    weakref_list_initialize(&info->node);
    weakref_list_add_tail(&th->linklist, &info->node);

    suc_param = system_requestMallocdownload_succ_t();
    info->uuid = uuid();
    token = (char*)FeatureMalloc(strlen(info->uuid) + 1, FT_CHAR);
    sprintf(token, "%s", info->uuid);
    suc_param->token = token;
    // REQUEST_INFO("suc_param._token = %s, info = %p", suc_param->token, info);
    INVOKE_SUCCESS_CB(param->success, suc_param);
    INVOKE_COMPLET_CB(param->complete);
    FeatureRemoveCallback(feature, param->fail);
    FeatureFreeValue(suc_param);
    return;
callFail:
    REQUEST_INFO("code = %d, msg = %s", code, msg);
    INVOKE_FAIL_CB(param->fail, msg, code);
    INVOKE_COMPLET_CB(param->complete);
    FeatureRemoveCallback(feature, param->success);
    if (info) {
        freeRequestInfo(info);
    }
}

void system_request_wrap_onDownloadComplete(FeatureInstanceHandle feature, AppendData append_data, system_request_dl_cmpl_t* param)
{
    REQUEST_INFO("onDownloadComplete token = %s", param->token);
    int code;
    const char* msg;
    RequestContext* th = getRequestContext(feature);
    system_request_dl_cmpl_succ_t* succ_param;
    std::map<std::string, DownloadResult*>* downloadResults = th->download_results;

    if (param->token == NULL) {
        REQUEST_ERROR("empty token");
        code = TASK_NOT_EXISTS;
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
            auto it = (*downloadResults).find(param->token);
            if (it != (*downloadResults).end()) {
                if (it->second->success) {
                    REQUEST_INFO("get (*downloadResults).data = %s", it->second->data);
                    succ_param = system_requestMallocdl_cmpl_succ_t();
                    char* value = (char*)FeatureMalloc(strlen(it->second->data) + 1, FT_CHAR);
                    sprintf(value, "%s", it->second->data);
                    succ_param->uri = value;
                    INVOKE_SUCCESS_CB(param->success, succ_param);
                    FeatureRemoveCallback(feature, param->fail);
                    FeatureFreeValue(succ_param);
                } else {
                    INVOKE_FAIL_CB(param->fail, it->second->data, it->second->code);
                    FeatureRemoveCallback(feature, param->success);
                }
                INVOKE_COMPLET_CB(param->complete);
            } else {
                code = TASK_NOT_EXISTS;
                msg = "task not exist";
                goto fail;
            }
        }
        return;
    }
fail:
    INVOKE_FAIL_CB(param->fail, msg, code);
    FeatureRemoveCallback(feature, param->fail);
    INVOKE_COMPLET_CB(param->complete);
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
