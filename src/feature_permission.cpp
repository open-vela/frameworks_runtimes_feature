/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "feature_permission.h"
#include "feature_log.h"

#include <cstdint>
#include <string.h>

#define MAX_PERMISSIONS_NUM 128

#define INIT_PERMISSION_NAME(perm_enum, perm_id, perm_str) \
    perm_str,

static const char* perm_name_array[MAX_PERMISSIONS_NUM] = {
    FEATURE_PERMISSION_LIST(INIT_PERMISSION_NAME)
};

const char* FeatureGetPermissionName(FeaturePermissionId perm_enum)
{
    return perm_name_array[perm_enum];
}

FeaturePermissionId FeatureGetPermissionId(const char* permission) {
    for (size_t i = 0; i < FEATURE_PERMISSION_MAX; i ++) {
      if (strcmp(perm_name_array[i], permission) == 0) {
        return (FeaturePermissionId)i;
      }
    }

    return FEATURE_PERMISSION_MAX;
}

