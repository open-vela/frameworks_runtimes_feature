#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "feature_utils.h"
#include "utils/feature_utils.h"
#include <cstring>
#include <iostream>
#define countof(ptr) (sizeof(ptr) / sizeof(ptr[0]))

class TestInstance : public ft_utils::FeatureInstance {

public:
    TestInstance(FeatureInstanceHandle hInstance)
        : ft_utils::FeatureInstance(hInstance)
    {
    }
};

int main(int argc, char** argv)
{
#if 0
    std::cout << "feature unit test begin...." << std::endl;
    FeatureInstanceHandle feature = nullptr;

    std::cout << "FeatureStrCopy test begin-------------------" << std::endl;
    const char* str = "Hello World!";
    char* copyStr = FeatureStrCopy(feature, str);
    FEATURE_LOG_INFO("copystring:%s", copyStr);
    FEATURE_CHECK_EQ(strcmp(str, copyStr), 0);
    FEATURE_CHECK_NE(str, copyStr);
    FeatureFreeValue(copyStr);
    std::cout << "FeatureStrCopy test end-------------------" << std::endl;

    std::cout << "FeatureCreateArray test begin-------------------" << std::endl;
    FeatureInstanceHandle handle = nullptr;
    FtArray* createArr = FeatureCreateArray(handle, 5, FT_INT);
    FEATURE_LOG_INFO("arr_size:%d", createArr->_size);
    FEATURE_LOG_INFO("arr_capacity:%d", createArr->_capacity);
    FeatureFreeValue(createArr);
    std::cout << "FeatureCreateArray test end-------------------" << std::endl;

    std::cout << "FeatureArrayCopyRaw test begin-------------------" << std::endl;
    const char* cstr[] = {
        "Hello World!",
        "hello world 1",
        "hello world 2",
        "hello world 3",
        "hello world 4",
    };
    FeatureInstanceHandle chandle = nullptr;
    FtArray* copyArr = FeatureArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
    FEATURE_LOG_INFO("arr_size:%d", copyArr->_size);
    FEATURE_LOG_INFO("arr_capacity:%d", copyArr->_capacity);

    for (int i = 0; i < countof(cstr); ++i) {
        FEATURE_CHECK_EQ(strcmp(((char**)copyArr->_element)[i], cstr[i]), 0);
    }

    FEATURE_CHECK_NE(cstr, copyArr->_element);
    std::cout << "FeatureArrayCopyRaw test end-------------------" << std::endl;

    std::cout << "FeatureArrayResize test begin-------------------" << std::endl;
    copyArr = FeatureArrayResize(copyArr, copyArr->_size + 8);
    std::cout << "copyArr->_capacity:" << copyArr->_capacity << std::endl;
    std::cout << "copyArr->_size:" << copyArr->_size << std::endl;
    for (int i = 0; i < copyArr->_size; ++i) {
        std::cout << "copyArr[" << i << "]" << ((char**)copyArr->_element)[i] << std::endl;
    }
    std::cout << "FeatureArrayResize test end-----------------" << std::endl;

    std::cout << "FeatureArrayGetLength test begin-------------------" << std::endl;
    std::cout << "size of copyarr:" << FeatureArrayGetLength(copyArr) << std::endl;
    std::cout << "FeatureArrayGetLength test end-------------------" << std::endl;

    std::cout << "FeatureArrayRemove test begin-------------------" << std::endl;
    std::cout << "you delete " << FeatureArrayRemove(copyArr, 0, 2) << " elements!" << std::endl;
    for (int i = 0; i < copyArr->_size; ++i) {
        std::cout << "copyarr[" << i << "]" << ((char**)copyArr->_element)[i] << std::endl;
    }

    std::cout << "you delete " << FeatureArrayRemove(copyArr, 1, 5) << " elements!" << std::endl;
    for (int i = 0; i < copyArr->_size; ++i) {
        std::cout << "copyArr[" << i << "]" << ((char**)copyArr->_element)[i] << std::endl;
    }
    FeatureFreeValue(copyArr);
    std::cout << "FeatureArrayRemove test end-------------------" << std::endl;

    std::cout << "FeatureArrayClear test begin-------------------" << std::endl;
    FtArray* cArr = FeatureArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
    std::cout << "you clear " << FeatureArrayClear(cArr) << " elements!" << std::endl;
    for (int i = 0; i < cArr->_size; ++i) {
        std::cout << "cArr[" << i << "]" << ((char**)cArr->_element)[i] << std::endl;
    }
    FeatureFreeValue(cArr);
    std::cout << "FeatureArrayClear test end-------------------" << std::endl;

    std::cout << "FeatureArrayAppend test begin-------------------" << std::endl;
    FtArray* appendArr = FeatureArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
    const char* appendStr = { "helloworldappend" };
    char* featurestr = FeatureStrCopy(nullptr, appendStr);
    appendArr = FeatureArrayAppend(appendArr, featurestr);
    FeatureFreeValue(featurestr);
    for (int i = 0; i < appendArr->_size; ++i) {
        std::cout << "carr[" << i << "]" << ((char**)appendArr->_element)[i] << std::endl;
    }
    std::cout << "FeatureArrayAppend test end-------------------" << std::endl;

    std::cout << "FeatureArrayAppendRaw test begin-------------------" << std::endl;
    const char* appendRawStr = "helloWorldAppendRaw";
    appendArr = FeatureArrayAppendRaw(appendArr, appendRawStr);
    for (int i = 0; i < appendArr->_size; ++i) {
        std::cout << "carr[" << i << "]" << ((char**)appendArr->_element)[i] << std::endl;
    }
    FeatureFreeValue(appendArr);
    std::cout << "FeatureArrayAppendRaw test end-------------------" << std::endl;

    std::cout << "FeatureArrayInsertRawAfter test begin-------------------" << std::endl;
    FtArray* arr = FeatureArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
    const char* insertStr[] = {
        "Hello World0!I am inserted!",
        "Hello World1!I am inserted!",
        "Hello World2!I am inserted!"
    };
    int successInsertAfter = FeatureArrayInsertRawAfter(arr, 1, insertStr, 2);
    for (int i = 0; i < arr->_size; ++i) {
        std::cout << "arr[" << i << "]" << ((char**)arr->_element)[i] << std::endl;
    }
    std::cout << "FeatureArrayInsertRawAfter test end-------------------" << std::endl;

    std::cout << "FeatureArrayInsertRawBefore test begin-------------------" << std::endl;
    int successInsertBefore = FeatureArrayInsertRawBefore(arr, 1, insertStr, 2);
    for (int i = 0; i < arr->_size; ++i) {
        std::cout << "arr[" << i << "]" << ((char**)arr->_element)[i] << std::endl;
    }

    FeatureFreeValue(arr);
    std::cout << "FeatureArrayInsertRawBefore test end-------------------" << std::endl;

    std::cout << "feature unit test end..." << std::endl;
#else
    std::cout << "feature unit test begin...." << std::endl;
    TestInstance* pInstance = new TestInstance(nullptr);

    std::cout << "FeatureStrCopy test begin-------------------" << std::endl;
    const char* str = "Hello World!";
    {
        auto copyStr = pInstance->strdup(str);
        FEATURE_LOG_INFO("copystring:%s", copyStr.ptr());
        FEATURE_CHECK_EQ(strcmp(str, copyStr.ptr()), 0);
        FEATURE_CHECK_NE(str, copyStr.ptr());
    }
    std::cout << "FeatureStrCopy test end-------------------" << std::endl;

    std::cout << "FeatureCreateArray test begin-------------------" << std::endl;
    {
        auto createArr = pInstance->makeArray<int>(5);
        FEATURE_LOG_INFO("arr_size:%d", createArr.size());
        FEATURE_LOG_INFO("arr_capacity:%d", createArr.capacity());
    }
    std::cout << "FeatureCreateArray test end-------------------" << std::endl;

    std::cout << "FeatureArrayCopyRaw test begin-------------------" << std::endl;
    const char* cstr[] = {
        +"Hello World!",
        "hello world 1",
        "hello world 2",
        "hello world 3",
        "hello world 4",
    };
    {
        auto copyArr = pInstance->makeArrayRaw<const char*>(cstr, countof(cstr));
        FEATURE_LOG_INFO("arr_size:%d", copyArr.size());
        FEATURE_LOG_INFO("arr_capacity:%d", copyArr.capacity());

        for (int i = 0; i < countof(cstr); ++i) {
            FEATURE_CHECK_EQ(strcmp(copyArr[i], cstr[i]), 0);
        }

        FEATURE_CHECK_NE(cstr, copyArr.data());
        std::cout << "FeatureArrayCopyRaw test end-------------------" << std::endl;

        std::cout << "FeatureArrayResize test begin-------------------" << std::endl;
        copyArr.resize(copyArr.size() + 8);
        std::cout << "copyArr->_capacity:" << copyArr.capacity() << std::endl;
        std::cout << "copyArr->_size:" << copyArr.size() << std::endl;
        for (int i = 0; i < copyArr.size(); ++i) {
            std::cout << "copyArr[" << i << "]" << copyArr[i] << std::endl;
        }
        std::cout << "FeatureArrayResize test end-----------------" << std::endl;

        std::cout << "FeatureArrayGetLength test begin-------------------" << std::endl;
        std::cout << "size of copyarr:" << copyArr.size() << std::endl;
        std::cout << "FeatureArrayGetLength test end-------------------" << std::endl;

        std::cout << "FeatureArrayRemove test begin-------------------" << std::endl;
        std::cout << "you delete " << copyArr.erase(0, 2) << " elements!" << std::endl;
        for (int i = 0; i < copyArr.size(); ++i) {
            std::cout << "copyarr[" << i << "]" << copyArr[i] << std::endl;
        }

        std::cout << "you delete " << copyArr.erase(1, 5) << " elements!" << std::endl;
        for (int i = 0; i < copyArr.size(); ++i) {
            std::cout << "copyArr[" << i << "]" << copyArr[i] << std::endl;
        }
    }
    std::cout << "FeatureArrayRemove test end-------------------" << std::endl;

    std::cout << "FeatureArrayClear test begin-------------------" << std::endl;
    {
        auto cArr = pInstance->makeArrayRaw<const char*>(cstr, countof(cstr));
        std::cout << "you clear " << cArr.clear() << " elements!" << std::endl;
        for (int i = 0; i < cArr.size(); ++i) {
            std::cout << "cArr[" << i << "]" << cArr[i] << std::endl;
        }
    }
    std::cout << "FeatureArrayClear test end-------------------" << std::endl;

    std::cout << "FeatureArrayAppend test begin-------------------" << std::endl;
    {
        auto appendArr = pInstance->makeArrayRaw<const char*>(cstr, countof(cstr));
        const char* appendStr = { "helloworldappend" };
        {
            auto featurestr = pInstance->strdup(appendStr);
            auto str = featurestr.ptr();
            appendArr.append(str);
        }
        for (int i = 0; i < appendArr.size(); ++i) {
            std::cout << "carr[" << i << "]" << appendArr[i] << std::endl;
        }
        std::cout << "FeatureArrayAppend test end-------------------" << std::endl;

        std::cout << "FeatureArrayAppendRaw test begin-------------------" << std::endl;
        const char* appendRawStr = "helloWorldAppendRaw";
        appendArr.appendRaw(appendRawStr);
        for (int i = 0; i < appendArr.size(); ++i) {
            std::cout << "carr[" << i << "]" << appendArr.at(i) << std::endl;
        }
    }
    std::cout << "FeatureArrayAppendRaw test end-------------------" << std::endl;

    std::cout << "FeatureArrayInsertRawAfter test begin-------------------" << std::endl;
    {
        auto arr = pInstance->makeArrayRaw<const char*>(cstr, countof(cstr));
        const char* insertStr[] = {
            "Hello World0!I am inserted!",
            "Hello World1!I am inserted!",
            "Hello World2!I am inserted!"
        };
        // int successInsertAfter = FeatureArrayInsertRawAfter(arr, 1, insertStr, 2);
        arr.insertRawAfter(1, insertStr, 2);
        int i = 0;
        for (const char* str : arr) {
            std::cout << "arr[" << i << "]" << str << std::endl;
            i++;
        }
        // for (int i = 0; i < arr->size(); ++i) {
        //     std::cout << "arr[" << i << "]" << arr->at(i) << std::endl;
        // }
        std::cout << "FeatureArrayInsertRawAfter test end-------------------" << std::endl;

        std::cout << "FeatureArrayInsertRawBefore test begin-------------------" << std::endl;
        int successInsertBefore = arr.insertRawBefore(1, insertStr, 2);
        i = 0;
        for (const char* str : arr) {
            std::cout << "arr[" << i << "]" << str << std::endl;
            i++;
        }
        auto sharedArray = arr.getShared();
    }
    std::cout << "FeatureArrayInsertRawBefore test end-------------------" << std::endl;
    delete pInstance;
    std::cout << "feature unit test end..." << std::endl;
#endif
    return 0;
}