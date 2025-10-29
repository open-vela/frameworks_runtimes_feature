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

#define ALLOCA_PARAM_PTR(featureType, value)                             \
    {                                                                    \
        int type_len = getAlignedCount(featureType) * sizeof(uintptr_t); \
        value = alloca(type_len);                                        \
        memset(value, 0, type_len);                                      \
    }

int getAlignedCount(const FeatureType param);

int getParamCount(const FeatureType* param, bool* hasRest = NULL, int* optional_size = NULL, int32_t* int32_count = NULL);

int getValueSize(FeatureType featureType);

int countMember(ObjectMember* member);

bool convertOptional(OptionalType* opt, void* out);

FtCallbackId findCallbackIdByName(FeatureType ftype, void* pnative, const char* name);

void freeFtValue(ft_context_ref ft_ctx, FeatureType ftype, void* pnative);

void dupFtValue(ft_context_ref ft_ctx, FeatureType ftype, void* pnative);

class Clearable {
public:
    virtual void clear() = 0;
};

template <typename TPtr>
class AutoPtr {
public:
    using FreeFunc = void (*)(TPtr);
    AutoPtr(FreeFunc func)
        : ptr_(nullptr)
        , free_func_(func)
    {
    }

    ~AutoPtr()
    {
        if (ptr_ && free_func_)
            free_func_(ptr_);
    }

    AutoPtr& operator=(TPtr ptr)
    {
        ptr_ = ptr;
        return *this;
    }

    operator TPtr&()
    {
        return ptr_;
    }

private:
    TPtr ptr_;
    FreeFunc free_func_;
};

template <typename TCtx, typename TArg, int N = 16>
class AutoArgs {
public:
    using FreeFunc = void (*)(TCtx, TArg&);
    using UndefFunc = TArg (*)(TCtx);

    AutoArgs(TCtx ctx, FreeFunc free_func, UndefFunc undef_func, int count, int head = 0, int tail = 0)
        : ctx_(ctx)
        , free_func_(free_func)
        , undef_func_(undef_func)
        , count_(count)
        , head_(head)
        , tail_(tail > 0 ? tail : count_)
    {
        if (count_ > N) {
            argv_ = new TArg[count_];
            memset(argv_, 0, sizeof(TArg) * count_);
        } else {
            argv_ = args_;
            memset(args_, 0, sizeof(args_));
        }

        if (undef_func_) {
            for (int i = head_; i < tail_; i++)
                argv_[i] = undef_func_(ctx_);
        }
    }

    ~AutoArgs()
    {
        if (free_func_) {
            for (int i = head_; i < tail_; i++) {
                free_func_(ctx_, argv_[i]);
            }
        }

        if (argv_ != args_)
            delete[] argv_;
    }

    TArg& operator[](int idx)
    {
        return argv_[idx];
    }

    operator TArg*()
    {
        return argv_;
    }

private:
    TCtx ctx_;
    FreeFunc free_func_;
    UndefFunc undef_func_;
    TArg args_[N];
    TArg* argv_;
    int count_;
    int head_;
    int tail_;
};

#endif // FEATURE_COMMON_H
