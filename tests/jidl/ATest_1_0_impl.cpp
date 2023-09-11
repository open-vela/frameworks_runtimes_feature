#include "ATest_1_0.h"

const char *file_tag1 = "[jidl_feature] ATest_1_0_impl";
int IDX = 0;
template <typename T>
class FTArrayHelper
{
private:
    FTArray *_data;

public:
    FTArrayHelper(FTArray *data)
    {
        _data = data;
    }

    ~FTArrayHelper()
    {
        free(_data);
    }

    T &operator[](int32_t index)
    {
        return ((T *)_data->_element)[index];
    }

    int32_t size() const { return _data->_size; }
};
// FeatureCallbacks to be implemented
void ATest_1_0_onRegister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_1_0_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_1_0_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_1_0_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_1_0_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_1_0_onUnregister(FeatureRuntimeContext ctx)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

FtString ATest_1_0_wrap_test1(FeatureInstanceHandle feature, AppendData data, FtString a, FtInt b)
{
    printf("ATest_1_0_wrap_test1: %s, %d\n", a, b);
    const char *res = "hello, world!";
    return res;
}

void ATest_1_0_wrap_test2(FeatureInstanceHandle feature, AppendData data, FtInt a, FeatureCallbackId cb)
{
    printf("ATest_1_0_wrap_test2 %d,%d\n", a, cb);
    int ret = InvokeFeatureCallback(feature, cb, a, 666);
    if (ret)
    {
        printf("invoke failed !");
        return;
    }
    RemoveCallback(feature, cb);
}

void ATest_1_0_wrap_test3(FeatureInstanceHandle feature, AppendData data, FtString a, FeatureCallbackId cb)
{
    printf("ATest_1_0_wrap_test3 %s,%d\n", a, cb);
    int ret = InvokeFeatureCallback(feature, cb, a);
    if (ret)
    {
        printf("invoke failed !");
        return;
    }
    RemoveCallback(feature, cb);
}

void ATest_1_0_wrap_test4(FeatureInstanceHandle feature, AppendData data, FeaturePromiseHandle promiseHandle, FtInt a)
{
    if (a != 0)
    {
        FeaturePromiseResolve(feature, promiseHandle, a);
    }
    else
    {
        FeaturePromiseReject(feature, promiseHandle, a + 100);
    }
}

void ATest_1_0_wrap_print(FeatureInstanceHandle feature, AppendData data, ...)
{
    va_list ap;
    va_start(ap, data);
    int variadic_count = va_arg(ap, int);
    ft_context_ref ctx = GetFeatureContext(feature);

    for (int i = 0; i < variadic_count; i++)
    {
        ft_value_t *t = va_arg(ap, ft_value_t *);
        ft_value_t param = *t;
        const char *str_json = ft_to_string(ctx, param);
        printf("%s\n", str_json);
        ft_free_string(ctx, str_json);
    }
    va_end(ap);
}

void ATest_1_0_wrap_test5(FeatureInstanceHandle feature, AppendData data, FTArray &values)
{
    FTArrayHelper<int> int_array(&values);
    printf("%s::%s(), int_array size: %d\n", file_tag1, __FUNCTION__, int_array.size());
    printf("int_array = [\n");
    for (size_t i = 0; i < int_array.size(); i++)
    {
        printf(" index %lu: %d\n", i, int_array[i]);
    }
    printf("]\n");
    return;
}

FTArray *ATest_1_0_wrap_test6(FeatureInstanceHandle feature, AppendData data, FtInt a)
{
    printf("ATest_1_0_wrap_test6 pass number is %d\n", a);
    FTArray *strArray = ATest_1_0_malloc_string_array();
    strArray->_size = 2;
    strArray->_element = malloc(sizeof(char *) * 2);
    for (int i = 0; i < 2; i++)
    {
        char *str = static_cast<char *>(FTMalloc(100, FT_CHAR));
        sprintf(str, "hello%d", i);
        ((char **)strArray->_element)[i] = str;
    }
    return strArray;
}

// Property getters and setters to be implemented
FtInt ATest_1_0_get_idx(void *feature, AppendData data)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
    return IDX;
}

void ATest_1_0_set_idx(void *feature, AppendData data, FtInt idx)
{
    printf("%s::%s(),idx is %d\n", file_tag1, __FUNCTION__, idx);
    IDX = idx;
}