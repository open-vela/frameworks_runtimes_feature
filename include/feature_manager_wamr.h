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
#ifndef __FEATURE_MANAGER_WAMR_H__
#define __FEATURE_MANAGER_WAMR_H__

#include "feature_context.h"
#include "wasm_export.h"
#include "gc_object.h"

#include <map>
#include <vector>
#include <vector>

namespace ferry {

class FeatureInstance;
class FeatureRegistry;
class FeatureManagerWamr;
class FeatureUnit;
struct Member;

typedef struct WarmAttachment {
    FeatureManagerWamr* manager;
    NativeSymbol* symbol;
    FeatureUnit* unit;
    int index;
} WarmAttachment;

class FeatureManagerWamr {
public:
    FeatureManagerWamr(FeatureRegistry* registry);
    void featureRelease();
    int register_wamr_module(const char* module_name);
    bool require_wamr(wasm_exec_env_t ctx, wasm_obj_t thiz, const char* name);
    Member* get_unit_member(FeatureUnit*unit, int index);
    FeatureInstance* get_feature_instance(wasm_obj_t obj);

private:
    bool make_attachment(NativeSymbol* symbol, FeatureUnit* unit, int index);
    FeatureRegistry* registry_;
    ft_context_ref ft_ctx_;
    std::vector<std::string> required_features_;
    std::map<wasm_obj_t, FeatureInstance*> wasmFeatureInstance_;
    std::map<NativeSymbol*, WarmAttachment> symbol_attachment_map_;

};

}
#endif // __FEATURE_MANAGER_WAMR_H__

