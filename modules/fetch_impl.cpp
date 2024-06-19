/*
 * Copyright (C) 2023 Xiaomi Corporation. All rights reserved.
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

#include <libgen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>

#include "fetch.h"
#include "net_utils.h"
#include "uv_ext.h"

namespace Fetch {

#define DEFAULT_TIMEOUT 20000

#define TAG "[fetch_impl] "

#define FETCH_DEBUG(fmt, ...) FEATURE_LOG_DEBUG(TAG fmt, ##__VA_ARGS__)

#define FETCH_INFO(fmt, ...) FEATURE_LOG_INFO(TAG fmt, ##__VA_ARGS__)

#define FETCH_ERROR(fmt, ...) FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

typedef enum MethodType {
    GET = 1,
    OPTIONS,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
} MethodType;

typedef enum PostDataType {
    UNDEF = 0,
    TF_STRING,
    SYS_STRING,
} PostDataType;

typedef enum ResponseType { TXT = 1,
    JSON,
    FILE,
    ARRAYBUFFER } ResponseType;

typedef enum ContentType { TEXT = 1,
    URLENCODED,
    STREAM } ContentType;

static const char* method_type[] = {
    NULL,
    "GET",
    "OPTIONS",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT",
};

static const char* response_type[] = {
    NULL,
    "text",
    "json",
    "file",
    "arraybuffer",
};

} // namespace Fetch

typedef struct content_t {
    Fetch::ContentType content_type;
    std::string data;
    uint8_t* buf_type_data = NULL;
    size_t size = 0;
} content_t;

typedef struct fetch_s {
    ft_context_ref ft_ctx;
    FeatureInstanceHandle feature;
    FtCallbackId success_cb;
    FtCallbackId fail_cb;
    FtCallbackId complete_cb;
    std::string filename;
    int type;
    Fetch::ResponseType response_type;
    bool exit;
    struct weakref_list_node node;
    uv_request_t* request;
    content_t* content;
    const char* url;
} fetch_t;

Fetch::ResponseType get_response_tpye(const char* type)
{
    if (!strcmp(type, Fetch::response_type[Fetch::ResponseType::ARRAYBUFFER])) {
        return Fetch::ResponseType::ARRAYBUFFER;
    } else if (!strcmp(type, Fetch::response_type[Fetch::ResponseType::FILE])) {
        return Fetch::ResponseType::FILE;
    } else if (!strcmp(type, Fetch::response_type[Fetch::ResponseType::JSON])) {
        return Fetch::ResponseType::JSON;
    } else if (!strcmp(type, Fetch::response_type[Fetch::ResponseType::TXT])) {
        return Fetch::ResponseType::TXT;
    } else {
        return (Fetch::ResponseType)0;
    }
}

Fetch::ContentType get_content_type(const char* type)
{
    if (!strcmp(type, Fetch::response_type[Fetch::ContentType::STREAM])) {
        return Fetch::ContentType::STREAM;
    } else if (!strcmp(type, Fetch::response_type[Fetch::ContentType::TEXT])) {
        return Fetch::ContentType::TEXT;
    } else if (!strcmp(type,
                   Fetch::response_type[Fetch::ContentType::URLENCODED])) {
        return Fetch::ContentType::URLENCODED;
    } else {
        return (Fetch::ContentType)0;
    }
}

static void fetch_request_cb(int state, uv_response_t* response);
void fetch_free(fetch_t* p)
{
    if (p) {
        FETCH_DEBUG("del node %p", p);
        weakref_list_delete(&p->node);
        if (p->content) {
            delete p->content;
            p->content = NULL;
        }
        if (p->url) {
            free((void*)p->url);
            p->url = NULL;
        }
        delete p;
    }
}

void system_fetch_onRegister(const char* feature_name) { FETCH_DEBUG(""); }
void system_fetch_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    request_context_t* p = static_cast<request_context_t*>(malloc(sizeof(request_context_t)));
    ASSERT_RET_ECHO(p, "malloc err!");

    p->pkg = FeatureGetPackageName(handle);
    ASSERT_RET_ECHO(uv_request_init(FeatureGetUVLoop(
                                        FeatureGetManagerHandleFromProto(handle)),
                        &(p->handle))
            == 0,
        "request init err");

    weakref_list_initialize(&p->linklist);
    FeatureSetProtoData(handle, p);
}
void system_fetch_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FETCH_DEBUG("");
}
void system_fetch_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FETCH_DEBUG("");
    request_context_t* p = get_request_context(handle);
    REQUEST_LIST_FOR_EVERY(&p->linklist, fetch_t)
    {
        FETCH_DEBUG("req=%p req->request=%p", req, req->request);
        if (req->feature == handle) {
            if (req->exit) {
                fetch_free(req);
            } else {
                // Cancel instance callback
                FETCH_INFO("Cancel instance callback %p", req);
                req->exit = true;
            }
        }
    }
}

void system_fetch_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FETCH_DEBUG("");
    request_context_t* p = static_cast<request_context_t*>(FeatureGetProtoData(handle));
    assert(p);

    // Cancel and delete all requests
    REQUEST_LIST_FOR_EVERY(&p->linklist, fetch_t)
    {
        FETCH_DEBUG("task:%p,request:%p", req, req->request);
        if (req->request) {
            FETCH_DEBUG("req=%p", req);
            uv_request_delete(req->request);
            req->request = NULL;
        }
        fetch_free(req);
    }
    uv_request_close(p->handle);
    FeatureSetProtoData(p, NULL);
    free(p);
}
void system_fetch_onUnregister(const char* feature_name) { FETCH_DEBUG(""); }

bool get_method(FtString method, Fetch::MethodType* out)
{
    FETCH_DEBUG("len:%d method:%s", strlen(method), method);
    if (!out) {
        return false;
    }
    if (!check_str(method)) {
        *out = Fetch::MethodType::GET;
        return true;
    }

    *out = (Fetch::MethodType)type_contain(
        Fetch::method_type, arrayof(Fetch::method_type), method, true);
    if (out) {
        return true;
    }
    return false;
}

static FtAny get_response_data(fetch_t* fetch, uv_response_t* response,
    ft_value_t* out)
{
    assert(fetch);
    if (!out) {
        FETCH_ERROR("arg err!");
        return NULL;
    }

    if (!response->body) {
        *out = ft_from_string(fetch->ft_ctx, "");
        return out;
    }

    switch (fetch->response_type) {
    case Fetch::ResponseType::JSON:
        *out = ft_parse_json(fetch->ft_ctx, response->body,
            strlen(response->body), NULL);
        break;
    default:
        if (fetch->type == UV_DOWNLOAD) {
            char* path = app_absolute_to_relative_path(
                FeatureGetPackageName(FeatureGetProtoHandle(fetch->feature)),
                response->body);
            assert(path);
            response->size = strlen(path) + 1;
            free(response->body);
            response->body = path;
        }
        *out = ft_from_string(fetch->ft_ctx, response->body);
        break;
    }
    return out;
}

static void fetch_request_cb(int state, uv_response_t* response)
{
    fetch_t* p = static_cast<fetch_t*>(response->userp);
    ASSERT_RET_ECHO(p, "The request has been cancelled");
    GET_FEATURE_AND_CTX(p);

    if (p->exit) {
        p->request = NULL;
        return;
    }
    FETCH_DEBUG("state:%d \nbody:%s ;\nheaders:%s", state, response->body,
        response->headers);
    if (state == UV_REQUEST_DONE && response->httpcode < HTTP_BAD_REQUES) {
        ft_value_t ft_header = ft_form_headers(p->ft_ctx, response->headers);
        system_fetch_SuccessRes res = {
            .code = (int)response->httpcode, .data = NULL, .headers = &ft_header
        };

        ft_value_t ft_data;
        res.data = get_response_data(p, response, &ft_data);

        if (check_any(res.data)) {
            INVOKE_SUCCESS_CB(p->success_cb, &res);
        } else {
            INVOKE_FAIL_CB(p->fail_cb, "responseType dosen't match response data",
                ErrorCode::IOERROR);
        }
    } else {
        FETCH_ERROR("upload err, error code: %d,msg: %s", response->httpcode,
            response->body);
        INVOKE_FAIL_CB(p->fail_cb, response->body, response->httpcode);
    }
    INVOKE_COMPLET_CB(p->complete_cb);
    REMOVE_ALL_CALLBACK(p->success_cb, p->fail_cb, p->complete_cb);
    p->exit = true;
    // request done,uv_request  has been released
    p->request = NULL;
}

static bool request_create(fetch_t* fetch, system_fetch_FetchPara* obj,
    Fetch::MethodType method,
    std::map<std::string, std::string>& headers)
{
    request_context_t* p = get_request_context(fetch->feature);
    // create reques
    ASSERT_RET_NULL(0 == uv_request_create(&fetch->request));
    FETCH_DEBUG("request:%p", fetch->request);

    // encode url
    const char* decode = url_decode(obj->url);
    if (strcmp(decode, obj->url) == 0) {
        fetch->url = url_encode(decode);
    } else {
        fetch->url = strdup(obj->url);
    }

    free((void*)decode);

    // set url
    uv_request_set_url(fetch->request, fetch->url);

    // set method
    if (method) {
        uv_request_set_method(fetch->request, Fetch::method_type[method]);
    }

    uv_request_set_data(
        fetch->request,
        (fetch->content->data.empty() ? (void*)fetch->content->buf_type_data
                                      : fetch->content->data.c_str()),
        fetch->content->size);

    // set header
    for (auto [key, val] : headers) {
        FETCH_DEBUG("header: %s", std::string(key + ": " + val).c_str());
        uv_request_append_header(fetch->request,
            std::string(key + ": " + val).c_str());
    }

    // set timeout
    if (!obj->timeout) {
        uv_request_set_timeout(fetch->request, obj->timeout);
    }

    // Set request to DOWNLOAD or FETCH
    uv_request_set_atrribute(fetch->request, fetch->type,
        (void*)fetch->filename.c_str());
    // set to userp
    uv_request_set_userp(fetch->request, fetch);

#if CONFIG_FEATURE_LOG_LEVEL == FEATURE_LOG_LEVEL_DEBUG
    uv_request_set_verbose(fetch->request);
#endif

    // start upload
    uv_request_commit(p->handle, fetch->request, fetch_request_cb);

    return true;
}

static fetch_t* fetch_create(FeatureInstanceHandle feature,
    ft_context_ref ft_ctx, system_fetch_FetchPara* obj,
    const char* pkg, content_t* ct)
{
    fetch_t* fetch = new fetch_t;
    assert(fetch);
    fetch->ft_ctx = ft_ctx;
    fetch->feature = feature;

    fetch->success_cb = obj->success;
    fetch->fail_cb = obj->fail;
    fetch->complete_cb = obj->complete;
    fetch->response_type = get_response_tpye(obj->responseType);
    fetch->type = (fetch->response_type == Fetch::ResponseType::FILE)
        ? UV_DOWNLOAD
        : UV_REQUEST;
    if (fetch->type == UV_DOWNLOAD) {
        char* path = NULL;
        char* filename = basename((char*)obj->url);

        if (strlen(filename) == strlen(obj->url)) {
            time_t cur_time = time(NULL);
            char time_buf[100];
            strftime(time_buf, sizeof(time_buf), "%Y%m%d %H%M%S",
                std::localtime(&cur_time));
            filename = time_buf;
        }

        path = app_relative_to_absolute_path(pkg, filename);

        if (path == NULL) {
            path = app_absolute_path_generator(pkg, "files", filename);
        }

        if (path == NULL) {
            FETCH_ERROR("No memory!");
            return NULL;
        }

        fetch->filename.assign(path);
        free((void*)path);
    }

    fetch->request = NULL;
    fetch->exit = false;
    fetch->content = ct;
    fetch->url = NULL;

    return fetch;
}

bool get_post_data_cb(const cJSON* const item, void* userp)
{
    std::string* out_str = static_cast<std::string*>(userp);

    ASSERT_RET_NULL(out_str);

    if (!out_str->empty()) {
        out_str->append("&");
    }
    out_str->append(item->string);
    out_str->append("=");

    char valuestr[24] = { '\0' };
    switch (item->type) {
    case cJSON_Number:
        sprintf(valuestr, "%lld", item->valueint);
        out_str->append(std::string(valuestr));
        break;
    case cJSON_String:
        out_str->append(item->valuestring);
        break;
    default:
        FETCH_ERROR("Unsupported %d type!", item->type);
        return false;
    }

    return true;
}

// Need to release  out->data
bool get_pdata_and_content_type(ft_context_ref ft_ctx, FtAny data,
    Fetch::ContentType ct, content_t* out)
{
    ft_type data_type = ft_get_type(ft_ctx, *data);
    FETCH_DEBUG("data: %d!", data_type);
    switch (data_type) {
    // ArrayBuffer type
    case FT_TYPE_BUFFER:
    case FT_TYPE_TYPED_BUFFER:
        out->content_type = ct ? ct : Fetch::ContentType::STREAM;
        out->buf_type_data = ft_to_buffer(ft_ctx, &out->size, *data);
        return true;

    // no set
    case FT_TYPE_NULL:
    case FT_TYPE_UNDEF:
    case FT_TYPE_NONE:
        out->content_type = ct ? ct : Fetch::ContentType::TEXT;
        return true;

    // string type
    case FT_TYPE_STRING:
        out->content_type = ct ? ct : Fetch::ContentType::TEXT;
        break;

    // json type
    case FT_TYPE_OBJECT:
        if (ct && ct != Fetch::ContentType::URLENCODED) {
            out->content_type = ct;
            break;
        }
        out->content_type = Fetch::ContentType::URLENCODED;

        if (!ft_map_for_every_entry(ft_ctx, data, (void*)&out->data,
                get_post_data_cb)) {
            return false;
        }
        out->size = out->data.size();
        FETCH_DEBUG("contenttype data:%s", out->data.c_str());
        return true;
    default:
        return false;
    }
    const char* ft_data = ft_to_string(ft_ctx, *data);
    out->data.assign(ft_data);
    out->size = out->data.size();
    ft_free_string(ft_ctx, ft_data);
    return true;
}

Fetch::ContentType get_cy_from_header(
    std::map<std::string, std::string>& headers)
{
    if (headers.count("content-type"))
        return get_content_type(headers.at("content-type").c_str());
    if (headers.count("Content-Type"))
        return get_content_type(headers.at("Content-Type").c_str());
    return (Fetch::ContentType)0;
}

void system_fetch_wrap_fetch(FeatureInstanceHandle feature, AppendData append_data,
    system_fetch_FetchPara* obj)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    assert(ft_ctx);
    const char* msg = "";
    int code = 0;
    fetch_t* fetch = NULL;
    Fetch::MethodType method;
    std::map<std::string, std::string> headers;
    request_context_t* p = get_request_context(feature);
    content_t* content = new content_t();
    SET_JS_ERROR(content, ErrorCode::GENERAL, "create content_t err");

    FETCH_DEBUG(
        "url:%s\ndata:%p\nheader:%p\nmethod:%p\nresponseType:%p\nsuccess:%"
        "d\nfail:%d\ncomplete:%d",
        obj->url, obj->data, obj->header, obj->method, obj->responseType,
        obj->success, obj->fail, obj->complete);

    // Check necessary parameters
    obj->timeout = obj->timeout > 0 ? obj->timeout : DEFAULT_TIMEOUT;
    SET_ARGERROR(check_url(obj->url), "invalid url");

    // Check for non-essential parameters
    SET_ARGERROR(get_method(obj->method, &method), "invalid method");

    if (check_any(obj->header)) {
        SET_ARGERROR(check_header(ft_ctx, obj->header, headers),
            "invalid headers");
    }

    if (check_any(obj->data) && (method != Fetch::MethodType::GET && method != Fetch::MethodType::HEAD)) {
        SET_ARGERROR(get_pdata_and_content_type(
                         ft_ctx, obj->data, get_cy_from_header(headers), content),
            "invalid data");
    }

    // create fetch context
    fetch = fetch_create(feature, ft_ctx, obj, p->pkg, content);
    SET_JS_ERROR(fetch, ErrorCode::GENERAL, "create native fetch err");

    // create curl request
    SET_JS_ERROR(request_create(fetch, obj, method, headers),
        ErrorCode::GENERAL, "create request err");

    // add in list
    weakref_list_initialize(&fetch->node);
    weakref_list_add_tail(&p->linklist, &fetch->node);

    return;
err:
    FETCH_DEBUG("msg:%s,code:%d", msg, code);
    INVOKE_FAIL_CB(obj->fail, msg, code);
    INVOKE_COMPLET_CB(obj->complete);
    REMOVE_ALL_CALLBACK(obj->success, obj->fail, obj->complete);
}
