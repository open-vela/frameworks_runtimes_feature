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

#include "feature_manager.h"

#include "feature_context.h"
#include "feature_description.h"
#include "feature_wamr_utils.h"

#include <map>
#include <vector>

struct Member;

namespace feature_framework {

class FeatureInstance;
class FeatureManagerWamr;
class FeatureRegistry;
class FeatureUnit;

class FeatureManagerWamr : public FeatureManager {
public:
    FeatureManagerWamr(FeatureRegistry* registry);
    virtual bool init();
    virtual void uninit();
    Member* getFeatureMember(const FeatureDescription* description, int index);
    bool require(wasm_exec_env_t ctx, wasm_obj_t thiz, const char* name);
    void* wamrEnv() { return wamr_env_; }

private:
    bool registerFeature(const FeatureDescription* description);

    bool registerSymbol(void* func, const char* name, const char* sig, void* attach);

    void* wamr_env_;
    std::vector<NativeSymbol*> native_symbols_;
    std::vector<char*> native_strings_;
};

}
#endif // __FEATURE_MANAGER_WAMR_H__
