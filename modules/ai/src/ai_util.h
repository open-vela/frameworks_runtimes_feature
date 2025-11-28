#include "ai_defs.h"

#include "cJSON.h"

#include <string>

namespace ai {
static const std::string kWhitelistPrefixes = "com.vela.system";
static const char* valid_engine_types[] = { "volcengine", "aliyun" };
static const int valid_engine_types_count = sizeof(valid_engine_types) / sizeof(valid_engine_types[0]);
inline bool whitelistVerifi(const std::string& str)
{
    return str.size() >= kWhitelistPrefixes.size() && str.compare(0, kWhitelistPrefixes.size(), kWhitelistPrefixes) == 0;
}

inline bool setupAuthFromConfig(const cJSON* root, const std::string& engine_type, ai_auth_t& ai_auth)
{
    if (engine_type == "volcengine") {
        cJSON* app_key = cJSON_GetObjectItem(root, "accessKey");
        cJSON* app_id = cJSON_GetObjectItem(root, "appKey");
        cJSON* module_name = cJSON_GetObjectItem(root, "model");
        if (!app_key || !cJSON_IsString(app_key) || !app_id || !cJSON_IsString(app_id)) {
            return false;
        }
        if (!module_name || !cJSON_IsString(module_name)) {
            return false;
        }

        ai_volc_auth_t* auth = (ai_volc_auth_t*)malloc(sizeof(ai_volc_auth_t));
        if (!auth) {
            return false;
        }
        auth->version = 1;
        auth->app_id = app_id->valuestring;
        auth->app_key = app_key->valuestring;
        auth->model = module_name->valuestring;

        ai_auth.version = 1;
        ai_auth.engine_type = 0;
        ai_auth.auth = auth;
        return true;
    } else if (engine_type == "aliyun") {
        // TODO: 实现阿里云相关赋值逻辑
        cJSON* api_key = cJSON_GetObjectItem(root, "apiKey");
        cJSON* module_name = cJSON_GetObjectItem(root, "model");
        if (!api_key || !cJSON_IsString(api_key) || !module_name || !cJSON_IsString(module_name)) {
            return false;
        }

        ai_ali_auth_t* auth = (ai_ali_auth_t*)malloc(sizeof(ai_ali_auth_t));
        if (!auth) {
            return false;
        }
        auth->version = 1;
        auth->API_Key = api_key->valuestring;
        auth->model = module_name->valuestring;

        ai_auth.version = 1;
        ai_auth.engine_type = 1;
        ai_auth.auth = auth;
        return true;
    }

    return false; // 其它engine_type暂不支持
}

inline bool checkEngine(const char* engine_type)
{
    if (engine_type == nullptr) {
        return true;
    }

    for (int i = 0; i < valid_engine_types_count; ++i) {
        if (strcmp(engine_type, valid_engine_types[i]) == 0) {
            return true;
        }
    }

    return false;
}
} // namespace ai