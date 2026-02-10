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

#include "quickapp_inspector.h"
#include "storage.h"
#include "unqlite.h"
#include "uv_ext.h"

static const char* file_tag = "[jidl_feature] storage_impl";

#ifdef CONFIG_HAP_APP_PATH
#define DB_PATH_PREFIX CONFIG_HAP_APP_PATH
#else
#define DB_PATH_PREFIX "/data/quickapp"
#endif

typedef struct {
    uv_db_t* db;
    char* db_path;
} StorageContext;

enum STORAGE_OP {
    STORAGE_OP_GET,
    STORAGE_OP_SET,
    STORAGE_OP_CLEAR,
    STORAGE_OP_DELETE,
    STORAGE_OP_KEY,
    STORAGE_OP_SET_TO_DELETE,
    STORAGE_OP_NONE
};

typedef struct {
    FeatureInstanceHandle feature;
    StorageContext* th;
    STORAGE_OP op;
    FtCallbackId success;
    FtCallbackId fail;
    FtCallbackId complete;
    uv_buf_t buf;
} StorageHandle;

#define STORAGE_CHECK_IF(ret, msg)                                               \
    if (ret) {                                                                   \
        FEATURE_LOG_ERROR("[STORAGE_CHECK_IF] %s", msg);                         \
        finish_callback(ret, feature, info->success, info->fail, info->complete, \
            msg, handle);                                                        \
    }

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

#define INVOKE_COMPLET_CB(cb, ...)                                  \
    do {                                                            \
        if (!FeatureInvokeCallback(feature, cb, ##__VA_ARGS__)) {   \
            FEATURE_LOG_ERROR("invoke complete callback failed !"); \
        }                                                           \
    } while (0)

#define REMOVE_ALL_CBS(success, fail, complete)   \
    do {                                          \
        FeatureRemoveCallback(feature, success);  \
        FeatureRemoveCallback(feature, fail);     \
        FeatureRemoveCallback(feature, complete); \
    } while (0)

int checkpath(const char* path)
{
    const char s[] = "/";
    char* data;
    char *token, *ret;
    char* savedptr = NULL;
    int res;

    data = (char*)malloc(PATH_MAX);
    if (data == NULL) {
        return -ENOMEM;
    }

    res = access(path, F_OK);
    if (res == 0) {
        free(data);
        return 0;
    }

    strcpy(data, path);
    ret = strrchr(data, '/');
    if (ret == 0) {
        free(data);
        return 0;
    }
    *ret++ = 0;
    // use strtok_r ranther than strtok because strtok_r is thread safe
    token = strtok_r(data, s, &savedptr);
    while (token != NULL) {
        token = strtok_r(NULL, s, &savedptr);
        if (token != NULL) {
            *(token - 1) = '/';
        }

        res = access(data, F_OK);
        if (res != 0) {
            res = mkdir(data, 0777);
        }
    }

    free(data);
    return res;
}

void system_storage_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_storage_onCreate(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    StorageContext* th = (StorageContext*)malloc(sizeof(StorageContext));
    const char* packageName = FeatureGetPackageName(handle);
    char name[PATH_MAX] = { 0 };
    sprintf(name, DB_PATH_PREFIX "/%s/%s/%s", "system", packageName, "usr.db");
    th->db_path = (char*)malloc(strlen(name) + 1);
    strcpy(th->db_path, name);
    checkpath(th->db_path);
    FeatureManagerHandle manager = FeatureGetManagerHandleFromProto(handle);
    int ret = uv_db_init(FeatureGetUVLoop(manager), &th->db, name);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s() uv_db_init error:%d\n", file_tag, __FUNCTION__,
            ret);
    }
    InspectHostStorageInit(th->db);
    FeatureSetProtoData(handle, th);
}

void system_storage_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_storage_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_storage_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(handle));
    if (th) {
        if (th->db) {
            uv_db_close(th->db);
        }
        if (th->db_path) {
            free(th->db_path);
        }
    }
    free(th);
}

void system_storage_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

FtInt system_storage_get_length(void* feature, AppendData data)
{
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(static_cast<FeatureInstanceHandle>(feature));
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(proto_handle));
    char* key;
    return uv_db_key(th->db, -1, &key, NULL, NULL);
}

void system_storage_set_length(void* feature, AppendData data, FtInt length) { }

static StorageHandle* storage_malloc(FeatureInstanceHandle feature)
{
    StorageHandle* handle = (StorageHandle*)malloc(sizeof(StorageHandle));
    handle->feature = FeatureDupInstanceHandle(feature);
    handle->th = NULL;
    handle->buf = { 0 };
    handle->op = STORAGE_OP_NONE;
    handle->success = -1;
    handle->fail = -1;
    handle->complete = -1;
    return handle;
}

static void storage_free(StorageHandle* handle)
{
    if (handle == NULL) {
        return;
    }
    if (handle->feature) {
        FeatureFreeInstanceHandle(handle->feature);
    }
    if (handle->buf.base != NULL) {
        free(handle->buf.base);
        handle->buf.base = NULL;
    }
    free(handle);
}

static void finish_callback(int status, FeatureInstanceHandle feature,
    FtCallbackId success_id, FtCallbackId fail_id,
    FtCallbackId complete_id, const char* msg,
    StorageHandle* handle)
{
    if (!FeatureInstanceIsDetached(feature)) {
        if (status == 0) {
            INVOKE_SUCCESS_CB(success_id, msg != NULL ? msg : "success");
        } else {
            INVOKE_FAIL_CB(fail_id, msg, status);
        }
        INVOKE_COMPLET_CB(complete_id, (status >= 0) ? "success" : "fail");
        REMOVE_ALL_CBS(success_id, fail_id, complete_id);
    }
    storage_free(handle);
}

static void storage_cb(int status, const char* key, uv_buf_t value,
    void* data)
{
    StorageHandle* handle = static_cast<StorageHandle*>(data);
    if (handle == NULL) {
        return;
    }
    const char* ret = "0";
    /*when the key does not exist,the unqlite return -6
    but storage feature require default value,not fail*/
    if (status == UNQLITE_NOTFOUND && handle->op == STORAGE_OP_GET) {
        status = 0;
        ret = handle->buf.base;
        finish_callback(status, handle->feature, handle->success, handle->fail,
            handle->complete, ret, handle);
        free(const_cast<char*>(key));
        return;
    }
    /*If the new value is an empty string of length 0, the data item indexed by key is deleted.
     but delete operator maybe return fail,storage feature require return success*/
    if (status == UNQLITE_NOTFOUND && handle->op == STORAGE_OP_SET_TO_DELETE) {
        status = 0;
        finish_callback(status, handle->feature, handle->success, handle->fail,
            handle->complete, ret, handle);
        free(const_cast<char*>(key));
        return;
    }
    if (status == 0) {
        if (handle->op == STORAGE_OP_GET) {
            ret = value.base;
        } else if (handle->op == STORAGE_OP_KEY) {
            ret = key;
        }
        finish_callback(status, handle->feature, handle->success, handle->fail,
            handle->complete, ret, handle);
    } else {
        if (status == UNQLITE_NOTFOUND && handle->op == STORAGE_OP_DELETE) {
            status = 200;
        }
        finish_callback(status, handle->feature, handle->success, handle->fail,
            handle->complete, uv_strerror(status), handle);
    }
    free(const_cast<char*>(key));
}

void system_storage_wrap_get_sync(FeatureInstanceHandle feature, AppendData data,
    system_storage_GetInfo* info)
{
    FEATURE_LOG_INFO("[STORAGE_GET_SYNC] key=%s,default=%s", info->key,
        info->_default);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(proto_handle));

    if (th == NULL) {
        FEATURE_LOG_ERROR("[STORAGE_GET_SYNC] FeatureGetObjectData fail");
        if (!FeatureInstanceIsDetached(feature)) {
            INVOKE_FAIL_CB(info->fail, "FeatureGetObjectData fail", -1);
            INVOKE_COMPLET_CB(info->complete, "fail");
            REMOVE_ALL_CBS(info->success, info->fail, info->complete);
        }
        return;
    }

    if ((info->key == NULL) || strcmp(info->key, "") == 0) {
        FEATURE_LOG_ERROR("[STORAGE_GET_SYNC] key is empty");
        if (!FeatureInstanceIsDetached(feature)) {
            INVOKE_FAIL_CB(info->fail, "key is empty", 202);
            INVOKE_COMPLET_CB(info->complete, "fail");
            REMOVE_ALL_CBS(info->success, info->fail, info->complete);
        }
        return;
    }

    uv_buf_t value = { .base = NULL, .len = 0 };
    int status = uv_db_get(th->db, info->key, &value, NULL, NULL);

    if (FeatureInstanceIsDetached(feature)) {
        if (value.base) {
            free(value.base);
        }
        return;
    }

    if (status == 0) {
        INVOKE_SUCCESS_CB(info->success, value.base);
        INVOKE_COMPLET_CB(info->complete, "success");
    } else if (status == UNQLITE_NOTFOUND) {
        const char* ret = (info->_default != NULL) ? info->_default : "";
        INVOKE_SUCCESS_CB(info->success, ret);
        INVOKE_COMPLET_CB(info->complete, "success");
    } else {
        INVOKE_FAIL_CB(info->fail, uv_strerror(status), status);
        INVOKE_COMPLET_CB(info->complete, "fail");
    }
    REMOVE_ALL_CBS(info->success, info->fail, info->complete);

    if (value.base) {
        free(value.base);
    }
}

void system_storage_wrap_get(FeatureInstanceHandle feature, AppendData data,
    system_storage_GetInfo* info)
{
    FEATURE_LOG_INFO("[STORAGE_GET] key=%s,default=%s", info->key,
        info->_default);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(proto_handle));
    StorageHandle* handle = storage_malloc(feature);
    if (th == NULL || handle == NULL) {
        FEATURE_LOG_ERROR("[STORAGE_GET]  FeatureGetObjectData fail");
        return finish_callback(-1, feature, info->success, info->fail,
            info->complete, "FeatureGetObjectData fail", handle);
    }

    if ((info->key == NULL) || strcmp(info->key, "") == 0) {
        FEATURE_LOG_ERROR("[STORAGE_GET]  key is empty");
        return finish_callback(-1, feature, info->success, info->fail,
            info->complete, info->_default, handle);
    }

    handle->th = th;
    handle->op = STORAGE_OP_GET;
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;
    if (info->_default != NULL) {
        handle->buf.base = strdup(info->_default);
        handle->buf.len = strlen(info->_default);
    }
    int status = uv_db_get(th->db, strdup(info->key), NULL, storage_cb, handle);
    STORAGE_CHECK_IF(status, "uv_db_get fail");
}

void system_storage_wrap_set(FeatureInstanceHandle feature, AppendData data,
    system_storage_SetInfo* info)
{
    FEATURE_LOG_INFO("[STORAGE_SET] key=%s,value=%s", info->key, info->value);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(proto_handle));
    StorageHandle* handle = storage_malloc(feature);
    if (th == NULL || handle == NULL) {
        FEATURE_LOG_ERROR("[STORAGE_SET] FeatureGetObjectData fail");
        return finish_callback(202, feature, info->success, info->fail,
            info->complete, "FeatureGetObjectData fail", handle);
    }

    if ((info->key == NULL) || strcmp(info->key, "") == 0) {
        FEATURE_LOG_ERROR("[STORAGE_SET]  key is empty");
        return finish_callback(202, feature, info->success, info->fail,
            info->complete, "fail", handle);
    }

    if (info->value == NULL) {
        FEATURE_LOG_ERROR("[STORAGE_SET]  value is null");
        return finish_callback(202, feature, info->success, info->fail,
            info->complete, "fail", handle);
    }

    handle->th = th;
    handle->op = STORAGE_OP_SET;
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;
    if (info->value != NULL) {
        handle->buf.len = strlen(info->value);
        handle->buf.base = (char*)malloc(handle->buf.len + 1);
        memcpy(handle->buf.base, info->value, handle->buf.len);
        handle->buf.base[handle->buf.len] = '\0';
    }

    int status = 0;
    // if value is empty,delete key
    if (strcmp(info->value, "") == 0) {
        handle->op = STORAGE_OP_SET_TO_DELETE;
        status = uv_db_delete(th->db, strdup(info->key), storage_cb, handle);
    } else {
        status = uv_db_set(th->db, strdup(info->key), &handle->buf, storage_cb, handle);
    }
    STORAGE_CHECK_IF(status, "uv_db_set fail");
}

void system_storage_wrap_clear(FeatureInstanceHandle feature, AppendData data,
    system_storage_ClearInfo* info)
{
    FEATURE_LOG_INFO("[STORAGE_CLEAR]");
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(proto_handle));
    StorageHandle* handle = storage_malloc(feature);
    if (th == NULL) {
        FEATURE_LOG_ERROR("[STORAGE_CLEAR] FeatureGetObjectData fail");
        finish_callback(-1, feature, info->success, info->fail, info->complete,
            "FeatureGetObjectData fail", handle);
        return;
    }
    int ret = uv_db_close(th->db);
    STORAGE_CHECK_IF(ret, "uv_db_close fail");
    ret = unlink(th->db_path);
    FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(feature);
    ret = uv_db_init(FeatureGetUVLoop(manager), &th->db, th->db_path);
    STORAGE_CHECK_IF(ret, "uv_db_init fail");
    finish_callback(ret, feature, info->success, info->fail, info->complete,
        "succcess", handle);
}

void system_storage_wrap_delete(FeatureInstanceHandle feature, AppendData data,
    system_storage_DeleteInfo* info)
{
    FEATURE_LOG_INFO("[STORAGE_DELETE] key=%s", info->key);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(proto_handle));
    StorageHandle* handle = storage_malloc(feature);
    if (th == NULL || handle == NULL) {
        FEATURE_LOG_ERROR("[STORAGE_DELETE] FeatureGetObjectData fail");
        return finish_callback(202, feature, info->success, info->fail,
            info->complete, "FeatureGetObjectData fail", handle);
    }
    if ((info->key == NULL) || strcmp(info->key, "") == 0) {
        FEATURE_LOG_ERROR("[STORAGE_DELETE]  key is empty");
        return finish_callback(202, feature, info->success, info->fail,
            info->complete, "fail", handle);
    }

    handle->th = th;
    handle->op = STORAGE_OP_DELETE;
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;

    int ret = uv_db_delete(th->db, strdup(info->key), storage_cb, handle);
    STORAGE_CHECK_IF(ret, "uv_db_delete fail");
}

void system_storage_wrap_key(FeatureInstanceHandle feature, AppendData data,
    system_storage_KeyInfo* info)
{
    FEATURE_LOG_INFO("[STORAGE_KEY] index=%d", info->index);
    FeatureProtoHandle proto_handle = FeatureGetProtoHandle(feature);
    StorageContext* th = static_cast<StorageContext*>(FeatureGetProtoData(proto_handle));
    StorageHandle* handle = storage_malloc(feature);
    if (th == NULL || handle == NULL) {
        FEATURE_LOG_ERROR("[STORAGE_KEY] FeatureGetObjectData fail");
        return finish_callback(-1, feature, info->success, info->fail,
            info->complete, "FeatureGetObjectData fail", handle);
    }
    if (info->index < 0) {
        FEATURE_LOG_ERROR("[STORAGE_KEY]  index Less than 0");
        return finish_callback(-1, feature, info->success, info->fail,
            info->complete, "fail", handle);
    }

    handle->th = th;
    handle->op = STORAGE_OP_KEY;
    handle->success = info->success;
    handle->fail = info->fail;
    handle->complete = info->complete;
    int ret = uv_db_key(th->db, info->index, NULL, storage_cb, handle);
    if (ret < 0) {
        FEATURE_LOG_ERROR("[STORAGE_KEY]  uv_db_key fail");
        finish_callback(ret, feature, info->success, info->fail, info->complete,
            "uv_db_key fail", handle);
    }
}
