#include "unit_jidl_util.h"

int JidlUtils::almostEqualDouble(double a, double b, double epsilon)
{
    // 判断绝对误差
    if (fabs(a - b) <= epsilon)
        return 1;

    // 判断相对误差
    double absA = fabs(a);
    double absB = fabs(b);
    double diff = fabs(a - b);

    return diff <= ((absA > absB ? absA : absB) * epsilon);
}
// 判断两个 float 是否几乎相等
int JidlUtils::almostEqualFloat(float a, float b, float epsilon)
{
    // 判断绝对误差
    if (fabsf(a - b) <= epsilon)
        return 1;

    // 判断相对误差
    float absA = fabsf(a);
    float absB = fabsf(b);
    float diff = fabsf(a - b);

    return diff <= ((absA > absB ? absA : absB) * epsilon);
}
char* JidlUtils::stringToFtString(std::string str)
{
    int len = str.length();
    char* ftStr = (char*)FeatureMalloc(len + 1, FT_STRING);
    strcpy(ftStr, str.c_str());
    return ftStr;
}