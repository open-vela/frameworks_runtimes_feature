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
#include "net_utils.h"

#include <nuttx/compiler.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef CONFIG_LIB_CURL
#include <curl/curl.h>
#endif

#include <cassert>
#include <iostream>
#include <map>
#include <ostream>
#include <regex>
#include <string>
#include <sys/types.h>

#include "feature_exports.h"
#include "feature_log.h"

static const char* file_tag = "[net_utils ]";

bool check_url(FtString url)
{
    ASSERT_RET_NULL(check_str(url));
    std::string _url(url);
    std::regex url_regex(
        R"(^http(([^:\/?#]+):)?(//([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
        std::regex::extended);

    std::smatch url_match_result;

    if (std::regex_match(_url, url_match_result, url_regex))
        return true;
    return false;
}
request_context_t* get_request_context(FeatureInstanceHandle feature)
{
    request_context_t* p = static_cast<request_context_t*>(
        FeatureGetProtoData(FeatureGetProtoHandle(feature)));
    assert(p);
    return p;
}

bool ft_map_for_every_entry(ft_context_ref ft_ctx, FtAny data, void* userp,
    bool (*user_cb)(const cJSON* const item,
        void* userp))
{
    ASSERT_RET_NULL(ft_ctx && data && user_cb);
    ASSERT_RET_NULL(ft_get_type(ft_ctx, *data) == FT_TYPE_OBJECT);
    const char* data_str_ = ft_to_string(ft_ctx, *(data));
    cJSON* root = cJSON_Parse(data_str_);
    bool abort = false;
    for (int j = 0; j < cJSON_GetArraySize(root); j++) {
        cJSON* item = cJSON_GetArrayItem(root, j);
        if (!cJSON_IsNull(item)) {
            FEATURE_LOG_DEBUG("%s type:%d,key:%s", file_tag, item->type,
                item->string);
            if (!user_cb(item, userp)) {
                FEATURE_LOG_ERROR("%s user exit", file_tag);
                abort = true;
                break;
            }
        }
    }
    cJSON_Delete(root);
    ft_free_string(ft_ctx, data_str_);
    return !abort;
}

static bool parse_header_cb(const cJSON* const item, void* userp)
{
    std::map<std::string, std::string>* out_headers = static_cast<std::map<std::string, std::string>*>(userp);

    if (out_headers == NULL) {
        return false;
    }
    FEATURE_LOG_DEBUG("%s type:%d", file_tag, item->type);
    if (item->type == cJSON_String) {
        // If is no user data, just check
        if (strcasecmp(item->string, CONTENT_TYPE) == 0 && strstr(item->valuestring, "charset=") == NULL) {
            out_headers->insert(
                std::pair<std::string, std::string>(CONTENT_TYPE, item->valuestring));
            auto it = out_headers->find(CONTENT_TYPE);
            if (it != out_headers->end()) {
                it->second.append("; charset=utf-8");
            }
        } else {
            out_headers->insert(
                std::pair<std::string, std::string>(item->string, item->valuestring));
        }
    } else if (item->type == cJSON_Number) {
        out_headers->insert(
            std::pair<std::string, std::string>(item->string, std::to_string(item->valueint)));
    } else {
        return false;
    }
    return true;
}

bool check_header(ft_context_ref ft_ctx, FtAny js_headers,
    std::map<std::string, std::string>& headers)
{
    if (ft_get_type(ft_ctx, *js_headers) == FT_TYPE_STRING) {
        const char* headers_str_ = ft_to_string(ft_ctx, *(js_headers));
        if (strlen(headers_str_)) {
            return false;
        }
        return true;
    }

    return ft_map_for_every_entry(ft_ctx, js_headers, (void*)&headers,
        parse_header_cb);
}

uint8_t type_contain(const char** type_array, int size, const char* type,
    bool ignore_case)
{
    for (int i = 1; i < size; ++i) {
        if (ignore_case && strcasecmp(type, type_array[i]) == 0) {
            return i;
        } else {
            if (strcmp(type, type_array[i]) == 0) {
                return i;
            }
        }
    }
    return 0;
}

char from_hex(char ch)
{
    return isdigit(ch) ? ch - '0' : toupper(ch) - 'A' + 10;
}

bool is_reserved_char(char c)
{
    static char reserved_char[] = { ';', ',', '/', '?', ':', '@', '&',
        '=', '+', '$', '-', '_', '.', '!',
        '~', '*', '\'', '(', ')', '#' };
    for (size_t i = 0; i < sizeof(reserved_char); i++) {
        if (c == reserved_char[i]) {
            return true;
        }
    }
    return false;
}

const char* url_encode(const char* str)
{
    const char* pstr = str;
    char* buf = (char*)malloc(strlen(str) * 3 + 1);
    assert(buf);
    char* pbuf = buf;
    static char hex[] = "0123456789ABCDEF";

    while (*pstr) {
        if (isalnum(*pstr) || is_reserved_char(*pstr)) {
            *pbuf++ = *pstr;
        } else {
            *pbuf++ = '%';
            *pbuf++ = hex[(*pstr >> 4) & 0x0F];
            *pbuf++ = hex[*pstr & 0x0F];
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}

const char* url_decode(const char* str)
{
    const char* pstr = str;
    char* buf = (char*)malloc(strlen(str) + 1);
    assert(buf);
    char* pbuf = buf;
    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                pstr += 2;
            }
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}

ft_value_t ft_form_headers(ft_context_ref ft_ctx, char* headers)
{
    ft_value_t ret_obj = ft_new_object(ft_ctx);
    char* saveptr;
    char* line = strtok_r(headers, "\n", &saveptr);

    while (line != NULL) {
        char* colon = strchr(line, ':');
        if (colon != NULL) {
            char* key = line;
            char* value = colon + 1;

            // remove value ' '
            if (strlen(key) > (size_t)(colon - line + 1)) {
                key[colon - line + 1] = '\0';
                value++;
            }

            // remove ':'
            key[colon - line] = '\0';

            // remove '\r'
            if (value[strlen(value) - 1] == '\r') {
                value[strlen(value) - 1] = '\0';
            }
            ft_value_t ft_value = ft_from_string(ft_ctx, value);
            ft_obj_set_property(ft_ctx, ret_obj, key, ft_value);
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }
    return ret_obj;
}

bool check_file_path(const char* pkg, FtString filename,
    std::string& dest_filename)
{
    ASSERT_RET_NULL(pkg);
    const char* path = app_relative_to_absolute_path(pkg, (char*)filename);

    if (path == NULL) {
        path = app_absolute_path_generator(pkg, "app", (char*)filename);
        ASSERT_RET_NULL(path);
    }
    dest_filename.assign(path);
    free((void*)path);
    ASSERT_RET_NULL(access(dest_filename.c_str(), F_OK) != -1);
    return true;
}

#ifdef CONFIG_LIB_CURL
ErrorCode map_curl_to_custom_error(long curl_code)
{
    switch (curl_code) {
    // System initialization failure or insufficient memory, mapped to system
    // general error
    case CURLE_FAILED_INIT:
    case CURLE_OUT_OF_MEMORY:
    case CURLE_WRITE_ERROR:
        return ErrorCode::GENERAL;

    // Parameter-related errors, mapped to parameter error
    case CURLE_BAD_FUNCTION_ARGUMENT:
    case CURLE_UNKNOWN_OPTION:
    case CURLE_SETOPT_OPTION_SYNTAX:
        return ErrorCode::ARGSERROR;

    // Operation timed out, mapped to request timeout
    case CURLE_OPERATION_TIMEDOUT:
        return ErrorCode::TIMEOUT;

    // Various read/write related errors, mapped to I/O error
    case CURLE_READ_ERROR:
    case CURLE_RECV_ERROR:
    case CURLE_SEND_ERROR:
        return ErrorCode::IOERROR;

    // Other uncategorized errors, mapped to system general error
    default:
        return ErrorCode::GENERAL;
    }
}
#endif
