#ifndef JIDL_UTIL_H
#define JIDL_UTIL_H

#include "feature_exports.h"

#include <math.h>
#include <string>

template <typename T>
class FTArrayHelper {
private:
    FtArray* _data;

public:
    FTArrayHelper(FtArray* data)
    {
        _data = data;
    }

    ~FTArrayHelper()
    {
    }

    T& operator[](int32_t index)
    {
        return ((T*)_data->_element)[index];
    }

    int32_t size() const { return _data->_size; }
};

class JidlUtils {
public:
    static int almostEqualDouble(double a, double b, double epsilon);
    // 判断两个 float 是否几乎相等
    static int almostEqualFloat(float a, float b, float epsilon);
    static char* stringToFtString(std::string str);
};

#endif
