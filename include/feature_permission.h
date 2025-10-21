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

#ifndef FEATURE_PERMISSION_H
#define FEATURE_PERMISSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PERMISSION_BITMAP_SIZE 2 // 128 bits needs two 64 bits integer

#define HAS_PERMISSION(permissions, perm_id) \
    (((permissions).per_bits[(perm_id) / 64] >> ((perm_id) % 64)) & 1)

#define MARK_PERMISSION(permissions, perm_id) \
    ((permissions).per_bits[(perm_id) / 64] |= (1ULL << ((perm_id) % 64)))

#define UNMARK_PERMISSION(permissions, perm_id) \
    ((permissions).per_bits[(perm_id) / 64] &= ~(1ULL << ((perm_id) % 64)))

#define FEATURE_PERMISSION_LIST(V)                                     \
    V(HAPJS_PERMISSION_INTERNET, 0, "hapjs.permission.INTERNET")       \
    V(HAPJS_PERMISSION_LOCATION, 1, "hapjs.permission.LOCATION")       \
    V(HAPJS_PERMISSION_RECORD, 2, "hapjs.permission.RECORD")           \
    V(HAPJS_PERMISSION_DEVICE_INFO, 3, "hapjs.permission.DEVICE_INFO") \
    V(HAPJS_PERMISSION_READ_HEALTH_DATA, 4, "hapjs.permission.READ_HEALTH_DATA")

#define DEF_PERMISSION_ENUM(perm_enum, perm_id, perm_str) perm_enum = perm_id,

typedef struct FeaturePermissions {
    uint64_t per_bits[PERMISSION_BITMAP_SIZE];
} FeaturePermissions;

// max FeaturePermissionId must less than 128
typedef enum FeaturePermissionId {
    FEATURE_PERMISSION_LIST(DEF_PERMISSION_ENUM)
    FEATURE_PERMISSION_MAX
} FeaturePermissionId;

const char* FeatureGetPermissionName(FeaturePermissionId perm_enum);

FeaturePermissionId FeatureGetPermissionId(const char* permission);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_PERMISSION_H
