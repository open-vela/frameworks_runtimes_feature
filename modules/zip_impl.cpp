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

#include "app_path.h"
#include "feature_config.h"
#include "feature_utils.h"
#include "minizip/unzip.h"
#include "zip.h"
#include <atomic>
#include <utime.h>
#include <uv.h>
#include <vector>

#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

static const char* file_tag = "[jidl_feature] zip_impl";

/* callback api */
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

typedef struct {
    uv_work_t req;
    std::atomic<bool> unzip_success_flag = false;
    char* src_path; // src pach
    char* dst_path; // dst pach
    int success;
    int fail;
    int complete;
    FeatureInstanceHandle handle;
} zipReq;

struct ZipContext {
    uv_loop_t* loop;
    const char* pkg_name;
    std::vector<zipReq*> zr_arr;

    ZipContext(uv_loop_t* loop_, const char* pkg_name_)
        : loop(loop_)
        , pkg_name(pkg_name_)
        , zr_arr()
    {
    }
};

static void freeZipReq(zipReq* zr)
{
    if (zr == NULL) {
        return;
    }
    if (zr->handle)
        FeatureFreeInstanceHandle(zr->handle);
    if (zr->src_path)
        free(zr->src_path);
    if (zr->dst_path)
        free(zr->dst_path);
    free(zr);
    zr = nullptr;
}

ZipContext* getZipContext(FeatureInstanceHandle handle)
{
    void* user_data = FeatureGetObjectData(handle);
    assert(user_data != nullptr);
    return static_cast<ZipContext*>(user_data);
}

void system_zip_onRegister(const char* feature_name)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_zip_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_zip_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    ZipContext* zc = (ZipContext*)FeatureGetObjectData(handle);
    if (zc == nullptr) {
        FeatureManagerHandle manager = FeatureGetManagerHandleFromInstance(handle);
        FeatureProtoHandle ProtoHandle = FeatureGetProtoHandle(handle);
        zc = new ZipContext(FeatureGetUVLoop(manager), FeatureGetPackageName(ProtoHandle));

        if (!zc->pkg_name || strlen(zc->pkg_name) == 0) {
            FEATURE_LOG_ERROR("package name is null\n");
            zc->pkg_name = "zip_test";
        }
        FEATURE_LOG_DEBUG("pkg name = %s \n", zc->pkg_name);
        FeatureSetObjectData(handle, zc);
    }
}

static void detach(FeatureInstanceHandle handle)
{
    ZipContext* zc = (ZipContext*)FeatureGetObjectData(handle);
    FEATURE_LOG_DEBUG("%s zc=%p", __func__, zc);

    if (zc != nullptr) {
        for (size_t i = 0; i < zc->zr_arr.size(); i++) {
            /* task not start, zc->zr_arr[i] not null, should be cancel the task when page destory */
            if (zc->zr_arr[i] != nullptr) {
                uv_cancel((uv_req_t*)(&(zc->zr_arr[i]->req)));
                zc->zr_arr[i] = nullptr;
            }
        }
        delete zc;
    }
    FeatureSetObjectData(handle, nullptr);
}

void system_zip_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
    detach(handle);
}

void system_zip_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_zip_onUnregister(const char* feature_name)
{
    FEATURE_LOG_DEBUG("%s::%s()\n", file_tag, __FUNCTION__);
}

/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDos format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
static void change_file_date(const char* filename, uLong dosdate, tm_unz tmu_date)
{
    (void)dosdate;
    struct utimbuf ut;
    struct tm newdate;
    newdate.tm_sec = tmu_date.tm_sec;
    newdate.tm_min = tmu_date.tm_min;
    newdate.tm_hour = tmu_date.tm_hour;
    newdate.tm_mday = tmu_date.tm_mday;
    newdate.tm_mon = tmu_date.tm_mon;
    if (tmu_date.tm_year > 1900)
        newdate.tm_year = tmu_date.tm_year - 1900;
    else
        newdate.tm_year = tmu_date.tm_year;
    newdate.tm_isdst = -1;

    ut.actime = ut.modtime = mktime(&newdate);
    utime(filename, &ut);
}

static int create_dir(const char* path)
{
    FEATURE_LOG_DEBUG("%s:, path = %s", __FUNCTION__, path);
    char data[CONFIG_PATH_MAX] = { 0 };
    char* ret;

    if ((strcmp(path, ".") == 0) || (strcmp(path, "/") == 0))
        return 0;

    if (access(path, F_OK) == 0) {
        return 0;
    } else {
        sprintf(data, "%s", path);
        if (strlen(data) > 0 && data[strlen(data) - 1] == '/')
            data[strlen(data) - 1] = '\0';
        ret = strrchr(data, '/');
        if (ret == 0) {
            return 0;
        }
        *ret = 0;
        create_dir(data);
    }

    if (mkdir(path, 0777) != 0) {
        FEATURE_LOG_ERROR("mkdir failed, path:%s,%d\n", path, errno);
    }
    return 1;
}

static int do_extract_currentfile(unzFile uf, const char* password)
{
    char filename_inzip[256];
    char* filename_withoutpath;
    char* p;
    int err = UNZ_OK;
    FILE* fout = NULL;
    void* buf;
    uInt size_buf;
    unz_file_info64 file_info;

    /* unzGetCurrentFileInfo64: get detailed information about the current file in the compressed file, including file name, size before and after compression, timestamp, etc. */
    err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);

    if (err != UNZ_OK) {
        FEATURE_LOG_ERROR("error %d with zipfile in unzGetCurrentFileInfo\n", err);
        return err;
    }

    size_buf = WRITEBUFFERSIZE;
    buf = (void*)malloc(size_buf);
    if (buf == NULL) {
        FEATURE_LOG_ERROR("Error allocating memory\n");
        return UNZ_INTERNALERROR;
    }

    p = filename_withoutpath = filename_inzip;
    while ((*p) != '\0') {
        if (((*p) == '/') || ((*p) == '\\'))
            /* Record which folders are in the zip file */
            filename_withoutpath = p + 1;
        p++;
    }

    if ((*filename_withoutpath) == '\0') {
        FEATURE_LOG_DEBUG("creating directory: %s\n", filename_inzip);
        mkdir(filename_inzip, 0755);
    } else {
        const char* write_filename;
        write_filename = filename_inzip;

        err = unzOpenCurrentFilePassword(uf, password);
        if (err != UNZ_OK) {
            FEATURE_LOG_ERROR("error %d with zipfile in unzOpenCurrentFilePassword\n", err);
        }

        if (err == UNZ_OK) {
            fout = FOPEN_FUNC(write_filename, "wb");
            if (fout == NULL) {
                FEATURE_LOG_ERROR("error opening %s\n", write_filename);
            }
            /* some zipfile don't contain directory alone file */
            if ((fout == NULL) && (filename_withoutpath != (char*)filename_inzip)) {
                char c = *(filename_withoutpath - 1);
                *(filename_withoutpath - 1) = '\0';
                create_dir(write_filename);
                *(filename_withoutpath - 1) = c;
                fout = FOPEN_FUNC(write_filename, "wb");
            }
        }

        if (fout != NULL) {
            FEATURE_LOG_DEBUG("extracting: %s\n", write_filename);
            do {
                err = unzReadCurrentFile(uf, buf, size_buf);
                if (err < 0) {
                    FEATURE_LOG_ERROR("error %d with zipfile in unzReadCurrentFile\n", err);
                    break;
                }
                if (err > 0)
                    if (fwrite(buf, (unsigned)err, 1, fout) != 1) {
                        FEATURE_LOG_ERROR("error in writing extracted file\n");
                        err = UNZ_ERRNO;
                        break;
                    }
            } while (err > 0);
            if (fout)
                fclose(fout);
            /* Change the creation time of a file or directory : time info from zip source file */
            if (err == UNZ_OK)
                change_file_date(write_filename, file_info.dosDate, file_info.tmu_date);
        }

        if (err == UNZ_OK) {
            err = unzCloseCurrentFile(uf);
            if (err != UNZ_OK) {
                FEATURE_LOG_ERROR("error %d with zipfile in unzCloseCurrentFile\n", err);
            }
        } else
            unzCloseCurrentFile(uf);
    }
    free(buf);
    return err;
}

static int __error_code_map(int error)
{
    int code = IOERROR;
    switch (error) {
    case -2:
        code = PATH_NOT_EXISTS;
        break;
    case -22:
        code = ARGSERROR;
        break;
    }
    return code;
}

static void clearZrPtr(std::vector<zipReq*>& zr_arr, zipReq* zr)
{
    for (size_t i = 0; i < zr_arr.size(); ++i) {
        if (zr_arr[i] == zr) {
            zr_arr[i] = nullptr;
        }
    }
    return;
}

static void __extract_zip_after_work_cb(uv_work_t* req, int status)
{
    zipReq* zr = static_cast<zipReq*>(req->data);
    if (!zr)
        return;

    if (status == UV_ECANCELED) {
        FEATURE_LOG_ERROR("%s::%s: uv decompress work was canceled !!!\n", file_tag, __FUNCTION__);
        freeZipReq(zr);
        return;
    }

    FEATURE_LOG_DEBUG("%s: srcUri=%s,dstUri=%s \n", __FUNCTION__, zr->src_path, zr->dst_path);
    FeatureInstanceHandle feature = zr->handle;
    if (!FeatureInstanceIsDetached(feature)) {
        /* status is 0 means uv task success execute and unzip_success_flag is true means zip operate success. */
        if (status == 0 && zr->unzip_success_flag) {
            FEATURE_LOG_DEBUG("unzip src_path success %s", zr->src_path);
            INVOKE_SUCCESS_CB(zr->success);
        } else {
            FEATURE_LOG_ERROR("unzip src_path failed %s", zr->src_path);
            INVOKE_FAIL_CB(zr->fail, uv_strerror(status), __error_code_map(status));
        }
        INVOKE_COMPLET_CB(zr->complete);

        REMOVE_ALL_CALLBACK(zr->success, zr->fail, zr->complete);

        /* if task success finished , zr array[i] set null and free zr. */
        ZipContext* zc = getZipContext(feature);
        if (zc != nullptr) {
            clearZrPtr(zc->zr_arr, zr);
            freeZipReq(zr);
        }
    } else {
        /* work running (can not be canceled) but page has destoryed, should be free zr.*/
        freeZipReq(zr);
    }
}

static void _do_extract_zip_work_cb(uv_work_t* wk)
{
    zipReq* zr = static_cast<zipReq*>(wk->data);
    if (!zr)
        return;
    FEATURE_LOG_INFO("%s: srcUri=%s,dstUri=%s \n", __FUNCTION__, zr->src_path, zr->dst_path);
    char filename_try[MAXFILENAME + 16] = "";
    /* if Unzip encrypted zip file */
    const char* password = NULL;
    int err = -1;
    unzFile uf = NULL;
    unz_global_info64 gi;

    strncpy(filename_try, zr->src_path, MAXFILENAME - 1);
    /* strncpy not append the trailing NULL, of the string is too long. */
    filename_try[MAXFILENAME] = '\0';

    uf = unzOpen64(zr->src_path);
    if (uf == NULL) {
        strcat(filename_try, ".zip");
        uf = unzOpen64(filename_try);
    }
    if (uf == NULL) {
        FEATURE_LOG_ERROR("Cannot open %s or %s.zip\n", zr->src_path, zr->src_path);
        goto ERROR;
    }
    /* cd to dst path */
    if (chdir(zr->dst_path)) {
        FEATURE_LOG_ERROR("Error changing into %s, aborting\n", zr->dst_path);
        goto ERROR;
    }
    /* unzGetGlobalInfo64 : get the overall information of all files in the compressed file, including total number of files, size before and after compression, etc. */
    err = unzGetGlobalInfo64(uf, &gi);
    if (err != UNZ_OK) {
        FEATURE_LOG_ERROR("error %d with zipfile in unzGetGlobalInfo \n", err);
        goto ERROR;
    }
    /* gi.number_entry : the total number of files in the compressed file */
    for (uLong i = 0; i < gi.number_entry; i++) {
        if (uv_loop_is_close(wk->loop)) {
            FEATURE_LOG_ERROR("%s:break current unzip task when uv loop closed \n", __FUNCTION__);
            break;
        }
        err = do_extract_currentfile(uf, password);
        if (err != UNZ_OK)
            break;
        if ((i + 1) < gi.number_entry) {
            /* unzGoToNextFile: traverse the files in the ZIP file, moving the file pointer to the location of the next file */
            err = unzGoToNextFile(uf);
            if (err != UNZ_OK) {
                FEATURE_LOG_ERROR("error %d with zipfile in unzGoToNextFile\n", err);
                break;
            }
        }
    }
    /* set unzip file flag is true */
    zr->unzip_success_flag = true;

ERROR:
    if (uf != NULL) {
        unzClose(uf);
    }
    return;
}

void system_zip_wrap_decompress(FeatureInstanceHandle feature, AppendData append_data, system_zip_DecompressInfo* info)
{
    if (info == NULL)
        return;
    FEATURE_LOG_INFO("%s: srcUri=%s,dstUri=%s \n", __FUNCTION__, info->srcUri, info->dstUri);

    char *src_path = NULL, *dst_path = NULL;
    const char* msg;
    int code, r;

    ZipContext* zc = getZipContext(feature);
    zipReq* zr = static_cast<zipReq*>(malloc(sizeof(*zr)));

    if (!zr) {
        FEATURE_LOG_ERROR("malloc fail");
        msg = "malloc fail";
        code = GENERAL;
        goto fail;
    }

    zr->src_path = NULL;
    zr->dst_path = NULL;
    zr->handle = NULL;

    if (!zc->loop) {
        FEATURE_LOG_ERROR("uvloop is null");
        msg = "uvloop is null";
        code = GENERAL;
        goto fail;
    }

    if (info->dstUri == NULL || info->srcUri == NULL || is_path_in_tmp(info->srcUri) || is_path_in_tmp(info->dstUri)) {
        msg = "invalid file path";
        code = ARGSERROR;
        goto fail;
    }

    zr->req.data = zr;
    zr->handle = FeatureDupInstanceHandle(feature);
    zr->success = info->success;
    zr->fail = info->fail;
    zr->complete = info->complete;

    if (*(info->srcUri) == '/') {
        src_path = (char*)malloc(CONFIG_PATH_MAX);
        memset(src_path, 0, CONFIG_PATH_MAX);
#ifdef CONFIG_QUICKAPP
        sprintf(src_path, "%s/app/%s%s", CONFIG_HAP_APP_PATH, zc->pkg_name, info->srcUri);
#else
        // CONFIG_QUICK_APP not open, as default value.
        sprintf(src_path, "data/app/%s%s", zc->pkg_name, info->srcUri);
#endif
    } else {
        /* src path convert to absolute path */
        src_path = app_relative_to_absolute_path(zc->pkg_name, info->srcUri);
    }
    /* dst path convert to absolute path */
    dst_path = app_relative_to_absolute_path(zc->pkg_name, info->dstUri);

    zr->src_path = src_path;
    zr->dst_path = dst_path;

    if (src_path == NULL || dst_path == NULL) {
        FEATURE_LOG_ERROR("invalid file path: %s, %s", info->srcUri, info->dstUri);
        msg = "invalid file path";
        code = ARGSERROR;
        goto fail;
    }

    FEATURE_LOG_DEBUG("%s: src_path = %s, dst_path = %s", __FUNCTION__, src_path, dst_path);
    /* if zip file source dir not exsit */
    if (access(src_path, F_OK) == -1) {
        FEATURE_LOG_ERROR("zip source file Path does not exist or is inaccessible! src_path = %s\n", src_path);
        msg = "src path invalid!";
        code = PATH_NOT_EXISTS;
        goto fail;
    }

    /* if decompress zip file output dir not exsit, need create it */
    if (access(dst_path, F_OK) == -1) {
        FEATURE_LOG_DEBUG("decompress output dir does not exist, need to create it \n");
        if (create_dir(dst_path) == -1) {
            msg = "create dst path failed!";
            code = IOERROR;
            goto fail;
        }
    }

    /* maybe decompress API called multiple times in a page, need a array save different zr in a ZipContext (instance) */
    zc->zr_arr.push_back(zr);

    r = uv_queue_work(zc->loop, &zr->req, _do_extract_zip_work_cb,
        __extract_zip_after_work_cb);
    if (r != 0) {
        FEATURE_LOG_ERROR("execute uv_queue_work fail");
    }
    return;
fail:
    INVOKE_FAIL_CB(info->fail, msg, code);
    INVOKE_COMPLET_CB(info->complete);
    REMOVE_ALL_CALLBACK(info->success, info->fail, info->complete);
    freeZipReq(zr);
}
