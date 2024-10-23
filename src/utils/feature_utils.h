#pragma once
#include "feature_common.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_instance.h"
#include "feature_types.h"
#include <new>
#include <string>
#include <type_traits>
#include <utility>

FeatureType getElementType(FtArray* arr);

namespace ft_utils {

template <typename T>
class FeatureArray;

/**
 * @brief RefPtr utils class for memory management
 *
 * @tparam T
 */
template <typename T>
class RefPtr {
protected:
    T* p_;

    explicit RefPtr(T* ptr)
        : p_(ptr)
    {
    }

private:
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

    template <typename TElement>
    FeatureArray<TElement> getArray() const
    {
        return FeatureArray<TElement>::dup(p_);
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

    const T* ptr() const
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

    T& operator*()
    {
        return *p_;
    }

    const T* operator->() const
    {
        return p_;
    }

    RefPtr<T>& operator=(const RefPtr<T>& other)
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

using FtStringPtr = ft_utils::RefPtr<char>;

template <typename T>
class ArrayIterator {
private:
    int _pos;
    const class FeatureArray<T>* _p_vec;

public:
    ArrayIterator(const FeatureArray<T>* p_vec, int pos)
        : _pos(pos)
        , _p_vec(p_vec)
    {
    }

    bool operator!=(const ArrayIterator<T>& other) const
    {
        return _pos != other._pos;
    }

    T operator*() const
    {
        return _p_vec->at(_pos);
    }

    const ArrayIterator<T>& operator++()
    {
        ++_pos;
        return *this;
    }
};

template <typename T>
class FeatureArray : public RefPtr<FtArray> {
private:
    FeatureArray(FtArray* ptr)
        : RefPtr(ptr)
    {
    }

public:
    ~FeatureArray() = default;

    static FeatureArray adopt(FtArray* ptr)
    {
        return FeatureArray(ptr);
    }

    static FeatureArray dup(FtArray* ptr)
    {
        return FeatureArray(static_cast<FtArray*>(FeatureInstanceDupValue(ptr)));
    }

    FeatureArray(const FeatureArray& other)
    {
        p_ = FeatureInstanceDupValue(other.p_);
    }

    FeatureArray(const FeatureArray&& other)
    {
        p_ = other.p_;
        other.p_ = nullptr;
    }

    RefPtr<FtArray> getShared()
    {
        return RefPtr<FtArray>::dup(p_);
    }

public:
    inline int32_t size() const
    {
        return p_->_size;
    }

    inline int32_t capacity() const
    {
        return p_->_capacity;
    }

    inline T* data() const
    {
        return static_cast<T*>(p_->_element);
    }

    ArrayIterator<T> begin() const
    {
        return ArrayIterator<T>(this, 0);
    }

    ArrayIterator<T> end() const
    {
        return ArrayIterator<T>(this, size());
    }

    inline T& at(size_t index)
    {
        FeatureType element_type = getElementType(p_);
        T* elem = (T*)((uintptr_t)p_->_element + index * getValueSize(element_type));
        return *elem;
    }

    inline T at(size_t index) const
    {
        FeatureType element_type = getElementType(p_);
        T* elem = (T*)((uintptr_t)p_->_element + index * getValueSize(element_type));
        return *elem;
    }

    inline T& operator[](size_t index)
    {
        return at(index);
    }

    inline bool resize(size_t new_size)
    {
        return FeatureArrayResize(p_, new_size);
    }

    inline bool append(const T data)
    {
        if constexpr (std::is_pointer_v<T>) {
            return FeatureArrayAppend(p_, data);
        } else {
            return FeatureArrayAppend(p_, &data);
        }
    }

    // for RefPtr<T> specialization
    inline bool append(const RefPtr<std::remove_cv_t<std::remove_pointer_t<T>>>& data)
    {
        return FeatureArrayAppend(p_, data.ptr());
    }

    inline bool appendRaw(const T data)
    {
        if constexpr (std::is_pointer_v<T>) {
            return FeatureArrayAppendRaw(p_, data);
        } else {
            return FeatureArrayAppendRaw(p_, &data);
        }
    }

    inline int clear()
    {
        return FeatureArrayClear(p_);
    }

    inline int erase(int start, size_t count)
    {
        return FeatureArrayRemove(p_, start, count);
    }

    inline int insertAfter(int start, const void* data, size_t count)
    {
        return FeatureArrayInsertAfter(p_, start, data, count);
    }

    inline int insertRawAfter(int start, const void* data, size_t count)
    {
        return FeatureArrayInsertRawAfter(p_, start, data, count);
    }

    inline int insertBefore(int start, const void* data, size_t count)
    {
        return FeatureArrayInsertBefore(p_, start, data, count);
    }

    inline int insertRawBefore(int start, const void* data, size_t count)
    {
        return FeatureArrayInsertRawBefore(p_, start, data, count);
    }
};

inline namespace internal {
    template <typename T>
    struct has_member_getFeatureType {
        template <typename U>
        constexpr static auto check(const void*) -> decltype(std::declval<U>().getFeatureType(), std::true_type());

        template <typename U>
        constexpr static std::false_type check(...);

        static constexpr bool value = decltype(check<T>(nullptr))::value;
    };

    template <typename T, class... Args>
    RefPtr<T> make(FeatureInstanceHandle handle, Args... args)
    {
        if constexpr (has_member_getFeatureType<T>::value) {
            T* ret = new (FeatureInstanceAllocType(handle, sizeof(T), T::getFeatureType())) T(std::forward<Args>(args)...);
            return RefPtr<T>::adopt(ret);
        } else {
            T* ret = new (FeatureInstanceAlloc(handle, sizeof(T))) T(std::forward<Args>(args)...);
            return RefPtr<T>::adopt(ret);
        }
    }

    /**
     * @brief Get the Feature Type
     *
     * @tparam T
     * @return FeatureType
     */
    template <typename T>
    inline FeatureType getFeatureType()
    {
        FeatureType result = -1;
        using Ty = std::remove_cv_t<T>;
        if constexpr (has_member_getFeatureType<std::remove_pointer_t<Ty>>::value) {
            // for complex type
            result = std::remove_pointer_t<Ty>::getFeatureType();
        } else if constexpr (std::is_same_v<Ty, int>) {
            result = FT_INT;
        } else if constexpr (std::is_same_v<Ty, int8_t>) {
            result = FT_INT8;
        } else if constexpr (std::is_same_v<Ty, uint8_t>) {
            result = FT_UINT8;
        } else if constexpr (std::is_same_v<Ty, int16_t>) {
            result = FT_INT16;
        } else if constexpr (std::is_same_v<Ty, uint16_t>) {
            result = FT_UINT16;
        } else if constexpr (std::is_same_v<Ty, int32_t>) {
            result = FT_INT32;
        } else if constexpr (std::is_same_v<Ty, uint32_t>) {
            result = FT_UINT32;
        } else if constexpr (std::is_same_v<Ty, int64_t>) {
            result = FT_INT64;
        } else if constexpr (std::is_same_v<Ty, uint64_t>) {
            result = FT_UINT64;
        } else if constexpr (std::is_same_v<Ty, float>) {
            result = FT_FLOAT;
        } else if constexpr (std::is_same_v<Ty, double>) {
            result = FT_DOUBLE;
        } else if constexpr (std::is_same_v<Ty, bool>) {
            result = FT_BOOLEAN;
        } else if constexpr (std::is_same_v<Ty, char*> || std::is_same_v<Ty, const char*> || std::is_same_v<Ty, std::string>) {
            result = FT_STRING;
        }
        return result;
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
    explicit FeatureInstance(FeatureInstanceHandle hInstance)
        : _hInst(hInstance)
    {
    }

    virtual ~FeatureInstance() = default;

    /**
     * @brief get FeatureInstance handle
     *
     * @return FeatureInstanceHandle
     */
    inline FeatureInstanceHandle getHandle() const { return _hInst; }

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
        return internal::make<T>(getHandle(), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T* makeInterface(Args... args)
    {
        FeatureInterfaceHandle handle = FeatureCreateInterface(getHandle(), FEATURE_INSTANCE_CPP_VTABLE);
        T* pClass = new T(handle, std::forward<Args>(args)...);
        FeatureSetObjectData(handle, pClass);
        return pClass;
    }

    inline RefPtr<char> strdup(const char* str)
    {
        return FtStringPtr::adopt(FeatureStrCopy(getHandle(), str));
    }

    template <typename T>
    inline FeatureArray<T> makeArray(size_t capacity)
    {
        FeatureType featureType = getFeatureType<T>();
        if (featureType == -1) {
            FEATURE_LOG_ERROR("get element type failed !");
            return FeatureArray<T>::adopt(nullptr);
        }
        return FeatureArray<T>::adopt(FeatureCreateArray(getHandle(), capacity, featureType));
    }

    template <typename T>
    inline FeatureArray<T> makeArray(T* data, size_t count)
    {
        FeatureType featureType = getFeatureType<T>();
        if (featureType == -1) {
            FEATURE_LOG_ERROR("get element type failed !");
            return FeatureArray<T>::adopt(nullptr);
        }
        return FeatureArray<T>::adopt(FeatureArrayCopy(getHandle(), featureType, data, count));
    }

    template <typename T>
    inline FeatureArray<T> makeArrayRaw(T* data, size_t count)
    {
        FeatureType featureType = getFeatureType<T>();
        if (featureType == -1) {
            FEATURE_LOG_ERROR("get element type failed !");
            return FeatureArray<T>::adopt(nullptr);
        }
        return FeatureArray<T>::adopt(FeatureArrayCopyRaw(getHandle(), featureType, data, count));
    }
};

}
