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

#ifndef __FEATURE_PROTOTYPE_QJS_H__
#define __FEATURE_PROTOTYPE_QJS_H__

#include "feature_prototype.h"
#include "feature_utils.h"

namespace feature_framework {

class FeaturePrototypeQjs : public FeaturePrototype {
public:
    FeaturePrototypeQjs(const FeatureDescription* description);

    virtual ~FeaturePrototypeQjs();

    virtual FeatureInstance* createInterface(VTable* vtable);

    virtual FeaturePrototype* createInterfacePrototype(const FeatureDescription* description);

    ft_value_t& ft_proto() { return ft_proto_; }

    weakref_list_node& weak_ref_list() { return weak_ref_list_; }

    int inc_ref_count() { return ++weak_ref_count_; }

    int dec_ref_count() { return --weak_ref_count_; }

private:
    ft_value_t ft_proto_;
    weakref_list_node weak_ref_list_;
    int weak_ref_count_ = 0;
};

}
#endif // __FEATURE_PROTOTYPE_QJS_H__
