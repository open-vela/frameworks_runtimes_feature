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

#ifdef __NuttX__
#define ABS_PATH_PREFIX "/data/quickapp"
#else
#define ABS_PATH_PREFIX "/quickapp"
#endif

#define APP_PATH_PREFIX "internal://"
#define PATH_MAX_LENGTH CONFIG_PATH_MAX

#define arrayof(array) sizeof(array) / sizeof(array[0])

static const char* type_list[] = { "cache", "file", "mass", "tmp" };

/*app相对路径转换为绝对路径*/
char* app_relative_to_absolute_path(const char* pkg, char* relative_path)
{
    char* absolute_path = NULL;
    char *offset, *type, *filename = NULL;

    if (!relative_path || !pkg) {
        return NULL;
    }

    relative_path = strdup(relative_path);
    if (relative_path == NULL) {
        return NULL;
    }

    if (strstr(relative_path, APP_PATH_PREFIX) == NULL) {
        free(relative_path);
        return NULL;
    }
    offset = relative_path + strlen(APP_PATH_PREFIX);

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
            free(relative_path);
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

    free(relative_path);
    return absolute_path;
}

/*绝对路径转换为app相对路径*/
char* app_absolute_to_relative_path(const char* pkg, char* absolute_path)
{
    char* relative_path = NULL;
    char *offset = NULL, *type = NULL, *filename = NULL;
    int len = 0;

    if (!absolute_path) {
        goto fail;
    }

    len = strlen(absolute_path);
    absolute_path = strdup(absolute_path);
    if ((offset = strstr(absolute_path, ABS_PATH_PREFIX)) == NULL) {
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
    if (filename - absolute_path > len) {
        filename = (char*)"";
    }

    relative_path = (char*)malloc(PATH_MAX_LENGTH);
    snprintf(relative_path, PATH_MAX_LENGTH, APP_PATH_PREFIX "%s/%s", type, filename);

    free(absolute_path);
    return relative_path;

fail:
    free(absolute_path);
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

int checkpath(const char* path)
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

    token = strtok(data, s);
    while (token != NULL) {
        token = strtok(NULL, s);
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
