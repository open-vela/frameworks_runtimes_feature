#include "function_test.h"

#include "unit_jidl_util.h"

// FeatureCallbacks to be implemented
void function_test_onRegister(const char* feature_name)
{
    return;
}
void function_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}
void function_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}
void function_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}
void function_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}
void function_test_onUnregister(const char* feature_name)
{
    return;
}

// Struct defines

// Function wrappers to be implemented
FtInt function_test_wrap_intTest(FeatureInstanceHandle feature, AppendData append_data, FtInt parame)
{
    return parame;
}

FtBool function_test_wrap_booleanTest(FeatureInstanceHandle feature, AppendData append_data, FtBool parame)
{
    return parame;
}

FtUint32 function_test_wrap_uintTest(FeatureInstanceHandle feature, AppendData append_data, FtUint32 parame)
{
    return parame;
}

FtInt64 function_test_wrap_longTest(FeatureInstanceHandle feature, AppendData append_data, FtInt64 parame)
{
    return parame;
}

FtFloat function_test_wrap_floatTest(FeatureInstanceHandle feature, AppendData append_data, FtFloat parame)
{
    return parame;
}

FtDouble function_test_wrap_doubleTest(FeatureInstanceHandle feature, AppendData append_data, FtDouble parame)
{
    return parame;
}

FtString function_test_wrap_strTest(FeatureInstanceHandle feature, AppendData append_data, FtString parame)
{
    FeatureDupValue((char*)parame);
    return parame;
}

FtBool function_test_wrap_cbTest(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId cb)
{
    if (FeatureCheckCallbackId(feature, cb)) {
        FeatureInvokeCallback(feature, cb, 200);
        return true;
    }
    return false;
}

FtAny function_test_wrap_objectTest(FeatureInstanceHandle feature, AppendData append_data, FtAny obj)
{
    FeatureDupValue(obj);
    return obj;
}

FtBool function_test_wrap_optionTest(FeatureInstanceHandle feature, AppendData append_data, FtInt i, FtBool b, FtUint32 u, FtInt64 l, FtFloat f, FtDouble d, FtString s)
{
    if (i != 1)
        return false;
    if (b == true)
        return false;
    if (u != (FtUint32)77)
        return false;
    if (l != 10000000000)
        return false;
    if (!JidlUtils::almostEqualFloat(f, 1.123f, 1e-5f))
        return false;
    if (!JidlUtils::almostEqualDouble(d, 1.77777, 1e-5))
        return false;
    if (strcmp(s, "hello"))
        return false;
    return true;
}

FtBool function_test_wrap_optionTest1(FeatureInstanceHandle feature, AppendData append_data, FtAny obj)
{
    if (obj)
        return false;
    return true;
}

FtBool function_test_wrap_optionTest2(FeatureInstanceHandle feature, AppendData append_data, FtInt a, FtUint32 u)
{
    if (u != (FtUint32)100)
        return false;
    return true;
}

FtBool function_test_wrap_optionTest3(FeatureInstanceHandle feature, AppendData append_data, FtInt a, FtUint32 u, FtFloat f)
{
    if (u != (FtUint32)100)
        return false;
    return true;
}

FtBool function_test_wrap_variableTest(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params)
{
    // TODO: test vari_params===>FT_TYPE_UNDEF,  FT_TYPE_NULL, FT_TYPE_BUFFER, FT_TYPE_TYPED_BUFFER
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    for (int i = 0; i < vari_params.vari_count; i++) {
        ft_value_t param = vari_params.vari_args[i];
        ft_type param_type = ft_get_type(ft_ctx, param);
        if (param_type == FT_TYPE_ARRAY) {
            uint32_t array_size = ft_array_size(ft_ctx, param);
            for (uint32_t j = 0; j < array_size; ++j) {
                ft_value_t elem = ft_array_at(ft_ctx, param, j);
                ft_type elem_type = ft_get_type(ft_ctx, elem);
                if (elem_type == FT_TYPE_NUMBER) {
                    double param_num;
                    if (!ft_to_double(ft_ctx, elem, &param_num))
                        return false;
                } else if (elem_type == FT_TYPE_STRING) {
                    const char* param_str = ft_to_string(ft_ctx, elem);
                    ft_free_string(ft_ctx, param_str);
                } else if (elem_type == FT_TYPE_BOOL) {
                    bool param_bool;
                    if (!ft_to_bool(ft_ctx, param, &param_bool))
                        return false;
                } else {
                    return false;
                }
            }
        } else if (param_type == FT_TYPE_STRING) {
            const char* param_str = ft_to_string(ft_ctx, param);
            ft_free_string(ft_ctx, param_str);
        } else if (param_type == FT_TYPE_NUMBER) {
            double param_num;
            if (!ft_to_double(ft_ctx, param, &param_num))
                return false;
        } else if (param_type == FT_TYPE_BOOL) {
            bool param_bool;
            if (!ft_to_bool(ft_ctx, param, &param_bool))
                return false;
        } else {
            return false;
        }
    }
    return true;
}

FtBool function_test_wrap_variableTest1(FeatureInstanceHandle feature, AppendData append_data, FtInt a, FtString b, FtVariParams vari_params)
{
    if (a != 1)
        return false;
    if (strcmp(b, "hello"))
        return false;
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    for (int i = 0; i < vari_params.vari_count; i++) {
        ft_value_t param = vari_params.vari_args[i];
        ft_type param_type = ft_get_type(ft_ctx, param);
        if (param_type == FT_TYPE_ARRAY) {
            uint32_t array_size = ft_array_size(ft_ctx, param);
            for (uint32_t j = 0; j < array_size; ++j) {
                ft_value_t elem = ft_array_at(ft_ctx, param, j);
                ft_type elem_type = ft_get_type(ft_ctx, elem);
                if (elem_type == FT_TYPE_NUMBER) {
                    double param_num;
                    if (!ft_to_double(ft_ctx, elem, &param_num))
                        return false;
                } else if (elem_type == FT_TYPE_STRING) {
                    const char* param_str = ft_to_string(ft_ctx, elem);
                    ft_free_string(ft_ctx, param_str);
                } else if (elem_type == FT_TYPE_BOOL) {
                    bool param_bool;
                    if (!ft_to_bool(ft_ctx, param, &param_bool))
                        return false;
                } else {
                    return false;
                }
            }
        } else if (param_type == FT_TYPE_STRING) {
            const char* param_str = ft_to_string(ft_ctx, param);
            ft_free_string(ft_ctx, param_str);
        } else if (param_type == FT_TYPE_NUMBER) {
            double param_num;
            if (!ft_to_double(ft_ctx, param, &param_num))
                return false;
        } else if (param_type == FT_TYPE_BOOL) {
            bool param_bool;
            if (!ft_to_bool(ft_ctx, param, &param_bool))
                return false;
        } else {
            return false;
        }
    }
    return true;
}

FtString function_test_wrap_throwErrorTest(FeatureInstanceHandle feature, AppendData append_data, FtBool flag)
{
    if (flag) {
        FeatureThrowError(feature, "test error");
        return NULL;
    }
    char* ret = (char*)FeatureMalloc(5, FT_STRING);
    sprintf(ret, "%s", "test");
    return ret;
}

void function_test_wrap_throwErrorTest1(FeatureInstanceHandle feature, AppendData append_data, FtBool flag)
{
    if (flag) {
        FeatureThrowError(feature, "test error");
    }
    return;
}