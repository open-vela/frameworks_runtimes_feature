#include "ATest_1_0.h"

const char* file_tag1 = "[jidl_feature] ATest_impl";
int IDX = 0;
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

// FeatureCallbacks to be implemented
void ATest_onRegister(const char* feature_name)
{
    printf("%s::%s(), feature_name: %s\n", file_tag1, __FUNCTION__, feature_name);
}

void ATest_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
}

void ATest_onUnregister(const char* feature_name)
{
    printf("%s::%s(), feature_name: %s\n", file_tag1, __FUNCTION__, feature_name);
}

FtString ATest_wrap_test1(FeatureInstanceHandle feature, AppendData data, FtString a, FtInt b)
{
    printf("ATest_wrap_test1: %s, %d\n", a, b);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "hello, world: %s, %d", a, b);
    return buf;
}

void ATest_wrap_test2(FeatureInstanceHandle feature, AppendData data, FtInt a, FtCallbackId cb)
{
    printf("ATest_wrap_test2 %d,%d\n", a, cb);
    if (!FeatureInvokeCallback(feature, cb, a, 666)) {
        printf("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void ATest_wrap_test3(FeatureInstanceHandle feature, AppendData data, FtString a, FtCallbackId cb)
{
    printf("ATest_wrap_test3 %s,%d\n", a, cb);
    if (!FeatureInvokeCallback(feature, cb, a)) {
        printf("invoke failed !");
        return;
    }
    FeatureRemoveCallback(feature, cb);
}

void ATest_wrap_test4(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt a)
{
    if (a != 0) {
        FeaturePromiseResolve(feature, pid, a);
    } else {
        FeaturePromiseReject(feature, pid, a + 100);
    }
}

void ATest_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    for (int i = 0; i < vari_params.vari_count; i++) {
        ft_value_t param = vari_params.vari_args[i];
        ft_type param_type = ft_get_type(ft_ctx, param);
        if (param_type == FT_TYPE_OBJECT) {
            const char* param_obj = ft_to_string(ft_ctx, param);
            printf("%s ", param_obj);
            ft_free_string(ft_ctx, param_obj);
        } else if (param_type == FT_TYPE_ARRAY) {
            uint32_t array_size = ft_array_size(ft_ctx, param);
            printf("[");
            for (int i = 0; i < array_size; ++i) {
                ft_value_t elem = ft_array_at(ft_ctx, param, i);
                ft_type elem_type = ft_get_type(ft_ctx, elem);
                if (elem_type == FT_TYPE_NUMBER) {
                    double param_num;
                    if (ft_to_double(ft_ctx, elem, &param_num))
                        printf("%lf ", param_num);
                } else if (elem_type == FT_TYPE_STRING) {
                    const char* param_str = ft_to_string(ft_ctx, elem);
                    printf("%s ", param_str);
                    ft_free_string(ft_ctx, param_str);
                } else if (elem_type == FT_TYPE_BOOL) {
                    bool param_bool;
                    bool ret = ft_to_bool(ft_ctx, param, &param_bool);
                    printf("%d ", param_bool);
                } else {
                    printf("invalid array element type!");
                    return;
                }
            }
            printf("] ");
        } else if (param_type == FT_TYPE_STRING) {
            const char* param_str = ft_to_string(ft_ctx, param);
            printf("%s ", param_str);
            ft_free_string(ft_ctx, param_str);
        } else if (param_type == FT_TYPE_NUMBER) {
            double param_num;
            bool ret = ft_to_double(ft_ctx, param, &param_num);
            printf("%lf ", param_num);
        } else if (param_type == FT_TYPE_BOOL) {
            bool param_bool;
            bool ret = ft_to_bool(ft_ctx, param, &param_bool);
            printf("%d ", param_bool);
        } else {
            printf("invalid param type!");
            return;
        }
    }
    printf("\n");
}

void ATest_wrap_test5(FeatureInstanceHandle feature, AppendData data, FtArray& values)
{
    FTArrayHelper<int> int_array(&values);
    printf("%s::%s(), int_array size: %d\n", file_tag1, __FUNCTION__, int_array.size());
    printf("int_array = [\n");
    for (size_t i = 0; i < int_array.size(); i++) {
        printf(" index %lu: %d\n", i, int_array[i]);
    }
    printf("]\n");
    return;
}

FtArray* ATest_wrap_test6(FeatureInstanceHandle feature, AppendData data, FtInt a)
{
    printf("ATest_wrap_test6 pass number is %d\n", a);
    FtArray* strArray = ATest_malloc_string_array();
    strArray->_size = 2;
    strArray->_element = malloc(sizeof(char*) * 2);
    for (int i = 0; i < 2; i++) {
        char* str = (char*)FeatureMalloc(100, FT_STRING);
        sprintf(str, "hello%d", i);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

void ATest_wrap_test7(FeatureInstanceHandle feature, AppendData data, FtInt a, ATest_Person* b)
{
    printf("ATest_wrap_test7 %d\n", a);
    if (!b) {
        printf("%s::%s(), Person ptr is null!\n", file_tag1, __FUNCTION__);
        return;
    }
    if (!b->_name) {
        printf("%s::%s(), Person name is null!\n", file_tag1, __FUNCTION__);
    } else {
        printf("%s::%s(), Person: [name: %s]\n", file_tag1, __FUNCTION__, b->_name);
    }
    if (!b->_gender) {
        printf("%s::%s(), Person gender is null!\n", file_tag1, __FUNCTION__);
    } else {
        printf("%s::%s(), Person: [gender: %s]\n", file_tag1, __FUNCTION__, b->_gender);
    }
    if (!b->_age) {
        printf("%s::%s(), Person age is null!\n", file_tag1, __FUNCTION__);
    } else {
        printf("%s::%s(), Person: [age: %d]\n", file_tag1, __FUNCTION__, b->_age);
    }
}

ATest_Person* ATest_wrap_test8(FeatureInstanceHandle feature, AppendData data, FtInt a)
{
    printf("%s::%s(), a: %d\n", file_tag1, __FUNCTION__, a);
    ATest_Person* per = mallocPerson();
    char* name = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(name, "%s", "level");
    per->_name = name;
    char* gender = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(gender, "%s", "male");
    per->_gender = gender;
    per->_age = a;
    return per;
}

// Property getters and setters to be implemented
FtInt ATest_get_idx(void* feature, AppendData data)
{
    printf("%s::%s()\n", file_tag1, __FUNCTION__);
    return IDX;
}

void ATest_set_idx(void* feature, AppendData data, FtInt idx)
{
    printf("%s::%s(),idx is %d\n", file_tag1, __FUNCTION__, idx);
    IDX = idx;
}
