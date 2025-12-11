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

#include "app_path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <unistd.h>

#include "feature_exports.h"
#include "feature_log.h"

#ifdef CONFIG_HAP_APP_PATH
#define ABS_PATH_PREFIX CONFIG_HAP_APP_PATH
#else
#define ABS_PATH_PREFIX "/data/quickapp"
#endif

#define APP_PATH_PREFIX "internal://"
#define PATH_MAX_LENGTH CONFIG_PATH_MAX

#define arrayof(array) sizeof(array) / sizeof(array[0])

static const char* type_list[] = { "cache", "file", "mass", "tmp" };

/*app相对路径转换为绝对路径*/
char* app_relative_to_absolute_path(const char* pkg, const char* relative_path)
{
    char* absolute_path = NULL;
    char* rel_path = NULL;
    char *offset, *type, *filename = NULL;

    if (!relative_path || !pkg) {
        return NULL;
    }

    rel_path = strdup(relative_path);
    if (strstr(rel_path, APP_PATH_PREFIX) == NULL) {
        free(rel_path);
        return NULL;
    }
    offset = rel_path + strlen(APP_PATH_PREFIX);

    type = strchr(offset, '/');
    if (type != NULL) {
        *type = '\0';
        filename = ++type;
    }

    type = offset;
    for (unsigned long i = 0; i < arrayof(type_list); i++) {
        if (!strncmp(type_list[i], type, strlen(type_list[i]))) {
            break;
        }

        if (i == arrayof(type_list) - 1) {
            free(rel_path);
            return NULL;
        }
    }

    absolute_path = (char*)malloc(PATH_MAX_LENGTH);
    memset(absolute_path, 0, PATH_MAX_LENGTH);
#ifndef __NuttX__
    getcwd(absolute_path, PATH_MAX_LENGTH);
#endif
    offset = absolute_path + strlen(absolute_path);

    if (!strcmp(type, type_list[3])) {
        snprintf(offset, PATH_MAX_LENGTH - (offset - absolute_path), ABS_PATH_PREFIX "/%s", type);
    } else {
        snprintf(offset, PATH_MAX_LENGTH - (offset - absolute_path), ABS_PATH_PREFIX "/%s/%s", type, pkg);
    }
    if (filename != NULL) {
        strcat(offset, "/");
        strcat(offset, filename);
    }

    free(rel_path);
    return absolute_path;
}

/*绝对路径转换为app相对路径*/
char* app_absolute_to_relative_path(const char* pkg, const char* absolute_path)
{
    char* relative_path = NULL;
    char* abs_path = NULL;
    char *offset = NULL, *type = NULL, *filename = NULL;
    int len = 0;

    if (!absolute_path) {
        goto fail;
    }

    len = strlen(absolute_path);
    abs_path = strdup(absolute_path);
    if ((offset = strstr(abs_path, ABS_PATH_PREFIX)) == NULL) {
        goto fail;
    }
    offset = offset + strlen(ABS_PATH_PREFIX);

    type = strchr(offset, '/');
    if (!type) {
        goto fail;
    }
    *type++ = '\0';

    offset = strstr(type, pkg);
    if (offset == NULL) {
        goto fail;
    }

    *(offset - 1) = '\0';
    filename = offset + strlen(pkg) + 1;
    if (filename - abs_path > len) {
        filename = (char*)"";
    }

    relative_path = (char*)malloc(PATH_MAX_LENGTH);
    snprintf(relative_path, PATH_MAX_LENGTH, APP_PATH_PREFIX "%s/%s", type, filename);

    free(abs_path);
    return relative_path;

fail:
    free(abs_path);
    return NULL;
}

/*根据文件名，生成相对路径*/
char* app_relative_path_generator(const char* pkg, const char* type, const char* filename)
{
    char* relative_path = NULL;
    relative_path = (char*)malloc(PATH_MAX_LENGTH);
    snprintf(relative_path, PATH_MAX_LENGTH, APP_PATH_PREFIX "%s/%s/%s", type, pkg, filename);

    return relative_path;
}

char* app_absolute_path_generator(const char* pkg, const char* type, const char* filename)
{
    char *absolute_path = NULL, *offset;
    absolute_path = (char*)malloc(PATH_MAX_LENGTH);
    memset(absolute_path, 0, PATH_MAX_LENGTH);
#ifndef __NuttX__
    snprintf(absolute_path, PATH_MAX_LENGTH, ".");
#endif
    offset = absolute_path + strlen(absolute_path);

    snprintf(offset, PATH_MAX_LENGTH - (offset - absolute_path), ABS_PATH_PREFIX "/%s/%s/%s", type, pkg, filename);

    return absolute_path;
}

int app_check_path(const char* path)
{
    const char s[] = "/";
    char* data;
    char *token, *ret;
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

    char* savedptr = NULL;
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

#ifndef CONFIG_QUICKAPP_DISK_RESERVED
#define CONFIG_QUICKAPP_DISK_RESERVED (-1LL)
#endif

bool check_disk_limit(void)
{
#if CONFIG_QUICKAPP_DISK_RESERVED > 0
    uint64_t freeDisk;
    struct statfs diskInfo;

    int res = statfs("/data", &diskInfo);
    if (res != 0) {
        return false;
    }

    freeDisk = diskInfo.f_bfree * diskInfo.f_bsize;
    if (freeDisk < CONFIG_QUICKAPP_DISK_RESERVED) {
        return false;
    }

#endif
    return true;
}

void notify_disk_space_insufficient(FeatureInstanceHandle feature, const char* pkg)
{
#if 0  // TODO Cannot notify the application, remove this function, use uorb or a global topic to notify the message
#if CONFIG_QUICKAPP_DISK_RESERVED > 0
    IApplication* app = static_cast<IApplication*>(FeatureInstanceGetManagerUserData(feature, "app"));
    app->notifyEvent(FRMEVT_APP_ENOSPC);
    FEATURE_LOG_WARN("[%s] No space left on device", pkg);
#endif
#endif
}

bool is_path_in_tmp(const char* path)
{
    if (!path || strstr(path, APP_PATH_PREFIX) == NULL) {
        return false;
    }

    const char* type = path + strlen(APP_PATH_PREFIX);
    if (strncmp(type_list[3], type, strlen(type_list[3])) == 0) {
        return true;
    }
    return false;
}
