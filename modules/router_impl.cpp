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

#include <assert.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "feature_exports.h"
#include "router.h"
#ifdef CONFIG_QUICKAPP_VAPP_XMS
#include "app/Context.h"
#endif

#include "framework/app_interface.h"

static const char* file_tag = "[jidl_feature] router";
static const char* CLEAR_TASK = "___PARAM_LAUNCH_FLAG___=clearTask";

#ifdef CONFIG_QUICKAPP_VAPP_XMS
static const char* quickapp_prefix = "hap://app/";
#endif

using namespace ferry;

#ifdef CONFIG_MIWEAR_APPS
extern "C" int quickapp_navigate_async(const char* uri, const char* arg);
#endif

static char* str_to_ftstr(const char* str, const int len)
{
    char* ftstr = (char*)FeatureMalloc(len + 1, FT_STRING);
    strncpy(ftstr, str, len);
    ftstr[len] = 0;
    return ftstr;
}

static void get_prop_string(ft_context_ref ft_ctx, ft_value_t ft_val, const char* prop, std::string& out_str)
{
    ft_value_t value = ft_obj_get_property(ft_ctx, ft_val, prop);
    if (ft_get_type(ft_ctx, value) == FT_TYPE_STRING) {
        const char* c_str = ft_to_string(ft_ctx, value);
        out_str = c_str;
        ft_free_string(ft_ctx, c_str);
    }
    ft_free_value(ft_ctx, value);
}

// FeatureCallbacks to be implemented
void system_router_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_router_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_router_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_router_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_router_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void system_router_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
}

void replaceNullsInObject(rapidjson::Value& nestedObj, rapidjson::Document::AllocatorType& allocator)
{
    for (auto nestedIt = nestedObj.MemberBegin(); nestedIt != nestedObj.MemberEnd(); ++nestedIt) {
        if (nestedIt->value.IsNull()) {
            nestedIt->value.SetString("", allocator);
        } else if (nestedIt->value.IsObject()) {
            replaceNullsInObject(nestedIt->value, allocator);
        }
    }
}

void jsonToQueryString(const rapidjson::Document& doc, std::string& result)
{
    result.clear();

    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        const char* key = it->name.GetString();
        if (!result.empty()) {
            result += "&"; // Add separator between key-value pairs
        }

        if (it->value.IsObject()) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

            // Create a new document for nested serialization
            rapidjson::Document nestedDoc;
            nestedDoc.CopyFrom(it->value, nestedDoc.GetAllocator());

            // Replace nulls in the nested document
            replaceNullsInObject(nestedDoc, nestedDoc.GetAllocator());

            // Serialize the modified nested document
            nestedDoc.Accept(writer);
            std::string nested_value = buffer.GetString();

            result += std::string(key) + "=" + nested_value;
        } else if (it->value.IsString()) {
            std::string value = it->value.GetString();
            result += std::string(key) + "=" + (value.empty() ? "\"\"" : value);
        } else if (it->value.IsInt() || it->value.IsUint() || it->value.IsInt64() || it->value.IsUint64() || it->value.IsFloat() || it->value.IsDouble()) {
            result += std::string(key) + "=" + std::to_string(it->value.GetDouble());
        } else if (it->value.IsBool()) {
            result += std::string(key) + "=" + (it->value.GetBool() ? "true" : "false");
        } else if (it->value.IsNull()) {
            result += std::string(key) + "=" + "\"\"";
        } else {
            FEATURE_LOG_ERROR("route param value format error");
        }
    }
}

int parse_routerobj(RouteInfo& info, ft_context_ref ft_ctx, system_router_RouteObj* obj, std::string* query_result_ptr = nullptr)
{
    /* The quick application standard stipulates that the uri must be filled in, but when using it, the uri may be empty,
    which will cause a crash */
    if (obj->uri == nullptr) {
        FEATURE_LOG_ERROR("router uri is null");
        return -1;
    }
    if (Navigator::getRouteInfoFromUri(&info, obj->uri) != 0) {
        FEATURE_LOG_ERROR("router uri format error:%s", obj->uri);
        return -1;
    }
    /* if find CLEAR_TASK from obj->uri, set info.param.clear_task = true */
    if (obj->uri && strstr(obj->uri, CLEAR_TASK) != nullptr) {
        info.param.clear_task = true;
        FEATURE_LOG_DEBUG("CLEAR_TASK parameter found in URI: %s", obj->uri);
    }

    if (!obj->params) {
        FEATURE_LOG_WARN("route not params");
        return 0;
    }

    std::string launch_mode;
    get_prop_string(ft_ctx, *obj->params, "___PARAM_LAUNCH_FLAG___", launch_mode);
    if (launch_mode == "clearTask") {
        info.param.clear_task = true;
    }

    ft_value_t page_anim = ft_obj_get_property(ft_ctx, *obj->params, "___PARAM_PAGE_ANIMATION___");
    if (ft_get_type(ft_ctx, page_anim) == FT_TYPE_OBJECT) {
        std::string anim_value;
        get_prop_string(ft_ctx, page_anim, "openEnter", anim_value);
        if (anim_value == "none") {
            info.param.open_enter = RouteInfo::PageAnimation::None;
        }
        get_prop_string(ft_ctx, page_anim, "closeEnter", anim_value);
        if (anim_value == "none") {
            info.param.close_enter = RouteInfo::PageAnimation::None;
        }
        get_prop_string(ft_ctx, page_anim, "openExit", anim_value);
        if (anim_value == "none") {
            info.param.open_exit = RouteInfo::PageAnimation::None;
        }
        get_prop_string(ft_ctx, page_anim, "closeExit", anim_value);
        if (anim_value == "none") {
            info.param.close_exit = RouteInfo::PageAnimation::None;
        }
    }
    ft_free_value(ft_ctx, page_anim);

    // uri 格式中可以存放变量，在params中也可以存放变量。这里把两处合并起来
    rapidjson::Document uri_param;
    uri_param.Parse(info.param.body.c_str()); // uri前面解析的数据在body中
    const char* body_params = ft_to_string(ft_ctx, *obj->params);
    rapidjson::Document user_param;
    if (body_params) {
        user_param.Parse(body_params);
        // page 参数中不需要下面两项
        user_param.RemoveMember("___PARAM_LAUNCH_FLAG___");
        user_param.RemoveMember("___PARAM_PAGE_ANIMATION___");
        rapidjson::Document::AllocatorType& alloc = user_param.GetAllocator();
        for (auto it = uri_param.MemberBegin(); it != uri_param.MemberEnd(); ++it) {
            user_param.AddMember(it->name, it->value, alloc);
        }
        ft_free_string(ft_ctx, body_params);
    }

    if (query_result_ptr) {
        /* Originally: *query_result_ptr = convertJsonToQueryString(user_param); However, it will cause a crash during unit testing,
        which may be caused by insufficient stack size when calling multiple times */
        jsonToQueryString(user_param, *query_result_ptr);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    user_param.Accept(writer);
    info.param.body = buffer.GetString();
    return 0;
}

// Function wrappers to be implemented
void system_router_wrap_push(FeatureInstanceHandle feature, AppendData append_data, system_router_RouteObj* obj)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    assert(app != nullptr);
    Navigator* navigator = app->navigator();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    std::string queryString;
    RouteInfo router_info;
    router_info.clear();
    if (parse_routerobj(router_info, ft_ctx, obj, &queryString) == 0) {
        if (router_info.package.empty()) {
            router_info.package = app->packageName();
        } else if (router_info.package != app->packageName()) {
// jump to other app
#ifdef CONFIG_MIWEAR_APPS
            const std::string target = router_info.package + router_info.pagename;
            rapidjson::Document userParam;
            userParam.Parse(router_info.param.body.c_str());
            jsonToQueryString(userParam, queryString);
            quickapp_navigate_async(target.c_str(), queryString.c_str());
            return;
#endif
#ifdef CONFIG_QUICKAPP_VAPP_XMS
            std::string uri_str(obj->uri);

            if (uri_str.find(quickapp_prefix) != 0) {
                FEATURE_LOG_ERROR("uri should start with %s!", quickapp_prefix);
                return;
            }

            size_t pos_pkg = strlen(quickapp_prefix);
            size_t pos_path = uri_str.find('/', pos_pkg);
            size_t pos_query = uri_str.find('?', pos_pkg);
            std::string pkg;

            if (pos_path == std::string::npos && pos_query == std::string::npos) {
                pkg = uri_str.substr(pos_pkg);
            } else if (pos_path == std::string::npos && pos_query != std::string::npos) {
                pkg = uri_str.substr(pos_pkg, pos_query - pos_pkg);
            } else {
                pkg = uri_str.substr(pos_pkg, pos_path - pos_pkg);
            }

            os::app::Intent intent;
            intent.setTarget(pkg);
            std::string path_with_params;

            if (pos_path != std::string::npos) {
                path_with_params = uri_str.substr(pos_path, (pos_query != std::string::npos ? pos_query : uri_str.length()) - pos_path);
            }

            if (obj->params || pos_query != std::string::npos) {
                path_with_params += (obj->params ? "?" + queryString : uri_str.substr(pos_query));
            }
            intent.setData(path_with_params);

            os::app::Context* xms_context = static_cast<os::app::Context*>(app->getXmsContext());
            if (xms_context == NULL) {
                FEATURE_LOG_ERROR("no xms_context");
                return;
            }
            xms_context->startActivity(intent);
            return;
#endif
            FEATURE_LOG_ERROR("Failed to handle quickapp uri: %s, app package: %s", obj->uri, router_info.package.c_str());
            return;
        }

        navigator->push(router_info);
    }
}

void system_router_wrap_replace(FeatureInstanceHandle feature, AppendData append_data, system_router_RouteObj* obj)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    assert(app != nullptr);
    Navigator* navigator = app->navigator();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    RouteInfo router_info;
    router_info.clear();
    if (parse_routerobj(router_info, ft_ctx, obj) == 0) {
        if (router_info.package.empty()) {
            router_info.package = app->packageName();
        }
        navigator->replace(router_info);
    }
}

void system_router_wrap_back(FeatureInstanceHandle feature, AppendData append_data, system_router_BackObj* obj)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    assert(app != nullptr);
    Navigator* navigator = app->navigator();
    if (obj && obj->path) {
        navigator->back(std::string(obj->path));
    } else {
        navigator->back();
    }
}

void system_router_wrap_clear(FeatureInstanceHandle feature, AppendData append_data)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    assert(app != nullptr);
    Navigator* navigator = app->navigator();
    navigator->clear();
}

FtInt system_router_wrap_getLength(FeatureInstanceHandle feature, AppendData append_data)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    assert(app != nullptr);
    Navigator* navigator = app->navigator();
    return navigator->getLength();
}

system_router_RouteState* system_router_wrap_getState(FeatureInstanceHandle feature, AppendData append_data)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    assert(app != nullptr);
    Navigator* navigator = app->navigator();
    system_router_RouteState* result = system_routerMallocRouteState();
    NavigatorInfo state;
    if (navigator->getState(&state) != 0) {
        result->index = -1;
        result->name = nullptr;
        result->path = nullptr;
    } else {
        result->index = state.index;
        result->name = str_to_ftstr(state.name.c_str(), state.name.length());
        result->path = str_to_ftstr(state.path.c_str(), state.path.length());
    }

    return result;
}

FtArray* system_router_wrap_getPages(FeatureInstanceHandle feature, AppendData append_data)
{
    FEATURE_LOG_INFO("%s::%s()", file_tag, __FUNCTION__);
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    assert(app != nullptr);
    Navigator* navigator = app->navigator();

    /* malloc pageArray type */
    FtArray* pageArray = system_router_malloc_RouteStack_struct_type_array();
    /* set array size */
    pageArray->_size = navigator->getLength();
    /* malloc array element obj type */
    pageArray->_element = malloc(sizeof(system_router_RouteStack*) * pageArray->_size);

    /* fill array with system_router_RouteStack obj */
    for (size_t i = 0; i < (size_t)navigator->getLength(); i++) {
        system_router_RouteStack* elem = system_routerMallocRouteStack();
        NavigatorInfo state;
        navigator->getStateByIndex(&state, i);
        elem->name = str_to_ftstr(state.name.c_str(), state.name.length());
        elem->path = str_to_ftstr(state.path.c_str(), state.path.length());

        ((system_router_RouteStack**)pageArray->_element)[i] = elem;
    }
    return pageArray;
}
