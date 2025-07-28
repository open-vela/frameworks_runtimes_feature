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

#include "exchange.h"
#include "feature.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] exchange_impl";

#define EXCHANGE_PERSIST "persist."
#define EXCHANGE_PERSIST_LEN 8
#define ERROR_CODE 202

enum EXCHAGE_OP {
    EXCHANGE_OP_GET,
    EXCHANGE_OP_SET,
    EXCHANGE_OP_REMOVE,
    EXCHANGE_OP_CLEAR,
    EXCHANGE_OP_NONE,
};

typedef struct {
    FeatureInstanceHandle feature;
    EXCHAGE_OP op; /* The operation */
    FtCallbackId success;
    FtCallbackId fail;
    FtCallbackId complete;
    char* key; /* The key for easily memeory freed */
    char* value; /* The value for easily memory freed */
    char* scope; /* The scope for easily memory freed */
    char* package; /* The package for easily memory freed */
    char* sign; /* The sign of the data provider, SHA-256 */
    char getvalue[PROPERTY_VALUE_MAX + 1]; /* The getvalue buffer to store the value, used to return
                                            * the get value when EXCHAGE_OP = EXCHANGE_OP_GET
                                            */
} ExchangeHandle;

void system_exchange_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_exchange_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_exchange_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_exchange_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_exchange_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_exchange_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

ExchangeHandle* exchange_malloc(FeatureInstanceHandle feature)
{
    ExchangeHandle* handle = (ExchangeHandle*)malloc(sizeof(ExchangeHandle));
    handle->feature = FeatureDupInstanceHandle(feature);
    handle->key = NULL;
    handle->op = EXCHANGE_OP_NONE;
    handle->value = NULL;
    handle->scope = NULL;
    handle->package = NULL;
    handle->sign = NULL;
    handle->complete = -1;
    handle->success = -1;
    handle->fail = -1;
    handle->getvalue[0] = '\0';
    return handle;
}

void exchange_free(ExchangeHandle* handle)
{
    if (handle == NULL) {
        return;
    }
    if (handle->feature) {
        FeatureFreeInstanceHandle(handle->feature);
    }
    if (handle->key) {
        free(handle->key);
    }
    if (handle->value) {
        free(handle->value);
    }
    if (handle->scope) {
        free(handle->scope);
    }
    if (handle->package) {
        free(handle->package);
    }
    if (handle->sign) {
        free(handle->sign);
    }
    free(handle);
    handle = NULL;
}

static void exchange_cb(int status, const char* key, char* value, void* arg)
{
    if (arg == nullptr) {
        return;
    }

    ExchangeHandle* handle = static_cast<ExchangeHandle*>(arg);
    FEATURE_LOG_INFO("[exchange_cb:%d] key=%s,value=%s", handle->op, key, value);
    if (status >= 0) {
        if (value == NULL) {
            FeatureInvokeCallback(handle->feature, handle->success, "success");
        } else {
            if (handle->op == EXCHANGE_OP_GET) {
                system_exchange_GetRet* ret = system_exchangeMallocGetRet();
                char* value_ = (char*)FeatureMalloc(strlen(value) + 1, FT_STRING);
                sprintf(value_, "%s", value);
                ret->value = value_;
                FeatureInvokeCallback(handle->feature, handle->success, ret);
                FeatureFreeValue(ret);
            } else {
                FeatureInvokeCallback(handle->feature, handle->success, value);
            }
        }
    } else {
        FeatureInvokeCallback(handle->feature, handle->fail, "fail", ERROR_CODE);
    }
    FeatureInvokeCallback(handle->feature, handle->complete, (status >= 0) ? "success" : "fail");
    FeatureRemoveCallback(handle->feature, handle->complete);
    FeatureRemoveCallback(handle->feature, handle->success);
    FeatureRemoveCallback(handle->feature, handle->fail);
    exchange_free(handle);
}

static int exchange_args_get_and_check(ExchangeHandle* handle)
{
    int key_len = 0;
    char* tmpcat = NULL;

    /* Get key property and check */
    if (handle->op != EXCHANGE_OP_CLEAR) {
        if (!handle->key || strcmp(handle->key, "") == 0) {
            FEATURE_LOG_ERROR("key is null");
            goto error;
        }
    }

    /* Get value property and check */
    if (handle->op == EXCHANGE_OP_SET) {
        if (handle->value == NULL || strcmp(handle->value, "") == 0) {
            FEATURE_LOG_ERROR("set operation,value property is null");
            goto error;
        }
        if (strlen(handle->value) >= PROPERTY_VALUE_MAX) {
            FEATURE_LOG_ERROR("value exceeded maximum length limit");
            goto error;
        }
    }

    /* Get scope property and check, scope can be NULL or "global" */
    if (handle->scope == NULL) {
        if (handle->package == NULL) {
            goto error;
        } else {
            handle->scope = strdup(handle->package);
        }
    } else if (strcmp(handle->scope, "application") == 0) {
        if ((handle->package == NULL) || (handle->sign == NULL)) {
            FEATURE_LOG_ERROR(
                "exchange must provide the package and sign when scope is application");
            goto error;
        }
        free(handle->scope);
        handle->scope = strdup(handle->package);
    } else if (strcmp(handle->scope, "vendor") == 0 || strcmp(handle->scope, "global") == 0) {
        if (handle->package != NULL || handle->sign != NULL) {
            FEATURE_LOG_ERROR("exchange vendor or global scope not support package and sign");
            goto error;
        }
    } else {
        FEATURE_LOG_ERROR("exchange scope not support, scope=%s", handle->scope);
        goto error;
    }

    /* The total key length is calculated by follows:
     * EXCHANGE_PERSIST_LEN ==> "persist."
     *            scope_len ==> "global" or "vendor" or "com.xiaomi.xxx"
     *                    1 ==> "."
     *                    1 ==> "\0"
     * Final-key = "persist." + scope + "." + input-key
     */
    key_len += EXCHANGE_PERSIST_LEN + strlen(handle->scope) + 1;
    if (handle->key != NULL) {
        key_len += strlen(handle->key) + 1;
    }
    if (key_len > PROPERTY_KEY_MAX) {
        FEATURE_LOG_ERROR("exchange key length (add prefix) too long");
        goto error;
    }
    tmpcat = (char*)FeatureMalloc(key_len, FT_STRING);
    if (tmpcat == NULL) {
        FEATURE_LOG_ERROR("exchange malloc key failed");
        goto error;
    }

    snprintf(tmpcat, key_len, "%s%s%s%s", EXCHANGE_PERSIST, handle->scope, ".", handle->key);
    free(handle->key);
    handle->key = strdup(tmpcat);
    FeatureFreeValue(tmpcat);

    FEATURE_LOG_INFO("op=%d,key=%s, value=%s, scope=%s, package=%s,sign=%s", handle->op,
        handle->key, handle->value, handle->scope, handle->package, handle->sign);
    return 0;
error:
    FEATURE_LOG_ERROR("magic=%d,key=%s,value=%s,scope=%s,package=%s,sign=%s", handle->op,
        handle->key, handle->value, handle->scope, handle->package, handle->sign);
    return ERROR_CODE;
}

static void finish_callback(int status, FeatureInstanceHandle feature, FtCallbackId success_id,
    FtCallbackId fail_id, FtCallbackId complete_id, const char* msg,
    ExchangeHandle* handle)
{
    if (status == 0) {
        FeatureInvokeCallback(feature, success_id, msg);
    } else {
        FeatureInvokeCallback(feature, fail_id, msg, status);
    }
    FeatureInvokeCallback(feature, complete_id, (status >= 0) ? "success" : "fail");
    FeatureRemoveCallback(feature, success_id);
    FeatureRemoveCallback(feature, fail_id);
    FeatureRemoveCallback(feature, complete_id);
    exchange_free(handle);
}

static char* copyStr(const char* src)
{
    if (src == NULL) {
        return NULL;
    }
    return strdup(src);
}

void system_exchange_wrap_set(FeatureInstanceHandle feature, AppendData data, system_exchange_SetInfo* info)
{
    FEATURE_LOG_DEBUG("%s::%s()", file_tag, __FUNCTION__);
    FEATURE_LOG_DEBUG("key=%s,value=%s,scpoe=%s", info->key, info->value, info->scope);
    ExchangeHandle* handle = exchange_malloc(feature);
    if (handle == NULL) {
        FEATURE_LOG_ERROR("[SET] exchange handle malloc failed");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "exchange malloc fail", handle);
    }
    handle->key = copyStr(info->key);
    handle->op = EXCHANGE_OP_SET;
    handle->value = copyStr(info->value);
    handle->scope = copyStr(info->scope);
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;

    int ret = exchange_args_get_and_check(handle);
    if (ret != 0) {
        FEATURE_LOG_ERROR("[SET] exchange input argurments invalid");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "param is invalid", handle);
    }

    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    int status = uv_property_set(FeatureGetUVLoop(manager), handle->key, handle->value, exchange_cb,
        (void*)handle);
    if (status != 0) {
        FEATURE_LOG_ERROR("[SET] uv_property_set failed");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "uv_property_set fail", handle);
    }
}

void system_exchange_wrap_get(FeatureInstanceHandle feature, AppendData data, system_exchange_GetInfo* info)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    FEATURE_LOG_DEBUG("key=%s,scope=%s", info->key, info->scope);
    ExchangeHandle* handle = exchange_malloc(feature);
    if (handle == NULL) {
        FEATURE_LOG_ERROR("[GET] exchange handle malloc failed");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "exchange malloc fail", handle);
    }

    handle->key = copyStr(info->key);
    handle->op = EXCHANGE_OP_GET;
    handle->scope = copyStr(info->scope);
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;

    int ret = exchange_args_get_and_check(handle);
    if (ret != 0) {
        FEATURE_LOG_ERROR("[GET] exchange input argurments invalid");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "param is invalid", handle);
    }

    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    int status = uv_property_get(FeatureGetUVLoop(manager), handle->key, (char*)handle->getvalue, NULL,
        exchange_cb, (void*)handle);
    if (status != 0) {
        FEATURE_LOG_ERROR("[GET] uv_property_get failed");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "uv_property_get failed", handle);
    }
}

void system_exchange_wrap_remove(FeatureInstanceHandle feature, AppendData data,
    system_exchange_RemoveInfo* info)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    FEATURE_LOG_DEBUG("key=%s,scope=%s", info->key, info->scope);
    ExchangeHandle* handle = exchange_malloc(feature);
    if (handle == NULL) {
        FEATURE_LOG_ERROR("[REMOVE] exchange handle malloc failed");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "exchange malloc fail", handle);
    }
    handle->key = copyStr(info->key);
    handle->op = EXCHANGE_OP_REMOVE;
    handle->scope = copyStr(info->scope);
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;

    int ret = exchange_args_get_and_check(handle);
    if (ret != 0) {
        FEATURE_LOG_ERROR("[REMOVE] exchange input argurments invalid");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "param is invalid", handle);
    }

    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    int status = uv_property_delete(FeatureGetUVLoop(manager), handle->key, exchange_cb, (void*)handle);
    if (status != 0) {
        FEATURE_LOG_ERROR("[REMOVE] uv_property_delete failed");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "uv_property_delete failed", handle);
    }
}

void system_exchange_wrap_clear(FeatureInstanceHandle feature, AppendData data, system_exchange_ClearInfo* info)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    ExchangeHandle* handle = exchange_malloc(feature);
    if (handle == NULL) {
        FEATURE_LOG_ERROR("[CLEAR] exchange handle malloc failed");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "exchange malloc fail", handle);
    }
    handle->op = EXCHANGE_OP_CLEAR;
    handle->scope = copyStr(info->scope);
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;
    int ret = exchange_args_get_and_check(handle);
    if (ret != 0) {
        FEATURE_LOG_ERROR("[CLEAR] exchange input argurments invalid");
        return finish_callback(ERROR_CODE, feature, info->success, info->fail, info->complete,
            "param is invalid", handle);
    }

    /* Exchange clear operation only clear current application space data, not support yet */
    finish_callback(0, feature, info->success, info->fail, info->complete, "success", handle);
}
