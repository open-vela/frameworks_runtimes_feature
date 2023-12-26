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

template<typename TCtx, typename TArg, int N = 16>
struct AutoArgs {
    TCtx ctx_;
    TArg args_[N];
    TArg* argv_;
    int count_;

    AutoArgs(TCtx ctx, int count) : ctx_(ctx) {
        if (count > N)
            argv_ = new TArg[count];
        else
            argv_ = args_;
        count_ = count;
        for (int i = 0; i < count_; i++)
            argv_[i] = _get_undefined_arg(ctx_);
    }

    ~AutoArgs() {
        for (int i = 0; i < count_; i++)
            _free_arg(ctx_, argv_[i]);

        if (argv_ != args_)
            delete[] argv_;
    }

    TArg& operator [] (int idx) {
        return argv_[idx];
    }

    operator TArg* () {
        return argv_;
    }
};

#endif // FEATURE_COMMON_H
