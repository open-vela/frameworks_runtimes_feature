#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "feature_utils.h"
#include <cstring>
#include <iostream>
#define countof(ptr) (sizeof(ptr) / sizeof(ptr[0]))

int main(int argc, char** argv)
{

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
    FtArray* copyArr = FeaturenArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
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
    FtArray* cArr = FeaturenArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
    std::cout << "you clear " << FeatureArrayClear(cArr) << " elements!" << std::endl;
    for (int i = 0; i < cArr->_size; ++i) {
        std::cout << "cArr[" << i << "]" << ((char**)cArr->_element)[i] << std::endl;
    }
    FeatureFreeValue(cArr);
    std::cout << "FeatureArrayClear test end-------------------" << std::endl;

    std::cout << "FeatureArrayAppend test begin-------------------" << std::endl;
    FtArray* appendArr = FeaturenArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
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
    FtArray* arr = FeaturenArrayCopyRaw(chandle, FT_STRING, cstr, countof(cstr));
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

    return 0;
}