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

#include "gc_object.h"
#include "wasm_export.h"
#include "feature_context.h"
#include "feature_description.h"

#include <map>
#include <vector>

struct Member;

namespace ferry {

class FeatureInstance;
class FeatureManagerWamr;
class FeatureRegistry;
class FeatureUnit;

typedef struct WamrAttachment {
    FeatureManagerWamr* manager;
    NativeSymbol* symbol;
    const FeatureDescription* description;
    int index;
} WamrAttachment;

class FeatureManagerWamr {
public:
    FeatureManagerWamr(FeatureRegistry* registry);
    bool init();
    void release();
    Member* getFeatureMember(const FeatureDescription* description, int index);
    FeatureInstance* getFeatureInstance(wasm_obj_t obj);
    bool require(wasm_exec_env_t ctx, wasm_obj_t thiz, const char* name);

private:
    int registerFeature(const FeatureDescription* description);
    bool makeAttachment(NativeSymbol* symbol, const FeatureDescription* description, int index);

    ft_context_ref ft_ctx_;
    FeatureRegistry* registry_;
    std::vector<NativeSymbol*> nativesymbol_;
    std::map<wasm_obj_t, FeatureInstance*> feature_instance_map_;
    std::map<NativeSymbol*, WamrAttachment> symbol_attachment_map_;
};

}
#endif // __FEATURE_MANAGER_WAMR_H__

