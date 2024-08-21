#pragma once
#include "feature_common.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_types.h"
#include <new>
#include <utility>

namespace ft_utils {
/**
 * @brief RefPtr utils class for memory management
 *
 * @tparam T
 */
template <typename T>
class RefPtr {
private:
    T* p_;

private:
    explicit RefPtr(T* ptr)
        : p_(ptr)
    {
    }

    void release()
    {
        FeatureInstanceFreeValue(p_);
    }

public:
    RefPtr()
        : p_(nullptr)
    {
    }

    ~RefPtr()
    {
        if (p_) {
            release();
        }
    }

    static RefPtr<T> adopt(T* ptr)
    {
        return RefPtr<T>(ptr);
    }

    static RefPtr<T> dup(T* ptr)
    {
        auto ret = RefPtr<T>(ptr);
        ret.dup();
        return ret;
    }

    RefPtr<T>& dup()
    {
        if (p_)
            FeatureInstanceDupValue(p_);
        return *this;
    }

    T* ptr()
    {
        return p_;
    }

    RefPtr(const RefPtr<T>& other)
    {
        p_ = other.p_;
        dup();
    }

    RefPtr(RefPtr&& other)
    {
        p_ = other.drop();
    }

    T* drop()
    {
        T* p = p_;
        p_ = nullptr;
        return p;
    }

    T* operator()()
    {
        return p_;
    }

    operator bool()
    {
        return p_ != nullptr;
    }

    T* operator->()
    {
        return p_;
    }

    RefPtr<T>& operator=(RefPtr<T>& other)
    {
        if (p_ == other.p_)
            return *this;
        if (p_) {
            release();
        }
        p_ = other.p_;
        dup();
        return *this;
    }

    bool operator==(const RefPtr& other)
    {
        return p_ == other.p_;
    }

    bool operator!=(const RefPtr& other)
    {
        return p_ != other.p_;
    }
};

inline namespace internal {
    template <typename T>
    struct has_member_getType {
        template <typename U>
        constexpr static auto check(const void*) -> decltype(std::declval<U>().getType(), std::true_type());

        template <typename U>
        constexpr static std::false_type check(...);

        static constexpr bool value = decltype(check<T>(nullptr))::value;
    };

    template <typename T, class... Args>
    RefPtr<T> make(FeatureInstanceHandle handle, Args... args)
    {
        if constexpr (has_member_getType<T>::value) {
            T* ret = new (FeatureInstanceAllocType(handle, sizeof(T), T::getType())) T(std::forward<Args>(args)...);
            return RefPtr<T>::adopt(ret);
        } else {
            T* ret = new (FeatureInstanceAlloc(handle, sizeof(T))) T(std::forward<Args>(args)...);
            return RefPtr<T>::adopt(ret);
        }
    }
}

template <typename T>
inline T* From(FeatureInstanceHandle hInst)
{
    return static_cast<T*>(FeatureGetObjectData(hInst));
}

class FeatureInstance {
private:
    FeatureInstanceHandle _hInst;

public:
    explicit FeatureInstance(FeatureInstanceHandle hInst)
        : _hInst(hInst)
    {
    }

    // 从句柄获取的对象

    /**
     * @brief get FeatureInstance Handle
     *
     * @return FeatureInstanceHandle
     */
    inline FeatureInstanceHandle getHandle()
    {
        return _hInst;
    }

    /**
     * @brief create FeatureType object
     *
     * @tparam T
     * @tparam Args
     * @param type
     * @param args
     * @return RefPtr<T>
     */
    template <typename T, class... Args>
    RefPtr<T> make(Args... args)
    {
        // 针对任意对象的make函数
        return internal::make<T>(_hInst, std::forward<Args>(args)...);
    }

    // 基础类型的创建可以放在基类中，具体的Feature中声明的struct的创建在子类中处理
    // inline RefPtr<char> copyStr(const char* str)
    // {
    //     return RefPtr<char>(nullptr);
    // }

    // inline RefPtr<FtArray> newArray(...)
    // {
    //     return RefPtr<FtArray>(nullptr);
    // }
};

template <typename T>
class FeatureArray {
private:
    FtArray* p_;

public:
    FeatureArray(FtArray* ptr)
        : p_(ptr)
    {
    }
    int32_t size()
    {
        return p_->_size;
    }

    T& operator[](size_t index)
    {
        FTObjHeader* pHeader = (FTObjHeader*)((uintptr_t)p_ - sizeof(FTObjHeader));
        T* elem = (T*)((uintptr_t)p_->_element + index * getValueSize(pHeader->type));
        return *elem;
    }
};

using FtStringPtr = ft_utils::RefPtr<char>;

}
