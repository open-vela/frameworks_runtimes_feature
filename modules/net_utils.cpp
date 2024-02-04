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

#include <iostream>
#include <map>
#include <ostream>
#include <regex>
#include <string>

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
            FEATURE_LOG_ERROR("%s type:%d,key:%s,val:%s", file_tag, item->type,
                item->string, item->valuestring);
            if (!user_cb(item, userp)) {
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

    FEATURE_LOG_DEBUG("%s type:%d", file_tag, item->type);
    if (item->type != cJSON_String)
        return false;
    // If is no user data, just check
    if (out_headers) {
        out_headers->insert(
            std::pair<std::string, std::string>(item->string, item->valuestring));
    }
    return true;
}

bool check_header(ft_context_ref ft_ctx, FtAny js_headers,
    std::map<std::string, std::string>& headers)
{
    return ft_map_for_every_entry(ft_ctx, js_headers, (void*)&headers,
        parse_header_cb);
}

bool type_contain(const char** type_array, int size, const char* type)
{
    for (int i = 1; i < size; ++i) {
        if (strcmp(type, type_array[i]) == 0)
            return true;
    }
    return false;
}