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

#include "crypto_utils.h"
#include "fetch.h"
#include "net_utils.h"

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

typedef enum ResponseType { TXT = 1, JSON, FILE, ARRAYBUFFER } ResponseType;

typedef enum ContentType { TEXT = 1, URLENCODED, STREAM } ContentType;

static const char* method_type[] = {
    NULL, "GET", "OPTIONS", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT",
};

static const char* response_type[] = {
    NULL, "text", "json", "file", "arraybuffer",
};

static const char* content_type[] = {
    NULL,
    "text/plain",
    "application/x-www-form-urlencoded",
    "application/octet-stream",
};

}  // namespace Fetch

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
  bool exit;
  struct weakref_list_node node;
  uv_request_t* request;
} fetch_t;

Fetch::ResponseType get_response_tpye(const char* type) {
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

Fetch::ContentType get_content_type(const char* type) {
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
void request_cancel(fetch_t* fetch);
void fetch_free(fetch_t* p) {
  if (p) {
    FETCH_DEBUG("del node %p", p);
    weakref_list_delete(&p->node);
    delete p;
  }
}

void fetch_onRegister(const char* feature_name) { FETCH_DEBUG(""); }
void fetch_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  request_context_t* p =
      static_cast<request_context_t*>(malloc(sizeof(request_context_t)));
  ASSERT_RET_ECHO(p, "malloc err!");

  p->pkg = FeatureGetPackageName(handle);
  ASSERT_RET_ECHO(uv_request_init(FeatureGetUVLoop(
                                      FeatureGetManagerHandleFromProto(handle)),
                                  &(p->handle)) == 0,
                  "request init err");

  weakref_list_initialize(&p->linklist);
  FeatureSetProtoData(handle, p);
}
void fetch_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
  FETCH_DEBUG("");
}
void fetch_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
  request_context_t* p = get_request_context(handle);
  REQUEST_LIST_FOR_EVERY(&p->linklist, fetch_t) {
    if (req->exit) {
      fetch_free(req);
    } else {
      // Cancel instance callback
      uv_request_set_userp(req->request, NULL);
    }
  }
}

void fetch_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  request_context_t* p =
      static_cast<request_context_t*>(FeatureGetProtoData(handle));
  assert(p);

  // Cancel and delete all requests
  REQUEST_LIST_FOR_EVERY(&p->linklist, fetch_t) {
    FETCH_DEBUG("task:%p,request:%p", req, req->request);
    if (req->request) {
      request_cancel(req);
      uv_request_delete(req->request);
    }
    fetch_free(req);
  }
  uv_request_close(p->handle);
  FeatureSetProtoData(p, NULL);
  free(p);
}

void request_cancel(fetch_t* fetch) {
  uv_request_set_userp(fetch->request, NULL);

  uv_response_t response = {
      .httpcode = CANCEL_ERROR_CODE,
      .headers = NULL,
      .body = (char*)USER_ABORT_MSG,
      .size = USER_ABORT_MSG_SIZE,
      .userp = fetch,
  };
  fetch_request_cb(REQUEST_CANCEL, &response);
}

void fetch_onUnregister(const char* feature_name) { FETCH_DEBUG(""); }

bool get_method(FtString method, std::string& out) {
  if (!check_str(method)) {
    out.assign(Fetch::method_type[Fetch::MethodType::GET]);
    return true;
  } else if (!has_type(Fetch::method_type, arrayof(Fetch::method_type),
                       method)) {
    return false;
  }
  out.assign(method);
  return true;
}

static void fetch_request_cb(int state, uv_response_t* response) {
  fetch_t* p = static_cast<fetch_t*>(response->userp);
  ASSERT_RET(p);
  GET_FEATURE_AND_CTX(p);
  FETCH_DEBUG("state:%d \nbody:%s \nheaders:%s", state, response->body,
              response->headers);
  if (state == UV_REQUEST_DONE) {
    fetch_SuccessRes res;
    res._code = response->httpcode;
    res._data = (ft_value_t*)FeatureMalloc(
        sizeof(ft_value_t) + strlen(response->body), FT_ANY);
    res._headers = (ft_value_t*)FeatureMalloc(
        sizeof(ft_value_t) + strlen(response->headers), FT_ANY);
    ft_value_t ft_data = ft_from_string(p->ft_ctx, response->body);
    ft_value_t ft_header = ft_from_string(p->ft_ctx, response->headers);
    memcpy(res._data, &ft_data, strlen(response->body));
    memcpy(res._headers, &ft_header, strlen(response->headers));

    if (check_any(res._data)) {
      INVOKE_SUCCESS_CB(p->success_cb, &res);
    } else {
      INVOKE_FAIL_CB(p->fail_cb, "responseType dosen't match response data",
                     ErrorCode::IOERROR);
    }

    FeatureFreeValue(res._data);
    FeatureFreeValue(res._headers);

  } else {
    if (state == REQUEST_CANCEL) {
      FETCH_INFO(USER_ABORT_MSG);
      uv_request_delete(p->request);
    } else {
      FETCH_ERROR("upload err, error code: %d,msg: %s", response->httpcode,
                  response->body);
    }

    INVOKE_FAIL_CB(p->fail_cb, response->body, response->httpcode);
  }
  INVOKE_COMPLET_CB(p->complete_cb);
  p->exit = true;
  // request done,uv_request  has been released
  p->request = NULL;
}

static bool request_create(fetch_t* fetch, fetch_FetchPara* obj,
                           const char* method,
                           std::map<std::string, std::string>& headers,
                           content_t* ct) {
  request_context_t* p = get_request_context(fetch->feature);
  // create reques
  ASSERT_RET_NULL(0 == uv_request_create(&fetch->request));
  FETCH_DEBUG("request:%p", fetch->request);
  // set url
  uv_request_set_url(fetch->request, obj->_url);

  // set method
  uv_request_set_method(fetch->request, method);

  uv_request_set_data(
      fetch->request,
      (ct->data.empty() ? (void*)ct->buf_type_data : ct->data.c_str()),
      ct->size);

  // set header
  for (auto [key, val] : headers) {
    FETCH_DEBUG("header: %s", std::string(key + ":" + val).c_str());
    uv_request_append_header(fetch->request,
                             std::string(key + ":" + val).c_str());
  }

  // set timeout
  uv_request_set_timeout(fetch->request, obj->_timeout);

  // Set request to DOWNLOAD or FETCH
  uv_request_set_atrribute(fetch->request, fetch->type,
                           (void*)fetch->filename.c_str());
  // set to userp
  uv_request_set_userp(fetch->request, fetch);

  // start upload
  uv_request_commit(p->handle, fetch->request, fetch_request_cb);

  return true;
}

static fetch_t* fetch_create(FeatureInstanceHandle feature,
                             ft_context_ref ft_ctx, fetch_FetchPara* obj,
                             const char* pkg) {
  fetch_t* fetch = new fetch_t;
  assert(fetch);
  fetch->ft_ctx = ft_ctx;
  fetch->feature = feature;

  fetch->success_cb = obj->_success;
  fetch->fail_cb = obj->_fail;
  fetch->complete_cb = obj->_complete;

  fetch->type = strcmp(obj->_responseType, Fetch::response_type[Fetch::FILE])
                    ? UV_REQUEST
                    : UV_DOWNLOAD;
  if (fetch->type == UV_DOWNLOAD) {
    std::string url(obj->_url);

    fetch->filename = url.substr(url.find_last_of("/") + 1);

    if (fetch->filename.empty()) {
      time_t cur_time = time(NULL);
      char time_buf[100];
      strftime(time_buf, sizeof(time_buf), "%Y%m%d %H%M%S",
               std::localtime(&cur_time));
      char* path = app_absolute_path_generator(FeatureGetPackageName(feature),
                                               "files", (const char*)&time_buf);
      if (path) {
        fetch->filename.assign(path);
        free((void*)path);
      }
    }
  }

  fetch->request = NULL;
  fetch->exit = false;

  return fetch;
}

bool get_post_data_cb(const cJSON* const item, void* userp) {
  std::string* out_str = static_cast<std::string*>(userp);

  ASSERT_RET_NULL(out_str);
  if (item->type == cJSON_String) {
    if (!out_str->empty()) {
      out_str->append("&");
    }
    out_str->append(item->string);
    out_str->append("=");
    out_str->append(item->valuestring);
  }
  return true;
}

// Need to release  out->data
bool get_pdata_and_content_type(ft_context_ref ft_ctx, FtAny data,
                                Fetch::ContentType ct, content_t* out) {
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

      ft_map_for_every_entry(ft_ctx, data, (void*)&out->data, get_post_data_cb);
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
    std::map<std::string, std::string>& headers) {
  if (headers.count("content-type"))
    return get_content_type(headers.at("content-type").c_str());
  if (headers.count("Content-Type"))
    return get_content_type(headers.at("Content-Type").c_str());
  return (Fetch::ContentType)0;
}

void fetch_wrap_fetch(FeatureInstanceHandle feature, AppendData append_data,
                      fetch_FetchPara* obj) {
  ft_context_ref ft_ctx = FeatureGetContext(feature);
  assert(ft_ctx);
  const char* msg = "";
  int code = 0;
  fetch_t* fetch = NULL;
  std::string method;
  content_t content = {.buf_type_data = NULL, .size = 0};
  std::map<std::string, std::string> headers;
  request_context_t* p = get_request_context(feature);

  FETCH_DEBUG(
      "url:%s\ndata:%p\nheader:%p\nmethod:%p\nresponseType:%p\nsuccess:%"
      "d\nfail:%d\ncomplete:%d",
      obj->_url, obj->_data, obj->_header, obj->_method, obj->_responseType,
      obj->_success, obj->_fail, obj->_complete);

  // check arg
  obj->_timeout = obj->_timeout > 0 ? obj->_timeout : DEFAULT_TIMEOUT;
  SET_ARGERROR(check_url(obj->_url), "invalid url");

  SET_ARGERROR(get_method(obj->_method, method), "invalid method");

  SET_ARGERROR(check_header(ft_ctx, obj->_header, headers), "invalid headers");

  SET_ARGERROR(get_pdata_and_content_type(
                   ft_ctx, obj->_data, get_cy_from_header(headers), &content),
               "invalid data");

  // avoid setting twice
  if (content.content_type && headers.count("content-type")) {
    headers["content-type"].append(Fetch::content_type[content.content_type]);
    headers["content-type"].append(+"; charset=utf-8");
  }

  // create fetch context
  fetch = fetch_create(feature, ft_ctx, obj, p->pkg);
  SET_JS_ERROR(fetch, ErrorCode::GENERAL, "create native fetch err");

  // create curl request
  SET_JS_ERROR(request_create(fetch, obj, method.c_str(), headers, &content),
               ErrorCode::GENERAL, "create request err");

  FETCH_DEBUG("method:%s, request type:%d", method.c_str(), fetch->type);

  // add in list
  weakref_list_initialize(&fetch->node);
  weakref_list_add_tail(&p->linklist, &fetch->node);

  return;
err:
  FETCH_DEBUG("msg:%s,code:%d", msg, code);
  INVOKE_FAIL_CB(obj->_fail, msg, code);
  INVOKE_COMPLET_CB(obj->_complete);
}
