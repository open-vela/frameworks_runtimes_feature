#include "struct_test.h"

#include "unit_jidl_util.h"

void struct_test_onRegister(const char* feature_name)
{
    return;
}

void struct_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}

void struct_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}

void struct_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    return;
}

void struct_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    return;
}

void struct_test_onUnregister(const char* feature_name)
{
    return;
}

FtBool struct_test_wrap_testSimpleDefault(FeatureInstanceHandle feature, AppendData append_data, struct_test_Simple* s)
{
    if (s->int_test != 1)
        return false;
    if (s->boolean_test == true)
        return false;
    if (s->uint_test != 1)
        return false;
    if (s->long_test != 10000000000)
        return false;
    if (!JidlUtils::almostEqualFloat(s->float_test, 4.3f, 1e-5f))
        return false;
    if (!JidlUtils::almostEqualDouble(s->double_test, 5.2364, 1e-5))
        return false;
    if (strcmp(s->title, "hello world"))
        return false;
    return true;
}

struct_test_Simple* struct_test_wrap_testSimpleDefault1(FeatureInstanceHandle feature, AppendData append_data)
{
    struct_test_Simple* s = struct_testMallocSimple();

    s->int_test = 2;
    s->boolean_test = false;
    s->uint_test = 2;
    s->long_test = 20000000000;
    s->float_test = 3.4f;
    s->double_test = 1.234;
    s->title = JidlUtils::stringToFtString("hello world");
    s->fail = 0;
    return s;
}

FtBool struct_test_wrap_testSimpleReq(FeatureInstanceHandle feature, AppendData append_data, struct_test_SimpleReq* s)
{
    if (s->int_test != 1)
        return false;
    if (s->boolean_test == true)
        return false;
    if (s->uint_test != 1)
        return false;
    if (s->long_test != 10000000000)
        return false;
    if (!JidlUtils::almostEqualFloat(s->float_test, 4.3f, 1e-5f))
        return false;
    if (!JidlUtils::almostEqualDouble(s->double_test, 5.2364, 1e-5))
        return false;
    if (strcmp(s->title, "hello world"))
        return false;
    return true;
}

FtBool struct_test_wrap_testComplex(FeatureInstanceHandle feature, AppendData append_data, struct_test_Complex* c)
{
    if (c->simple != nullptr) {
        if (c->simple->int_test != 1)
            return false;
        if (c->simple->boolean_test == true)
            return false;
        if (c->simple->uint_test != 1)
            return false;
        if (c->simple->long_test != 10000000000)
            return false;
        if (!JidlUtils::almostEqualFloat(c->simple->float_test, 4.3f, 1e-5f))
            return false;
        if (!JidlUtils::almostEqualDouble(c->simple->double_test, 5.2364, 1e-5))
            return false;
        if (strcmp(c->simple->title, "hello world"))
            return false;
    }
    if (c->string_array != nullptr) {
        FTArrayHelper<const char*> arrayHelper(c->string_array);
        for (int i = 0; i < arrayHelper.size(); i++) {
            if (strcmp(arrayHelper[i], "hello world"))
                return false;
        }
    }
    if (c->double_array != nullptr) {
        FTArrayHelper<double> arrayHelper(c->double_array);
        for (int i = 0; i < arrayHelper.size(); i++) {
            if (!JidlUtils::almostEqualDouble(arrayHelper[i], 5.2364, 1e-5))
                return false;
        }
    }
    if (!FeatureCheckCallbackId(feature, c->test_cb)) {
        return false;
    }
    FeatureInvokeCallback(feature, c->test_cb, true);
    FeatureRemoveCallback(feature, c->test_cb);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_type type1 = ft_get_type(ft_ctx, *(c->object_test1));
    ft_type type2 = ft_get_type(ft_ctx, *(c->object_test2));
    if (type1 != FT_TYPE_NUMBER || type2 != FT_TYPE_STRING)
        return false;
    int32_t num = 0;
    if (!ft_to_int(ft_ctx, *(c->object_test1), &num)) {
        return false;
    }
    if (num != 1) {
        return false;
    }
    const char* str = ft_to_string(ft_ctx, *(c->object_test2));
    if (strcmp(str, "hello world")) {
        ft_free_string(ft_ctx, str);
        return false;
    }
    ft_free_string(ft_ctx, str);
    return true;
}

struct_test_Complex* struct_test_wrap_testComplex1(FeatureInstanceHandle feature, AppendData append_data)
{
    struct_test_Complex* c = struct_testMallocComplex();
    struct_test_Simple* s = struct_testMallocSimple();

    s->int_test = 2;
    s->boolean_test = false;
    s->uint_test = 2;
    s->long_test = 20000000000;
    s->float_test = 3.4f;
    s->double_test = 1.234;
    s->title = JidlUtils::stringToFtString("hello world");
    s->fail = 0;
    c->simple = s;
    FtArray* array;
    array = struct_test_malloc_string_array();
    array->_size = 2;
    array->_element = malloc(sizeof(const char*) * array->_size);
    ((const char**)array->_element)[0] = JidlUtils::stringToFtString("hello");
    ((const char**)array->_element)[1] = JidlUtils::stringToFtString("world");
    c->string_array = array;
    FtArray* array1;
    array1 = struct_test_malloc_double_array();
    array1->_size = 2;
    array1->_element = malloc(sizeof(double) * array1->_size);
    ((double*)array1->_element)[0] = 1.1234;
    ((double*)array1->_element)[1] = 2.2727;
    c->double_array = array1;
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    ft_value_t* val1 = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val1 = ft_from_int(ft_ctx, 1);
    ft_value_t* val2 = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *val2 = ft_from_string(ft_ctx, "hello world");
    c->object_test1 = val1;
    c->object_test2 = val2;
    return c;
}

FtBool struct_test_wrap_testNested(FeatureInstanceHandle feature, AppendData append_data, struct_test_Nested* n)
{
    return true;
}

struct_test_Nested* struct_test_wrap_testNested1(FeatureInstanceHandle feature, AppendData append_data)
{
    FtArray* array;
    array = struct_test_malloc_Nested_struct_type_array();
    array->_size = 0;
    array->_element = nullptr;
    struct_test_Nested* n = struct_testMallocNested();

    n->subs = array;
    n->a = 2;
    return n;
}
