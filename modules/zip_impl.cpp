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
#include "unzip.h"
#include "zip.h"
#include <utime.h>

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
        FeatureRemoveCallback(feature, cb);                        \
    } while (0)

#define INVOKE_FAIL_CB(cb, msg, code)                           \
    do {                                                        \
        if (!FeatureInvokeCallback(feature, cb, msg, code)) {   \
            FEATURE_LOG_ERROR("invoke fail callback failed !"); \
        }                                                       \
        FeatureRemoveCallback(feature, cb);                     \
    } while (0)

#define INVOKE_COMPLET_CB(cb)                                       \
    do {                                                            \
        if (!FeatureInvokeCallback(feature, cb)) {                  \
            FEATURE_LOG_ERROR("invoke complete callback failed !"); \
        }                                                           \
        FeatureRemoveCallback(feature, cb);                         \
    } while (0)

typedef struct {
    const char* pkg_name;
} ZipContext;

ZipContext* getZipContext(FeatureInstanceHandle handle)
{
    void* user_data = FeatureGetProtoData(FeatureGetProtoHandle(handle));
    assert(user_data != nullptr);
    return static_cast<ZipContext*>(user_data);
}

void system_zip_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_zip_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    ZipContext* zc = (ZipContext*)FeatureGetProtoData(handle);
    if (zc == nullptr) {
        zc = static_cast<ZipContext*>(malloc(sizeof(ZipContext)));
        zc->pkg_name = FeatureGetPackageName(handle);
        if (!zc->pkg_name || strlen(zc->pkg_name) == 0) {
            FEATURE_LOG_ERROR("package name is null\n");
            zc->pkg_name = "zip_test";
        }
        FEATURE_LOG_INFO("pkg name = %s \n", zc->pkg_name);
        FeatureSetProtoData(handle, zc);
    }
}

void system_zip_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}
void system_zip_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
}

void system_zip_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
    ZipContext* zc = (ZipContext*)FeatureGetProtoData(handle);
    if (!zc)
        return;
    free(zc);
}

void system_zip_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s::%s()\n", file_tag, __FUNCTION__);
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
        int n = (strlen(path) < sizeof(data) - 1) ? strlen(path) : (sizeof(data) - 1);
        strncpy(data, path, n);
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

static int do_extract(char* srcPath, char* dstPath)
{
    FEATURE_LOG_DEBUG("%s need cd path is %s!\n", __FUNCTION__, dstPath);
    char filename_try[MAXFILENAME + 16] = "";
    /* if Unzip encrypted zip file */
    const char* password = NULL;
    unzFile uf = NULL;
    unz_global_info64 gi;

    strncpy(filename_try, srcPath, MAXFILENAME - 1);
    /* strncpy not append the trailing NULL, of the string is too long. */
    filename_try[MAXFILENAME] = '\0';

    uf = unzOpen64(srcPath);
    if (uf == NULL) {
        strcat(filename_try, ".zip");
        uf = unzOpen64(filename_try);
    }
    if (uf == NULL) {
        FEATURE_LOG_ERROR("Cannot open %s or %s.zip\n", srcPath, srcPath);
        return -1;
    }
    /* cd to dst path */
    if (chdir(dstPath)) {
        FEATURE_LOG_ERROR("Error changing into %s, aborting\n", dstPath);
        exit(-1);
    }
    /* unzGetGlobalInfo64 : get the overall information of all files in the compressed file, including total number of files, size before and after compression, etc. */
    int err = unzGetGlobalInfo64(uf, &gi);
    if (err != UNZ_OK) {
        FEATURE_LOG_ERROR("error %d with zipfile in unzGetGlobalInfo \n", err);
        return err;
    }
    /* gi.number_entry : the total number of files in the compressed file */
    for (uLong i = 0; i < gi.number_entry; i++) {
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

    return err;
}

void system_zip_wrap_decompress(FeatureInstanceHandle feature, union AppendData append_data, system_zip_DecompressInfo* info)
{
    if (info == NULL)
        return;
    FEATURE_LOG_INFO("[ZiP_DECOMPRESS] srcUri=%s,dstUri=%s \n", info->srcUri, info->dstUri);

    char *src_path = NULL, *dst_path = NULL, *tmp = NULL;
    const char* msg;
    int code, ret = -1;

    ZipContext* zc = getZipContext(feature);

    if (info->dstUri == NULL || info->srcUri == NULL || is_path_in_tmp(info->srcUri) || is_path_in_tmp(info->dstUri)) {
        msg = "invalid file path";
        code = ARGSERROR;
        goto fail;
    }

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

    if (src_path == NULL || dst_path == NULL) {
        FEATURE_LOG_ERROR("invalid file path: %s, %s", info->srcUri, info->dstUri);
        msg = "invalid file path";
        code = ARGSERROR;
        goto fail;
    }

    FEATURE_LOG_INFO("src_path = %s, dst_path = %s", src_path, dst_path);
    /* if zip file source dir not exsit */
    if (access(src_path, F_OK) == -1) {
        FEATURE_LOG_ERROR("zip source file Path does not exist or is inaccessible! \n");
        free(src_path);
        msg = "src path invalid!";
        code = PATH_NOT_EXISTS;
        goto fail;
    }

    /* if decompress zip file output dir not exsit, need create it */
    if (access(dst_path, F_OK) == -1) {
        tmp = strdup(dst_path);
        FEATURE_LOG_INFO("decompress output dir does not exist, need to create it \n");
        if (create_dir(dst_path) == 1) {
            FEATURE_LOG_DEBUG("decompress output dir created successfully.\n");
        } else {
            free(dst_path);
            msg = "create dst path failed!";
            code = IOERROR;
            goto fail;
        }
        FEATURE_LOG_DEBUG("tmp is %s \n", tmp);
        ret = do_extract(src_path, tmp);
        free(tmp);
    } else {
        ret = do_extract(src_path, dst_path);
    }

    if (ret == 0) {
        INVOKE_SUCCESS_CB(info->success);
        INVOKE_COMPLET_CB(info->complete);
    }
    if (src_path)
        free(src_path);
    return;
fail:
    INVOKE_FAIL_CB(info->fail, msg, code);
    INVOKE_COMPLET_CB(info->complete);
    if (src_path)
        free(src_path);
}