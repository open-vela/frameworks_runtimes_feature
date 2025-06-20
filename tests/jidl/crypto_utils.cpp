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

#include "crypto_utils.h"
#include "feature_log.h"

#include <alloca.h>
#include <stdio.h>

static const char* file_tag = "[crypto_util] ";

bool has_type(const char** type_array, int size, const char* type)
{
    for (int i = 0; i < size; ++i) {
        if (strcmp(type, type_array[i]) == 0)
            return true;
    }
    return false;
}

char** split_str(const char* input, const char* delimiter, int* count)
{
    char* in_cpy = strdup(input);

    *count = 1;
    for (int i = 0; in_cpy[i] != '\0'; ++i) {
        if (in_cpy[i] == delimiter[0]) {
            (*count)++;
        }
    }

    char* savedptr = NULL;
    char** result = (char**)malloc((*count) * sizeof(char*));
    char* token = strtok_r(in_cpy, delimiter, &savedptr);
    int index = 0;

    while (token != NULL) {
        result[index] = strdup(token);
        token = strtok_r(NULL, delimiter, &savedptr);
        index++;
    }

    free(in_cpy);
    return result;
}

void free_str_array(char** str_array, int count)
{
    for (int i = 0; i < count; ++i) {
        free(str_array[i]);
    }
    free(str_array);
}
