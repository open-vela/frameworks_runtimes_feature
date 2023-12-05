#ifndef FEATURE_COMMON_H
#define FEATURE_COMMON_H

#include "feature_description.h"
#include "feature_utils.h"

#include <cassert>
#include <cstdint>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define FEATURE_DISABLE_COPY(cls) \
    cls(const cls&) = delete;     \
    cls& operator=(const cls&) = delete

#define FEATURE_DISABLE_MOVE(cls) \
    cls(cls&&) = delete;          \
    cls& operator=(cls&&) = delete

#define FEATURE_DISABLE_COPYMOVE(cls) \
    FEATURE_DISABLE_COPY(cls);        \
    FEATURE_DISABLE_MOVE(cls)

#define TRY_GET_REAL_TYPE(featureType) \
    if (FT_IS_COMPLEX(featureType)) { \
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType); \
        if (complexType->type == COMPLEX_OPTIONAL) { \
            featureType = ((OptionalType*)complexType)->type; \
        } \
    }

#define IS_INTERFACE_TYPE(featureType, ret) \
    if (FT_IS_COMPLEX(featureType)) { \
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType); \
        ret = complexType->type == COMPLEX_INTERFACE; \
    } else { \
        ret = false; \
    }

int getParamCount(const FeatureType* param, bool* hasRest = NULL, int* optional_size = NULL);

int getValueSize(FeatureType featureType);

int countMember(ObjectMember* member);

#endif // FEATURE_COMMON_H
