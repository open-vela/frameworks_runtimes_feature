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

#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <cassert>
#include <iostream>
#include <set>
#include <string>

#include "net_utils.h"
#include "uploadtask.h"

#define ALL_CALLBACK (-1)

#define TAG "[uploadtask] "

#define UPLOAD_DEBUG(fmt, ...) FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

#define UPLOAD_INFO(fmt, ...) FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

#define UPLOAD_ERROR(fmt, ...) FEATURE_LOG_ERROR(TAG fmt, ##__VA_ARGS__)

#define check_formdata(js_formdata, _formdata) \
    check_header(ft_ctx, js_formdata, _formdata)

typedef struct upload_task_s {
    ft_context_ref ft_ctx;
    FeatureInstanceHandle feature;
    FeatureInterfaceHandle interface_hd;
    bool exit;
    FtCallbackId success_cb;
    FtCallbackId fail_cb;
    FtCallbackId complete_cb;

    off_t last_sent_byte;
    std::set<FtCallbackId> progress_update_cb;
    struct weakref_list_node node;

    uv_request_t* request;
    std::string path;
} upload_task_t;

static void upload_request_cb(int state, uv_response_t* response);
static void uploadtask_free(upload_task_t* upload_task);

void _Interface_uploadtask_abort(FeatureInterfaceHandle feature,
    AppendData data)
{
    upload_task_t* upload_task = static_cast<upload_task_t*>(FeatureGetObjectData(feature));
    ASSERT_RET(upload_task);
    UPLOAD_DEBUG("feature:%p,upload_task:%p", feature, upload_task);
    uv_request_set_userp(upload_task->request, NULL);
    uv_response_t response = {
        .httpcode = REQUEST_CANCEL,
        .headers = NULL,
        .body = (char*)USER_ABORT_MSG,
        .size = USER_ABORT_MSG_SIZE,
        .userp = upload_task,
    };
    upload_request_cb(REQUEST_CANCEL, &response);
}

static int upload_progress_cb(uv_request_t* userp, off_t dltotal, off_t dlnow,
    off_t ultotal, off_t ulnow)
{
    upload_task_t* upload_task = (upload_task_t*)uv_request_get_userp(userp);

    // if onDetached,yes...direct return.
    if (!upload_task) {
        UPLOAD_ERROR("detach failed !");
        return 1;
    }

    // The js object is freed ,yes...direct return.
    if (!upload_task->interface_hd) {
        UPLOAD_ERROR("The js object is freed!");
        return 1;
    }

    system_uploadtask_ProgressUpdateRes res = {
        .progress = 0, .totalBytesSent = 0, .totalBytesExpectedToSend = 0
    };
    if (ultotal != 0) {
        res.totalBytesExpectedToSend = ulnow;
        res.totalBytesSent = ultotal;
        res.progress = 100 * ulnow / ultotal;
    }
    for (const auto& it : upload_task->progress_update_cb) {
        if (FeatureCheckCallbackId(upload_task->interface_hd, it)) {
            if (ulnow != upload_task->last_sent_byte) {
                upload_task->last_sent_byte = ulnow;
                if (!FeatureInvokeCallback(upload_task->interface_hd, it, &res)) {
                    UPLOAD_ERROR("invoke failed !");
                }
            }
        }
    }
    return 0;
}

#if 0
static void clean_progress_update_cb(FeatureInterfaceHandle handle,
                                     FtCallbackId cb) {
  upload_task_t *upload_task =
      static_cast<upload_task_t *>(FeatureGetObjectData(handle));
  assert(upload_task && (handle == upload_task->interface_hd));
  UPLOAD_DEBUG("handle:%p,upload_task:%p,cb:%d", handle, upload_task, cb);
  if (cb < 0) {
    for (const auto &it : upload_task->progress_update_cb) {
      FeatureRemoveCallback(upload_task->interface_hd, it);
    }
    upload_task->progress_update_cb.clear();
  } else {
    if (upload_task->progress_update_cb.find(cb) !=
        upload_task->progress_update_cb.end()) {
      FeatureRemoveCallback(upload_task->interface_hd, cb);
      upload_task->progress_update_cb.erase(cb);
    }
  }
}
#endif

static void upload_request_cb(int state, uv_response_t* response)
{
    upload_task_t* upload_task = static_cast<upload_task_t*>(response->userp);
    ASSERT_RET(upload_task);
    GET_FEATURE_AND_CTX(upload_task)
    UPLOAD_DEBUG("state:%d \nbody:%s ;\nheaders:%s", state, response->body,
        response->headers);
    if (state == UV_REQUEST_DONE) {
        system_uploadtask_SuccessRes res;
        res.statusCode = response->httpcode;
        res.data = response->body;
        ft_value_t ft_header = ft_form_headers(ft_ctx, response->headers);
        res.headers = &ft_header;

        INVOKE_SUCCESS_CB(upload_task->success_cb, &res);

        ft_free_value(ft_ctx, ft_header);
    } else {
        if (state == REQUEST_CANCEL) {
            UPLOAD_INFO(USER_ABORT_MSG);
            uv_request_delete(upload_task->request);
        } else {
            UPLOAD_ERROR("upload err, error code: %d,msg: %s", response->httpcode,
                response->body);
            INVOKE_FAIL_CB(upload_task->fail_cb, response->body, response->httpcode);
        }
    }
    if (state != REQUEST_CANCEL) {
        UPLOAD_DEBUG("upload_task->complete_c:%d", upload_task->complete_cb);
        INVOKE_COMPLET_CB(upload_task->complete_cb);
    }

    upload_task->exit = true;
    // request done,uv_request  has been released
    upload_task->request = NULL;
}

void system_uploadtask_onRegister(const char* feature_name) { UPLOAD_DEBUG(""); }

void system_uploadtask_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    UPLOAD_DEBUG("");
    request_context_t* p = static_cast<request_context_t*>(malloc(sizeof(request_context_t)));
    ASSERT_RET(p);

    p->pkg = FeatureGetPackageName(handle);
    ASSERT_RET(uv_request_init(
                   FeatureGetUVLoop(FeatureGetManagerHandleFromProto(handle)),
                   &(p->handle))
        == 0);
    weakref_list_initialize(&p->linklist);
    FeatureSetProtoData(handle, p);
}

void system_uploadtask_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    UPLOAD_DEBUG("handle:%p", handle);
}
void system_uploadtask_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    UPLOAD_DEBUG("");
    request_context_t* p = get_request_context(handle);
    REQUEST_LIST_FOR_EVERY(&p->linklist, upload_task_t)
    {
        UPLOAD_DEBUG("task:%p,req:%p,status:%d", req, req->request, req->exit);
        if (!req->exit) {
            // Cancel instance callback
            uv_request_set_userp(req->request, NULL);
            req->feature = NULL;
        } else {
            uploadtask_free(req);
        }
    }
    UPLOAD_DEBUG("");
}

void request_cancel(upload_task_t* p)
{
    uv_request_set_userp(p->request, NULL);

    uv_response_t response = {
        .httpcode = CANCEL_ERROR_CODE,
        .headers = NULL,
        .body = (char*)USER_ABORT_MSG,
        .size = USER_ABORT_MSG_SIZE,
        .userp = p,
    };
    upload_request_cb(REQUEST_CANCEL, &response);
}

void system_uploadtask_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    UPLOAD_DEBUG("");
    request_context_t* p = static_cast<request_context_t*>(FeatureGetProtoData(handle));
    assert(p);

    REQUEST_LIST_FOR_EVERY(&p->linklist, upload_task_t)
    {
        UPLOAD_DEBUG("task:%p,request:%p", req, req->request);
        if (req->request) {
            request_cancel(req);
        }
        uploadtask_free(req);
    }
    uv_request_close(p->handle);
    FeatureSetProtoData(p, NULL);
    free(p);
}
void system_uploadtask_onUnregister(const char* feature_name) { UPLOAD_DEBUG(""); }

void _Interface_uploadtask_onProgressUpdate(FeatureInterfaceHandle feature,
    AppendData data, FtCallbackId cb)
{
    UPLOAD_INFO("cb:%d", cb);
    upload_task_t* upload_task = static_cast<upload_task_t*>(FeatureGetObjectData(feature));
    assert(upload_task);
    if (!upload_task->progress_update_cb.insert(cb).second) {
        UPLOAD_INFO("callback %d already exist", cb);
    }
    UPLOAD_DEBUG("callback size:%d\n", upload_task->progress_update_cb.size());
}
// 取消的cb不一致
void _Interface_uploadtask_offProgressUpdate(FeatureInterfaceHandle feature,
    AppendData data, FtCallbackId cb)
{
    upload_task_t* upload_task = static_cast<upload_task_t*>(FeatureGetObjectData(feature));
    assert(upload_task);

    if (!cb) {
        for (const auto& it : upload_task->progress_update_cb) {
            UPLOAD_INFO("Do not support cb:%d", it);
            FeatureRemoveCallback(upload_task->interface_hd, it);
        }
        upload_task->progress_update_cb.clear();
        UPLOAD_INFO("There is no cb!");
        return;
    }

    UPLOAD_INFO("Do not support cb:%d", cb);
    auto it = upload_task->progress_update_cb.find(cb);
    if (it == upload_task->progress_update_cb.end()) {
        UPLOAD_INFO("cb:%d does not exist", cb);
        return;
    }
    FeatureRemoveCallback(feature, *it);
    upload_task->progress_update_cb.erase(it);
}

static void _Interface_uploadtask_finalize(FeatureInterfaceHandle handle)
{
    UPLOAD_DEBUG("");
    upload_task_t* p = static_cast<upload_task_t*>(FeatureGetObjectData(handle));
    ASSERT_RET(p);

    FeatureSetObjectData(handle, NULL);
    p->interface_hd = NULL;
#if 0
// 调用到这里时, 清除回调的子类已经被释放
// 暂时不支持回调
  if (!p->exit) {
   clean_progress_update_cb(handle,ALL_CALLBACK);
  }
#endif
    if (p->exit) {
        uploadtask_free(p);
    }
}

static void uploadtask_free(upload_task_t* upload_task)
{
    ASSERT_RET(upload_task);
    if (upload_task->interface_hd) {
        FeatureSetObjectData(upload_task->interface_hd, NULL);
        upload_task->interface_hd = NULL;
    }
    UPLOAD_DEBUG("del node %p", upload_task);
    weakref_list_delete(&upload_task->node);
    delete upload_task;
}

static upload_task_t* uploadtask_create(FeatureInstanceHandle feature,
    system_uploadtask_FileObject* obj)
{
    upload_task_t* upload_task = new upload_task_t;
    ASSERT_RET_NULL(upload_task);
    upload_task->complete_cb = obj->complete;
    UPLOAD_DEBUG("upload_task->complete_c:%d", upload_task->complete_cb);
    upload_task->fail_cb = obj->fail;
    upload_task->success_cb = obj->success;
    upload_task->last_sent_byte = 0;

    upload_task->feature = feature;
    upload_task->interface_hd = NULL;
    upload_task->ft_ctx = FeatureGetContext(feature);

    upload_task->request = NULL;
    upload_task->exit = false;
    return upload_task;
}

FeatureInterfaceHandle upload_fileobj_create(upload_task_t* upload_task)
{
    // we should combine the vtable
    static NativeFunc uploadtask_vtable_members[] = {
        NativeFunc(_Interface_uploadtask_abort),
        NativeFunc(_Interface_uploadtask_onProgressUpdate),
        NativeFunc(_Interface_uploadtask_offProgressUpdate)
    };
    static VTable uploadtask_vtable = {
        .size = 3,
        .finalizer = NativeFunc(_Interface_uploadtask_finalize),
        .members = uploadtask_vtable_members
    };
    FeatureInterfaceHandle handle = FeatureCreateInterface(upload_task->feature, &uploadtask_vtable);
    if (!handle) {
        return NULL;
    }
    upload_task->interface_hd = handle;
    FeatureSetObjectData(handle, upload_task);
    return handle;
}

static bool upload_request_create(
    upload_task_t* upload_task, system_uploadtask_FileObject* obj,
    std::map<std::string, std::string>& headers,
    std::map<std::string, std::string>& formdata)
{
    request_context_t* p = get_request_context(upload_task->feature);
    // create reques
    ASSERT_RET_NULL(0 == uv_request_create(&upload_task->request));
    UPLOAD_DEBUG("request:%p", upload_task->request);
    // set url
    uv_request_set_url(upload_task->request, obj->url);

    // set header
    for (auto [key, val] : headers) {
        UPLOAD_DEBUG("header: %s", std::string(key + ":" + val).c_str());
        uv_request_append_header(upload_task->request,
            std::string(key + ":" + val).c_str());
    }

    // set upload data
    const char* filename = basename((char*)upload_task->path.c_str());
    UPLOAD_DEBUG("filename:%s, filepath:%s", filename, upload_task->path.c_str());
    uv_request_set_formdata_file(upload_task->request, obj->name, filename,
        upload_task->path.c_str());

    for (auto [key, val] : formdata) {
        UPLOAD_DEBUG("formdata: %s", std::string(key + ":" + val).c_str());

        uv_request_set_formdata_buf(upload_task->request, key.c_str(), "",
            val.c_str(), val.size());
    }

    // set timeout
    uv_request_set_timeout(upload_task->request, obj->timeout);

    // Set request to UV_UPLOAD_TASK
    uv_request_set_atrribute(upload_task->request, UV_UPLOAD_TASK,
        (void*)upload_task->path.c_str());
    // set upload_task to userp
    uv_request_set_userp(upload_task->request, upload_task);

    // set progress callback
    uv_request_set_atrribute(upload_task->request, UV_DOWNLOAD_PROGRESS,
        (void*)upload_progress_cb);

    // start upload
    uv_request_commit(p->handle, upload_task->request, upload_request_cb);

    return true;
}

FeatureInstanceHandle system_uploadtask_wrap_uploadFile(FeatureInstanceHandle feature,
    AppendData data,
    system_uploadtask_FileObject* obj)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    assert(ft_ctx);
    FeatureInterfaceHandle handle = NULL;
    request_context_t* p = get_request_context(feature);
    const char* msg = "";
    int code = 0;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> formdata;
    upload_task_t* upload_task = uploadtask_create(feature, obj);
    SET_JS_ERROR(upload_task, ErrorCode::GENERAL, "new native uploadtask err");

    handle = upload_fileobj_create(upload_task);
    SET_JS_ERROR(handle, ErrorCode::GENERAL, "create js uploadtask err");

    UPLOAD_DEBUG("url:%s,name:%s,filePath:%s", obj->url, obj->name,
        obj->filePath);

    SET_ARGERROR(check_url(obj->url), "invalid url");
    SET_ARGERROR(check_filename(p->pkg, obj->filePath, upload_task->path),
        "invalid filename");
    SET_ARGERROR(check_str(obj->name), "name is a required field");

    if (check_any(obj->header)) {
        SET_ARGERROR(check_header(ft_ctx, obj->header, headers), "invalid headers");
    }

    if (check_any(obj->formData)) {
        SET_ARGERROR(check_formdata(obj->formData, formdata), "invalid formData");
    }

    UPLOAD_DEBUG("feature:%p,handle:%p", feature, handle);

    SET_JS_ERROR(upload_request_create(upload_task, obj, headers, formdata),
        ErrorCode::GENERAL, "create request err");

    weakref_list_initialize(&upload_task->node);
    weakref_list_add_tail(&p->linklist, &upload_task->node);

    return handle;
err:
    if (upload_task) {
        if (upload_task->interface_hd) {
            FeatureSetObjectData(upload_task->interface_hd, NULL);
            upload_task->interface_hd = NULL;
        }
        delete upload_task;
    }
    UPLOAD_DEBUG("msg:%s,code:%d", msg, code);
    INVOKE_FAIL_CB(obj->fail, msg, code);
    INVOKE_COMPLET_CB(obj->complete);
    return handle;
}
