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
#include "feature_context_qjs.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_types.h"
#include "net_utils.h"
#include "quickapp_inspector.h"
#include "request.h"
#include "uv_ext.h"
#include <cassert>
#include <cctype>
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
#include <string>
#include <time.h>
#include <type_traits>

#define REQUEST_CANCEL 2
#define DEFAULT_FILE_NAME "download_file"
#define DEFAULT_FILE_TYPE "txt"

#define REQUEST_DEBUG(fmt, ...) \
    FEATURE_LOG_DEBUG("[feature_request] " fmt, ##__VA_ARGS__)

#define REQUEST_LIFECYCLE_DEBUG() \
    REQUEST_DEBUG("[jidl_feature] Request_impl:: %s()", __FUNCTION__)

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
} ReqInfo;

typedef struct
{
    uv_request_session_t* handle;
    struct weakref_list_node linklist;
    const char* pkg_name;
    int exit;
    std::map<std::string, DownloadResult*>* download_results;
    ReqInfo* shareInfo = NULL;
} RequestContext;

RequestContext* getRequestContext(FeatureInstanceHandle handle)
{
    void* user_data = FeatureGetProtoData(FeatureGetProtoHandle(handle));
    assert(user_data != nullptr);
    return static_cast<RequestContext*>(user_data);
}

void addResult(FeatureInstanceHandle handle, char* uuid, DownloadResult* result)
{
    REQUEST_DEBUG("add download result {%s, %s, code: %d, success: %d}", uuid, result->data, result->code, result->success);

    std::map<std::string, DownloadResult*>* downloadResults = getRequestContext(handle)->download_results;
    (*downloadResults)[uuid] = result;
    if ((*downloadResults).size() >= DOWNLOAD_RESULT_CACHE_SIZE) {
        REQUEST_DEBUG("downloadResults size is out of range, free downloadResults.begin()");
        free((void*)(*downloadResults).begin()->second->data);
        free((*downloadResults).begin()->second);
        (*downloadResults).erase((*downloadResults).begin());
    }
}

void freeReqInfo(ReqInfo* info);

void __request_cancel(ReqInfo* info);

void system_request_onRegister(const char* feature_name)
{
    REQUEST_LIFECYCLE_DEBUG();
}
void system_request_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    REQUEST_LIFECYCLE_DEBUG();
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
        if (!check_str(th->pkg_name)) {
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
    REQUEST_LIFECYCLE_DEBUG();
}
void system_request_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    REQUEST_LIFECYCLE_DEBUG();
    // 退出页面，取消挂载在该instancehandle上的request请求
    RequestContext* th = getRequestContext(handle);
    ReqInfo *info, *temp;
    weakref_list_for_every_entry_safe(&th->linklist, info, temp, ReqInfo, node)
    {
        // The global task countinue to excute, but js callback function will not be called
        if (info->feature_handle == handle && !info->isGlobal) {
            __request_cancel(info);
        }
    }
}
void system_request_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    REQUEST_LIFECYCLE_DEBUG();
    RequestContext* th = static_cast<RequestContext*>(FeatureGetProtoData(handle));
    if (!th)
        return;
    // app 退出，cancel掉所有请求
    ReqInfo *info, *temp;
    weakref_list_for_every_entry_safe(&th->linklist, info, temp, ReqInfo, node)
    {
        uv_request_delete(info->request);
        freeReqInfo(info);
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
    REQUEST_LIFECYCLE_DEBUG();
}

void freeReqInfo(ReqInfo* info)
{
    REQUEST_DEBUG("free ReqInfo %p", info);
    if (info != NULL) {
        if (info->filename)
            free(info->filename);
        if (info->uuid) {
            free(info->uuid);
        }
        if (info->feature_handle) {
            FeatureFreeInstanceHandle(info->feature_handle);
        }
        free(info);
        info = NULL;
    }
}

static int start_uuid = 0;
// generate token
char* uuid()
{
    char buf[100] = "";
    time_t now = time(NULL);
    srand((unsigned int)now);
    sprintf(buf, "%ld-%d-%d", (long int)now, rand(), ++start_uuid);
    return strdup(buf);
}

static void __request_cb(int state, uv_response_t* response)
{
    REQUEST_DEBUG("in __request_cb, state = %d", state);
    ReqInfo* info = static_cast<ReqInfo*>(response->userp);
    if (!info)
        return;
    FeatureInstanceHandle feature = info->feature_handle;
    if (!FeatureInstanceIsDetached(feature)) {
        RequestContext* th = getRequestContext(feature);
        if (info->share == false) {
            th->shareInfo = NULL;
        }

        DownloadResult* res = static_cast<DownloadResult*>(malloc(sizeof(DownloadResult)));
        if (state == UV_REQUEST_DONE) {
            if (info->request_type == UV_DOWNLOAD) {
                char* body = app_absolute_to_relative_path(th->pkg_name, response->body);
                // 返回文件绝对地址
                system_request_dl_cmpl_succ_t* param = system_requestMallocdl_cmpl_succ_t();
                char* value = (char*)FeatureMalloc(strlen(body) + 1, FT_CHAR);
                sprintf(value, "%s", body);
                param->uri = value;
                INVOKE_SUCCESS_CB(info->success, param);
                FeatureFreeValue(param);

                res->success = true;
                res->data = body;
                res->code = UV_REQUEST_DONE;
            }
            InspectHostNetResponse(info->request, response, InspectHostNetGetCurrentReqId(), response->headers);

        } else if (state == UV_REQUEST_ERROR) {
            // body内存的是绝对路径的file位置
            REQUEST_ERROR("request error: %s", response->body);
            INVOKE_FAIL_CB(info->fail, response->body, FT_ERR_TASK_FAILED);
            res->success = false;
            res->data = strdup(response->body);
            res->code = FT_ERR_TASK_FAILED;
        } else if (state == REQUEST_CANCEL) {
            INVOKE_FAIL_CB(info->fail, "user cancel request", state);
            res->success = false;
            res->data = strdup(response->body);
            res->code = FT_ERR_CANCEL_ERROR_CODE;
        }
        addResult(feature, info->uuid, res);
        INVOKE_COMPLET_CB(info->complete);
        REMOVE_ALL_CALLBACK(info->success, info->fail, info->complete);
    } else {
        REQUEST_DEBUG("feature instance is detached");
    }

    weakref_list_delete(&info->node);
    freeReqInfo(info);
}

int __progress_cb(uv_request_t* request, off_t dltotal, off_t dlnow, off_t ultotal, off_t ulnow)
{
    // REQUEST_DEBUG("=== in __progress_cb, total = %ld, now = %ld", dltotal, dlnow);
    ReqInfo* info = (ReqInfo*)uv_request_get_userp(request);
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

void __request_cancel(ReqInfo* info)
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

void initInfo(ReqInfo* info)
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
    info->feature_handle = NULL;
}

// 校验 URI 是否合法：必须以 http:// 或 https:// 开头，且不含空白字符。
// 原实现使用 std::regex("^https?://[\\S]*$") 进行匹配，语义等价，
// 但 std::regex 内部基于回溯的 NFA 引擎在处理长 URL 时会产生大量栈递归，
// 在 NuttX 等栈空间有限的嵌入式环境下容易导致栈溢出（stack overflow）。
// 改用简单的字符串遍历，栈开销几乎为零。
bool __is_valid_uri(const char* uri)
{
    if (uri == NULL)
        return false;

    // 检查协议头：http:// 或 https://
    if (strncmp(uri, "https://", 8) == 0) {
        uri += 8;
    } else if (strncmp(uri, "http://", 7) == 0) {
        uri += 7;
    } else {
        return false;
    }

    // 协议头后面必须有内容，且不能包含空白字符
    if (*uri == '\0')
        return false;
    for (const char* p = uri; *p != '\0'; ++p) {
        if (isspace((unsigned char)*p))
            return false;
    }
    return true;
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
    REQUEST_DEBUG("url_main = %s", url_main);

    // get last slash mark in url_main
    const char* slash_mark = strrchr(url_main, '/');

    if (slash_mark != NULL) {
        char* filename = strdup(slash_mark + 1);
        REQUEST_DEBUG("tmp filename = %s", filename);
        const char* dot_mask = strrchr(filename, '.');
        if (dot_mask != NULL && dot_mask == strchr(filename, '.')) {
            // only one dot in filename, its a valid filename
            free(url_main);
            return filename;
        }
        free(filename);
    }
    free(url_main);

    REQUEST_DEBUG("genarate default filename");
    // cannot get filename from url, create one
    char tmp[64] = "";
    sprintf(tmp, "%s-%d.%s", DEFAULT_FILE_NAME, rand(), DEFAULT_FILE_TYPE);
    return strdup(tmp);
}

void system_request_wrap_download(FeatureInstanceHandle feature, AppendData append_data, system_request_download_t* param)
{
    const char* msg;
    char *filename, *token, *header;
    int code;
    system_request_download_succ_t* suc_param;
    RequestContext* th = getRequestContext(feature);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    std::map<std::string, std::string> formdata;

    ReqInfo* info = static_cast<ReqInfo*>(malloc(sizeof(ReqInfo)));
    if (!info) {
        REQUEST_ERROR("malloc fail");
        code = FT_ERR_GENERAL;
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
    // REQUEST_DEBUG("get url = %s", param->url);
    if (!check_str(param->url) || !__is_valid_uri(param->url)) {
        code = FT_ERR_ARGS;
        msg = "invalid url";
        goto callFail;
    }

    info->request_type = UV_DOWNLOAD;
    info->feature_handle = FeatureDupInstanceHandle(feature);

    if (check_str(param->filename)) {
        filename = strdup(param->filename);
    } else {
        filename = __get_filename_from_url(param->url);
    }
    // REQUEST_DEBUG("filename = %s", filename);

    info->filename = app_relative_to_absolute_path(th->pkg_name, filename);
    if (!info->filename) {
        info->filename = app_absolute_path_generator(th->pkg_name, "files", filename);
        if (!info->filename) {
            REQUEST_ERROR("info->filename is null");
        }
    }
    free(filename);
    REQUEST_DEBUG("info->filename = %s", info->filename);

    if (!check_disk_limit()) {
        // TODO cannot call at this place!
        // notify_disk_space_insufficient(feature, param->url);
        REQUEST_ERROR("insufficient memory to download file");
        code = FT_ERR_GENERAL;
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

    if (check_str(param->header)) {
        // header format "{"test":"abc","test2":"ddd"}"
        header = strdup(param->header);
        ft_value_t ft_header = ft_form_headers(ft_ctx, header);
        if (check_header(ft_ctx, &ft_header, formdata)) {
            for (auto [key, val] : formdata) {
                REQUEST_DEBUG("formdata: %s", std::string(key + ":" + val).c_str());
                uv_request_append_header(info->request, std::string(key + ":" + val).c_str());
            }
        }
        free(header);
        ft_free_value(ft_ctx, ft_header);
    }

    uv_request_set_userp(info->request, info);
    InspectHostNetRequest(info->request, "GET", InspectHostNetGetReqId());
    uv_request_commit(th->handle, info->request, __request_cb);
    weakref_list_initialize(&info->node);
    weakref_list_add_tail(&th->linklist, &info->node);

    suc_param = system_requestMallocdownload_succ_t();
    info->uuid = uuid();
    token = (char*)FeatureMalloc(strlen(info->uuid) + 1, FT_CHAR);
    sprintf(token, "%s", info->uuid);
    suc_param->token = token;
    // REQUEST_DEBUG("suc_param._token = %s, info = %p", suc_param->token, info);
    INVOKE_SUCCESS_CB(param->success, suc_param);
    INVOKE_COMPLET_CB(param->complete);
    REMOVE_ALL_CALLBACK(param->success, param->fail, param->complete);
    FeatureFreeValue(suc_param);
    return;
callFail:
    InspectHostNetLoadingFailed(true);
    REQUEST_ERROR("code = %d, msg = %s", code, msg);
    INVOKE_FAIL_CB(param->fail, msg, code);
    INVOKE_COMPLET_CB(param->complete);
    REMOVE_ALL_CALLBACK(param->success, param->fail, param->complete);
    if (info) {
        freeReqInfo(info);
    }
}

void system_request_wrap_onDownloadComplete(FeatureInstanceHandle feature, AppendData append_data, system_request_dl_cmpl_t* param)
{
    REQUEST_DEBUG("onDownloadComplete token = %s", param->token);
    int code;
    const char* msg;
    RequestContext* th = getRequestContext(feature);
    system_request_dl_cmpl_succ_t* succ_param;
    std::map<std::string, DownloadResult*>* downloadResults = th->download_results;

    if (param->token == NULL) {
        REQUEST_ERROR("empty token");
        code = FT_ERR_TASK_NOT_EXISTS;
        msg = "token is missing";
        goto fail;
    } else {
        ReqInfo *info, *temp, *res = NULL;
        weakref_list_for_every_entry_safe(&th->linklist, info, temp, ReqInfo, node)
        {
            if (strcmp(info->uuid, param->token) == 0) {
                res = info;
                break;
            }
        }
        if (res) {
            // REQUEST_DEBUG("info = %p", res);
            res->success = param->success;
            res->fail = param->fail;
            res->complete = param->complete;
        } else {
            auto it = (*downloadResults).find(param->token);
            if (it != (*downloadResults).end()) {
                if (it->second->success) {
                    REQUEST_DEBUG("get (*downloadResults).data = %s", it->second->data);
                    succ_param = system_requestMallocdl_cmpl_succ_t();
                    char* value = (char*)FeatureMalloc(strlen(it->second->data) + 1, FT_CHAR);
                    sprintf(value, "%s", it->second->data);
                    succ_param->uri = value;
                    INVOKE_SUCCESS_CB(param->success, succ_param);
                    FeatureFreeValue(succ_param);
                } else {
                    INVOKE_FAIL_CB(param->fail, it->second->data, it->second->code);
                }
                INVOKE_COMPLET_CB(param->complete);
                REMOVE_ALL_CALLBACK(param->success, param->fail, param->complete);
            } else {
                code = FT_ERR_TASK_NOT_EXISTS;
                msg = "task not exist";
                goto fail;
            }
        }
        return;
    }
fail:
    INVOKE_FAIL_CB(param->fail, msg, code);
    INVOKE_COMPLET_CB(param->complete);
    REMOVE_ALL_CALLBACK(param->success, param->fail, param->complete);
}
