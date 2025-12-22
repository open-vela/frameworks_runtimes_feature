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

#ifndef _NET_UTILS_H_
#define _NET_UTILS_H_
#include <stdbool.h>
#include <stdint.h>

#include <map>

#include "app_path.h"
#include "feature.h"
#include "feature_list.h"
#include "feature_types.h"
#include "netutils/cJSON.h"
#include "uv_ext.h"

// namespace NET {
typedef enum ErrorCode {
    GENERAL = 200,
    ARGSERROR = 202,
    TIMEOUT = 204,
    IOERROR = 300
} ErrorCode;

#define SET_ARGERROR(x, _msg) \
    do {                      \
        if (!(x)) {           \
            msg = _msg;       \
            code = ARGSERROR; \
            goto err;         \
        }                     \
    } while (0)

#define SET_JS_ERROR(x, _code, _msg) \
    do {                             \
        if (!(x)) {                  \
            msg = _msg;              \
            code = _code;            \
            goto err;                \
        }                            \
    } while (0)

#define ASSERT_RET_ECHO(x, fmt, ...)                   \
    do {                                               \
        if (!(x)) {                                    \
            FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__); \
            return;                                    \
        }                                              \
    } while (0)

#define ASSERT_RET(x) \
    do {              \
        if (!(x)) {   \
            return;   \
        }             \
    } while (0)

#define ASSERT_RET_NULL(x) \
    do {                   \
        if (!(x)) {        \
            return NULL;   \
        }                  \
    } while (0)

#define ASSERT_RET_FALSE(x) \
    do {                    \
        if (!(x)) {         \
            return false;   \
        }                   \
    } while (0)

#define ASSERT_RET_ZERO(x) \
    do {                   \
        if (!(x)) {        \
            return 0;      \
        }                  \
    } while (0)

#define REQUEST_LIST_FOR_EVERY(head, _type) \
    _type *req, *_temp;                     \
    weakref_list_for_every_entry_safe(head, req, _temp, _type, node)

#define GET_FEATURE_AND_CTX(x)                    \
    FeatureInstanceHandle feature = (x)->feature; \
    ft_context_ref ft_ctx = (x)->ft_ctx;

#define INIT_CB (-1)
#define REQUEST_CANCEL 2
#define USER_ABORT_MSG "user cancel request"
#define USER_ABORT_MSG_SIZE (strlen(USER_ABORT_MSG) + 1)
#define CANCEL_ERROR_CODE 1002
#define HTTP_OK 200
#define HTTP_BAD_REQUES 400
#define CONTENT_TYPE "Content-Type"

#ifdef CONFIG_LIB_CURL
/**
 * Map curl error code to custom ErrorCode
 *
 * @param curl_code The curl error code (CURLcode)
 * @return The corresponding ErrorCode
 */
ErrorCode map_curl_to_custom_error(long curl_code);
#endif

#define arrayof(array) sizeof(array) / sizeof(array[0])

#define INVOKE_SUCCESS_CB(cb, ...)                                 \
    do {                                                           \
        if (!FeatureInvokeCallback(feature, cb, ##__VA_ARGS__)) {  \
            FEATURE_LOG_ERROR("invoke success callback failed !"); \
        }                                                          \
    } while (0)

#define INVOKE_FAIL_CB(cb, msg, code)                           \
    do {                                                        \
        if (!FeatureInvokeCallback(feature, cb, msg, code)) {   \
            FEATURE_LOG_ERROR("invoke fail callback failed !"); \
        }                                                       \
    } while (0)

#define INVOKE_COMPLET_CB(cb)                                       \
    do {                                                            \
        if (!FeatureInvokeCallback(feature, cb)) {                  \
            FEATURE_LOG_ERROR("invoke complete callback failed !"); \
        }                                                           \
    } while (0)

#define REMOVE_ALL_CALLBACK(__succ__, __fail__, __complet__) \
    do {                                                     \
        FeatureRemoveCallback(feature, __succ__);            \
        FeatureRemoveCallback(feature, __fail__);            \
        FeatureRemoveCallback(feature, __complet__);         \
    } while (0)

#define check_str(ptr) ((ptr) && strlen(ptr) > 0)
#define check_any(ptr) ((ptr) && (ft_get_type(ft_ctx, *ptr) >= 0))
uint8_t
type_contain(const char** type_array, int size, const char* type, bool ignore_case);

typedef struct {
    uv_request_session_t* handle;
    const char* pkg;
    struct weakref_list_node linklist;
} request_context_t;

bool check_url(FtString url);
request_context_t* get_request_context(FeatureInstanceHandle feature);

bool ft_map_for_every_entry(ft_context_ref ft_ctx, FtAny data, void* userp,
    bool (*user_cb)(const cJSON* const item,
        void* userp));
bool check_header(ft_context_ref ft_ctx, FtAny js_headers,
    std::map<std::string, std::string>& headers);
const char* url_encode(const char* str);
const char* url_decode(const char* str);
ft_value_t ft_form_headers(ft_context_ref ft_ctx, char* headers);
bool check_file_path(const char* pkg, FtString filename,
    std::string& dest_filename);
// }

#endif