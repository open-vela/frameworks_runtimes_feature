#include "any_test.h"

#include "unit_jidl_util.h"

void any_test_onRegister(const char* feature_name)
{
    return;
}

void any_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}

void any_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}

void any_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}

void any_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}

void any_test_onUnregister(const char* feature_name)
{
    return;
}

FtBool any_test_wrap_testSimple(FeatureInstanceHandle feature, AppendData append_data, any_test_Simple* s)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    const char* test;
    test = ft_to_string(ft_ctx, *(s->test));
    if (strcmp(test, "hello world")) {
        ft_free_string(ft_ctx, test);
        return false;
    }
    ft_free_string(ft_ctx, test);
    return true;
}

any_test_Simple* any_test_wrap_testSimple1(FeatureInstanceHandle feature, AppendData append_data)
{
    any_test_Simple* s = any_testMallocSimple();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val = ft_from_string(ft_ctx, "hello world");
    s->test = val;
    return s;
}

FtBool any_test_wrap_testComplex(FeatureInstanceHandle feature, AppendData append_data, any_test_Complex* c)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    const char* test_option = ft_to_string(ft_ctx, *(c->test_option));
    if (strcmp(test_option, "hello world1")) {
        ft_free_string(ft_ctx, test_option);
        return false;
    }
    ft_free_string(ft_ctx, test_option);

    const char* test = ft_to_string(ft_ctx, *(c->simple->test));
    if (strcmp(test, "hello world2")) {
        ft_free_string(ft_ctx, test);
        return false;
    }
    ft_free_string(ft_ctx, test);

    if (c->any_array != nullptr) {
        FTArrayHelper<ft_value_t*> arrayHelper(c->any_array);
        for (int i = 0; i < arrayHelper.size(); i++) {
            const char* elem = ft_to_string(ft_ctx, *arrayHelper[i]);
            if (strcmp(elem, "hello world")) {
                ft_free_string(ft_ctx, elem);
                return false;
            }
            ft_free_string(ft_ctx, elem);
        }
    }
    return true;
}

any_test_Complex* any_test_wrap_testComplex1(FeatureInstanceHandle feature, AppendData append_data)
{
    any_test_Complex* c = any_testMallocComplex();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val = ft_from_string(ft_ctx, "hello world");
    c->test_option = val;
    return c;
}

any_test_Complex* any_test_wrap_testComplex2(FeatureInstanceHandle feature, AppendData append_data)
{
    any_test_Complex* c = any_testMallocComplex();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val = ft_from_string(ft_ctx, "hello world");
    c->simple = any_testMallocSimple();
    c->simple->test = val;
    return c;
}

any_test_Complex* any_test_wrap_testComplex3(FeatureInstanceHandle feature, AppendData append_data)
{
    any_test_Complex* c = any_testMallocComplex();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val1 = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val1 = ft_from_string(ft_ctx, "hello");
    ft_value_t* val2 = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val2 = ft_from_string(ft_ctx, "world");
    FtArray* array;
    array = any_test_malloc_object_array();
    array->_size = 2;
    array->_element = malloc(sizeof(ft_value_t*) * array->_size);
    ((ft_value_t**)array->_element)[0] = val1;
    ((ft_value_t**)array->_element)[1] = val2;
    c->any_array = array;
    return c;
}

any_test_Complex* any_test_wrap_testComplex4(FeatureInstanceHandle feature, AppendData append_data)
{
    any_test_Complex* c = any_testMallocComplex();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val = ft_from_string(ft_ctx, "hello world");
    c->test_option = val;

    ft_value_t* val1 = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val1 = ft_from_string(ft_ctx, "hello world1");
    c->simple = any_testMallocSimple();
    c->simple->test = val1;

    ft_value_t* val2 = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val2 = ft_from_string(ft_ctx, "hello");
    ft_value_t* val3 = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val3 = ft_from_string(ft_ctx, "world");
    FtArray* array;
    array = any_test_malloc_object_array();
    array->_size = 2;
    array->_element = malloc(sizeof(ft_value_t*) * array->_size);
    ((ft_value_t**)array->_element)[0] = val2;
    ((ft_value_t**)array->_element)[1] = val3;
    c->any_array = array;
    return c;
}

FtBool any_test_wrap_testCallback(FeatureInstanceHandle feature, AppendData append_data, FtCallbackId cb)
{
    if (!FeatureCheckCallbackId(feature, cb))
        return false;
    any_test_Simple* s = any_testMallocSimple();
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val = ft_from_string(ft_ctx, "hello world");
    s->test = val;
    if (!FeatureInvokeCallback(feature, cb, s)) {
        ft_free_value(ft_ctx, *val);
        return false;
    }
    ft_free_value(ft_ctx, *val);
    FeatureFreeValue(s);
    return true;
}

FtBool any_test_wrap_testObject(FeatureInstanceHandle feature, AppendData append_data, FtAny o)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    if (o != nullptr) {
        const char* test = ft_to_string(ft_ctx, *o);
        if (strcmp(test, "obj test")) {
            ft_free_string(ft_ctx, test);
            return false;
        }
        ft_free_string(ft_ctx, test);
        return true;
    } else {
        return false;
    }
}

FtAny any_test_wrap_testObject1(FeatureInstanceHandle feature, AppendData append_data)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val = ft_from_string(ft_ctx, "obj test");
    return val;
}