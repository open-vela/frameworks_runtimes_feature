#ifndef __FEATURE_WAMR_UTILS_H__
#define __FEATURE_WAMR_UTILS_H__

/* this file for new feature framework to deal with some basic operatrions about wasm obj */

#include "gc_export.h"
#include "wasm_export.h"

extern "C" wasm_struct_obj_t create_wasm_string(wasm_exec_env_t exec_env, const char *value);
extern "C" wasm_struct_obj_t create_wasm_array_with_string(wasm_exec_env_t exec_env, void *ptr, uint32_t arrlen);
extern "C" int32_t
get_string_struct_type(wasm_module_t wasm_module,
                       wasm_struct_type_t *p_struct_type);

enum field_flag {
    FIELD = 0,
    METHOD = 1,
    GETTER = 2,
    SETTER = 3,
};

typedef enum ts_value_type_t {
    TS_OBJECT = 0,
    TS_NULL = 3,
    TS_INT = 5,
    TS_NUMBER = 6,
    TS_BOOLEAN = 7,
    TS_STRING = 9,
    TS_ANY = 10,
    TS_ARRAY = 16,
    TS_FUNCTION = 24,
} ts_value_type_t;

typedef struct ts_value_t {
    ts_value_type_t type;
    /**
     * Type of the ts value, if it's TS_BOOLEAN or TS_INT, value can be retrieved from of.i32,
     * if it's TS_NUMBER, value can be retrived from f64, otherwise get value from of.ref.
    */
    union {
        int32_t i32;
        double f64;
        void *ref;
    } of;

} ts_value_t;

typedef struct Person
{
    char* name;
    char* gender;
    int age;
} Person;

static wasm_value_type_t
get_refTypeValue_from_ts_value(wasm_exec_env_t exec_env, ts_value_t ts_val)
{
    uint32_t string_type_idx;
    wasm_ref_type_t arr_ref_type;
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    wasm_struct_type_t string_struct_type = NULL;

    string_type_idx = get_string_struct_type(module, &string_struct_type);
    wasm_ref_type_set_type_idx(&arr_ref_type, true, string_type_idx);

    switch (ts_val.type)
    {
    case TS_STRING:
    {
        return arr_ref_type.value_type;
    }
    break;
    case TS_NUMBER:
    {
        return VALUE_TYPE_F64;
    }
    break;
    case TS_BOOLEAN:
    {
        return VALUE_TYPE_I32;
    }
    break;
    default:
        /*other type */
        break;
    }
    return 0;
}

static uint32_t
get_wasm_cls_struct_type(wasm_exec_env_t exec_env,
                         wasm_value_type_t field_types[], uint32_t member_count,
                         wasm_struct_type_t *p_struct_type)
{
    bool mut;
    uint32_t type_count;
    wasm_defined_type_t type;
    wasm_ref_type_t field_type;
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    type_count = wasm_get_defined_type_count(module);

    for (uint32_t i = 0; i < member_count; i++)
    {
        for (uint32_t j = 0; j < type_count; j++)
        {
            type = wasm_get_defined_type(module, j);
            if (wasm_defined_type_is_struct_type(type) && (wasm_struct_type_get_field_count((wasm_struct_type_t)type) == member_count + 1))
            {
                /* find the ref type of each field from cls struct, starting from index 1*/
                field_type = wasm_struct_type_get_field_type(
                    (wasm_struct_type_t)type, i + 1, &mut);

                if ((field_types[i] == field_type.value_type) && mut)
                {
                    if (p_struct_type)
                    {
                        *p_struct_type = (wasm_struct_type_t)type;
                        return j;
                    }
                }
            }
        }
    }
    if (p_struct_type)
    {
        *p_struct_type = NULL;
    }
    return -1;
}

/*
    utilities for class object
    * class struct (WasmGC struct)
    +----------+      +---------------------------+
    |  0:vtable| ---> |      struct (field i32)   |
    +----------+      +---------------------------+
    |  1: type |      ^      content data 1       ^
    +----------+      |---------------------------|
    |  2: type |      ^      content data 2       ^
    +----------+      |---------------------------|
    |  ...     |      ^      content ...          ^
    +----------+      |---------------------------|
*/

wasm_struct_obj_t
create_wasm_class_struct(wasm_exec_env_t exec_env, ts_value_t obj_arr[],
                         uint32_t member_count)
{
    if (member_count < 1)
        return NULL;

    wasm_value_t ele = {.gc_obj = NULL};
    wasm_struct_type_t cls_struct_type = NULL;
    wasm_struct_obj_t cls_struct_obj;
    wasm_struct_obj_t str_obj;
    wasm_value_type_t field_types[member_count];

    /* traverse the array elements and find the ref type value through the
     * element type */
    for (uint32_t i = 0; i < member_count; i++)
    {
        field_types[i] = get_refTypeValue_from_ts_value(exec_env, obj_arr[i]);
    }
    /* through ref type and member_count, find cls struct type */
    get_wasm_cls_struct_type(exec_env, field_types, member_count,
                             &cls_struct_type);

    /* create new cls struct obj through cls_struct_type */
    cls_struct_obj = wasm_struct_obj_new_with_type(exec_env, cls_struct_type);

    /* traverse the array element value and fill it in cls_struct_obj */
    for (uint32_t k = 0; k < member_count; k++)
    {
        switch (obj_arr[k].type)
        {
        case TS_STRING:
        {
            const char *str = (const char *)obj_arr[k].of.ref;
            str_obj = create_wasm_string(exec_env, str);
            ele.gc_obj = (wasm_obj_t)str_obj;
        }
        break;
        case TS_NUMBER:
        {
            ele.f64 = obj_arr[k].of.f64;
        }
        break;
        case TS_BOOLEAN:
        {
            ele.i32 = obj_arr[k].of.i32;
        }
        break;
        default:
            break;
        }
        wasm_struct_obj_set_field(cls_struct_obj, k + 1, &ele);
    }
    return cls_struct_obj;
}

#endif // __FEATURE_WAMR_UTILS_H__