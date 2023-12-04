
#include "ajs_features_init.h"
#include "feature_exports.h"
#include "feature_description.h"
#include "feature_log.h"
#include <ffi.h>

using namespace ferry;
using namespace FEATURE;

#define countof(x) (sizeof(x) / sizeof(x[0]))

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

struct Point {
    double _x;
    double _y;
    double _z;

    Point(double x, double y, double z)
        : _x(x)
        , _y(y)
        , _z(z)
    {
    }

    Point()
        : _x(0)
        , _y(0)
        , _z(0)
    {
    }
};

static Point* __printPoint(FeatureInstanceHandle handle, int64_t data, Point* point)
{
    printf("point {x: %f, y: %f, z: %f}\n", point->_x, point->_y, point->_z);
    FeatureDupValue(point);
    point->_x += 1;
    point->_y += 2;
    return point;
}

static const char* __printString(FeatureInstanceHandle handle, int64_t data, const char* str)
{
    printf("str is: %s\n", str);
    char* buf = (char*)FeatureMalloc(128, FT_CHAR);
    sprintf(buf, "returned string: %s", str);
    return buf;
}

static Point g_point = { 100.0, 200.0, 300.0 };

static Point* __get_myPoint(FeatureInstanceHandle handle, int64_t data)
{
    Point* p = static_cast<Point*>(FeatureGetObjectData(handle));
    FeatureDupValue(p);
    return p;
}

static Point* __with_optional(FeatureInstanceHandle handle, int64_t data, const char* str)
{
    Point* p = static_cast<Point*>(FeatureGetObjectData(handle));
    FeatureDupValue(p);
    printf("with optional receive str: %s\n", str);
    return p;
}

static void __recv_point_ptr_array_ptr(FeatureInstanceHandle handle, int64_t data, FtArray& array)
{
    FTArrayHelper<Point*> point_array(&array);
    printf("%s: point_array size: %d\n", __func__, point_array.size());
    printf("point_array = [\n");
    for (int32_t i = 0; i < point_array.size(); i++) {
        Point& p = *point_array[i];
        printf("    { x = %lf, y = %lf, z = %lf }\n", p._x, p._y, p._z);
    }
    printf("]\n");
}

static void __recv_string_array_ptr(FeatureInstanceHandle handle, AppendData data, FtArray& array)
{
    FTArrayHelper<const char*> point_array(&array);
    printf("%s: point_array size: %d\n", __func__, point_array.size());
    printf("point_array = [\n");
    for (int32_t i = 0; i < point_array.size(); i++) {
        printf("    %d: %s\n", i, point_array[i]);
    }
    printf("]\n");
}

static void __return_promise(FeatureInstanceHandle handle, AppendData data, FtPromiseId pid, bool isReject)
{
    Point p;
    p._x = 1.0;
    p._y = 2.0;
    p._z = 3.0;
    if (isReject) {
        FeaturePromiseReject(handle, pid, &p);
    } else {
        FeaturePromiseResolve(handle, pid, &p);
    }
}

static void __set_myPoint(FeatureInstanceHandle handle, int64_t data, Point* point)
{
    Point* p = static_cast<Point*>(FeatureGetObjectData(handle));
    if (p) {
        FeatureFreeValue(p);
    }
    FeatureSetObjectData(handle, FeatureDupValue(point));
}

static Point* __init_const1(FeatureInstanceHandle handle, int64_t data)
{
    Point* p = (Point*)data;
    // add reference count.
    FeatureDupValue(p);
    p->_x += 1;
    p->_y += 1;
    p->_z += 1;
    return p;
}

static void __print(FeatureInstanceHandle handle, int64_t data, FtVariParams vari_params)
{
    ft_context_ref ft_ctx = FeatureGetContext(handle);
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
            for (uint32_t j = 0; j < array_size; ++j) {
                ft_value_t elem = ft_array_at(ft_ctx, param, j);
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
                    ft_to_bool(ft_ctx, param, &param_bool);
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
            ft_to_double(ft_ctx, param, &param_num);
            printf("%lf ", param_num);
        } else if (param_type == FT_TYPE_BOOL) {
            bool param_bool;
            ft_to_bool(ft_ctx, param, &param_bool);
            printf("%d ", param_bool);
        } else {
            printf("invalid param type!");
            return;
        }
    }
    printf("\n");
}

static void __func_with_cb(FeatureInstanceHandle handle, int64_t data, FtCallbackId callback)
{
    if (!FeatureInvokeCallback(handle, callback, "hello world", 123.0, 456.0, 789.0)) {
        FEATURE_LOG_ERROR("invoke failed !");
    }

    FeatureRemoveCallback(handle, callback);
}

static void __func_with_cb2(FeatureInstanceHandle handle, int64_t data, FtCallbackId callback)
{
    char* arg1 = (char*)FeatureMalloc(sizeof("test1") + 1, FT_CHAR);
    sprintf(arg1, "%s", "test1");
    char* arg2 = (char*)FeatureMalloc(sizeof("test2") + 1, FT_CHAR);
    sprintf(arg2, "%s", "test2");
    if (!FeatureInvokeCallbackCount(handle, callback, 6, "hello world", 123.0, 456.0, 789.0, arg1, arg2)) {
        FEATURE_LOG_ERROR("invoke failed !");
    }
    FeatureFreeValue(arg1);
    FeatureFreeValue(arg2);

    FeatureRemoveCallback(handle, callback);
}

static ObjectMember Point_member[] = {
    { "x", FT_DOUBLE, offsetof(Point, _x), sizeof(double) },
    { "y", FT_DOUBLE, offsetof(Point, _y), sizeof(double) },
    { "z", FT_DOUBLE, offsetof(Point, _z), sizeof(double) },
    { nullptr },
};

// complex defination
static ObjectMapType Point_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(Point) },
    .members = Point_member
};

static PromiseType Promise_type {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { FT_MK_COMPLEX_REF(&Point_type), FT_MK_COMPLEX_REF(&Point_type) }
};

static FeatureType return_promise_parameters[] = {
    FT_BOOLEAN, FT_PARAM_END
};

static FeatureType printPoint_parameters[] = {
    FT_MK_COMPLEX_REF(&Point_type), FT_PARAM_END
};

static FeatureType printString_parameters[] = {
    FT_STRING, FT_PARAM_END
};

static FeatureType print_parameters[] = {
    FT_PARAM_REST_END
};

static FeatureType cb1_parameters[] = {
    FT_STRING, FT_FLOAT, FT_FLOAT, FT_FLOAT, FT_PARAM_END
};

static CallbackType cb1_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = cb1_parameters,
    .return_type = FT_MK_COMPLEX(&Point_type)
};

static FeatureType func_with_cb_parameters[] = {
    FT_MK_COMPLEX(&cb1_type), FT_PARAM_END
};

static FeatureType cb2_parameters[] = {
    FT_STRING, FT_FLOAT, FT_FLOAT, FT_FLOAT, FT_PARAM_REST_END
};

static CallbackType cb2_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = cb2_parameters,
    .return_type = FT_MK_COMPLEX(&Point_type)
};

static FeatureType func_with_cb2_parameters[] = {
    FT_MK_COMPLEX(&cb2_type), FT_PARAM_END
};

static OptionalType with_optional_type {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = FT_STRING,
    .str = "this is optional default string"
};

static FeatureType with_optional_parameters[] = {
    FT_MK_OPTIONAL(&with_optional_type), FT_PARAM_END
};

static ArrayType point_ptr_array_type = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_MK_COMPLEX_REF(&Point_type)
};

static FeatureType recv_point_ptr_array_parameters[] = {
    FT_MK_COMPLEX_REF(&point_ptr_array_type), FT_PARAM_END
};

static ArrayType string_array_type = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_STRING
};

static FeatureType recv_string_array_parameters[] = {
    FT_MK_COMPLEX_REF(&string_array_type), FT_PARAM_END
};

Point* g_point1 = new (FeatureMalloc(sizeof(Point), FT_MK_COMPLEX(&Point_type))) Point(4.0, 5.0, 6.0);

static FtArray* __return_array(FeatureInstanceHandle handle, int64_t data)
{
    FtArray* strArray = static_cast<FtArray*>(FeatureMalloc(sizeof(FtArray), FT_MK_COMPLEX(&string_array_type)));
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_CHAR));
        sprintf(str, "hello%d", i);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

static const Member g_members[] = {
    { .type = MEMBER_METHOD, .name = "printPoint", .method = { .func = { .callback = FFI_FN(__printPoint) }, .parameters = printPoint_parameters, .return_type = FT_MK_COMPLEX_REF(&Point_type), .data = { 12 } } },
    { .type = MEMBER_METHOD, .name = "printString", .method = { .func = { .callback = FFI_FN(__printString) }, .parameters = printString_parameters, .return_type = FT_STRING, .data = { 123 } } },
    { .type = MEMBER_ACCESSOR, .name = "myPoint", .accessor = { .getter = { .callback = FFI_FN(__get_myPoint) }, .setter = { .callback = FFI_FN(__set_myPoint) }, .type = FT_MK_COMPLEX_REF(&Point_type), .data = { 100 } } },
    { .type = MEMBER_CONST, .name = "myConstant", .value = { .type = FT_MK_COMPLEX_REF(&Point_type), .func = { .callback = nullptr }, .data = { .ptr = g_point1 } } },
    { .type = MEMBER_CONST, .name = "myConstant1", .value = { .type = FT_MK_COMPLEX_REF(&Point_type), .func = { .callback = FFI_FN(__init_const1) }, .data = { .ptr = g_point1 } } },
    { .type = MEMBER_CONST, .name = "myConstant2", .value = { .type = FT_INT, .func = { .callback = nullptr }, .data = { 12345 } } },
    { .type = MEMBER_METHOD, .name = "print", .method = { .func = { .callback = FFI_FN(__print) }, .parameters = print_parameters, .return_type = FT_VOID, .data = { 0 } } },
    { .type = MEMBER_METHOD, .name = "func_with_cb", .method = { .func = { .callback = FFI_FN(__func_with_cb) }, .parameters = func_with_cb_parameters, .return_type = FT_VOID, .data = { 0 } } },
    { .type = MEMBER_METHOD, .name = "func_with_cb2", .method = { .func = { .callback = FFI_FN(__func_with_cb2) }, .parameters = func_with_cb2_parameters, .return_type = FT_VOID, .data = { 0 } } },
    { .type = MEMBER_METHOD, .name = "withOptional", .method = { .func = { .callback = FFI_FN(__with_optional) }, .parameters = with_optional_parameters, .return_type = FT_MK_COMPLEX_REF(&Point_type), .data = { .i32 = 0 } } },
    { .type = MEMBER_METHOD, .name = "recv_point_ptr_array_ptr", .method = { .func = { .callback = FFI_FN(__recv_point_ptr_array_ptr) }, .parameters = recv_point_ptr_array_parameters, .return_type = FT_VOID, .data = { .i64 = 1789 } } },
    { .type = MEMBER_METHOD, .name = "recv_string_array_ptr", .method = { .func = { .callback = FFI_FN(__recv_string_array_ptr) }, .parameters = recv_string_array_parameters, .return_type = FT_VOID, .data = { .i64 = 123456 } } },
    { .type = MEMBER_METHOD, .name = "return_promise", .method = { .func = { .callback = FFI_FN(__return_promise) }, .parameters = return_promise_parameters, .return_type = FT_MK_COMPLEX_REF(&Promise_type), .data = { .i64 = 0 } } },
    { .type = MEMBER_METHOD, .name = "return_array", .method = { .func = { .callback = FFI_FN(__return_array) }, .parameters = NULL, .return_type = FT_MK_COMPLEX_REF(&string_array_type), .data = { .i64 = 0 } } },
};

// callbacks
static const struct FeatureCallbacks callbacks {
    [](const char* feature_name) {
        FEATURE_LOG_INFO("onRegister");
    },
        [](FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
            FeatureSetProtoData(handle, new (FeatureMalloc(sizeof(Point), FT_MK_COMPLEX(&Point_type))) Point());
            FEATURE_LOG_INFO("onCreate");
        },
        [](FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
            FeatureSetObjectData(handle, new (FeatureMalloc(sizeof(Point), FT_MK_COMPLEX(&Point_type))) Point());
            FEATURE_LOG_INFO("onRequired");
        },
        [](FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
            Point* point_data = (Point*)FeatureGetObjectData(handle);
            FeatureFreeValue(point_data);
            FEATURE_LOG_INFO("onDetached");
        },
        [](FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
            Point* point_data = (Point*)FeatureGetProtoData(handle);
            FeatureFreeValue(point_data);
            FEATURE_LOG_INFO("onDestroy");
        },
        [](const char* feature_name) {
            FEATURE_LOG_INFO("onUnregister");
        }
};

static const FeatureDescription timers_description = { 1, "Timer", "js timer feature, for setTimeout and setInterval and so on", 0, &callbacks, countof(g_members), g_members };


QAPPFEATURE_INIT(timers)
{
    bool ret = false;
    ret = mgr->registerFeature(features, &timers_description);
    return ret;
}
