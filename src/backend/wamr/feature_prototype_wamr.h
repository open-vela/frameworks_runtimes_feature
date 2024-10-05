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

#ifndef __FEATURE_PROTOTYPE_WAMR_H__
#define __FEATURE_PROTOTYPE_WAMR_H__

#include "feature_prototype.h"

namespace feature_framework {

class FeaturePrototypeWamr : public FeaturePrototype {
public:
    FeaturePrototypeWamr(const FeatureDescription* description);

    virtual ~FeaturePrototypeWamr();

    virtual FeatureInstance* createInterface(VTable* vtable);

    virtual FeaturePrototype* createInterfacePrototype(const FeatureDescription* description);
};

}
#endif // __FEATURE_PROTOTYPE_WAMR_H__
