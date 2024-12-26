/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file feature_context.h
 * @brief Feature developers cannot directly use front-end objects such as JSValue or Wasm. \n
 * This file defines ft_value_t and a series of common interface functions \n
 * to help developers process these values ​​without having to worry about the differences in front-end objects.。
 */
#ifndef __FEATURE_CONTEXT_H__
#define __FEATURE_CONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/** ft_value_t type */
typedef enum ft_type {
    FT_TYPE_NULL = -2, /**< null */
    FT_TYPE_UNDEF = -1, /**< undefined */
    FT_TYPE_NONE = 0, /**< none */
    FT_TYPE_NUMBER, /**< number */
    FT_TYPE_BOOL, /**< bool */
    FT_TYPE_STRING, /**< string */
    FT_TYPE_ARRAY, /**< array */
    FT_TYPE_BUFFER, /**< buffer */
    FT_TYPE_TYPED_BUFFER, /**< typed_buffer */
    FT_TYPE_OBJECT /**< obj */
} ft_type;

/** ft_array type */
typedef enum FtTypedArrayType {
    FT_Int8Array = 0, /**< 0 */
    FT_Uint8Array, /**< 1 */
    FT_Int16Array, /**< 2 */
    FT_Uint16Array, /**< 3 */
    FT_Int32Array, /**< 4 */
    FT_Uint32Array, /**< 5 */
    FT_Float32Array, /**< 6 */
    FT_Float64Array /**< 7 */
} FtTypedArrayType;

/**
 * @brief a base struct for feature framwork
 *
 * @note Although the Feature framework provides a set of APIs and tools to help developers isolate the JS environment \n
 * in some scenarios, very complex data structures need to be passed, \n
 * and these structures are difficult to map to specific C/C++ structures or objects. \n
 * To solve this problem, the framework packages the js object and defines it as ft_value_t, and opens some interfaces for developers to use.
 */
typedef struct ft_value_t {
#if INTPTR_MAX >= INT64_MAX
    uint64_t val[2]; /**< value */
#else
    uint64_t val; /**< value */
#endif
} ft_value_t;

/** pointer of ft_value_t */
typedef ft_value_t* ft_value_ref;

/** const ft_value_t */

typedef const ft_value_t ft_value_const;

/** a struct representing the context */
struct FeatureContext;

/**
 * @brief A context object used to store and manage user data
 *
 * @note ft_value_t requires a companion object: ft_context_ref(*FeatureContext) \n
 * This object represents a context and is required by ft_value_t.
 * @image html ft_context.svg JS&&FeatureFramework width=900px
 */
typedef struct FeatureContext* ft_context_ref;

/**
 * @brief get data from FeatureContext
 *
 * @param[in] ft_ctx current feature context
 * @return userdata
 */
void* ft_context_get_data(ft_context_ref ft_ctx);

/**
 * @brief get type from a ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] ft_val a ft_value_t arguments
 * @return the type of the ft_value_t @see ft_type
 * @note this func can be used to sure the type of the ft_value_t
 */
ft_type ft_get_type(ft_context_ref ft_ctx, ft_value_t ft_val);

/** convert int to ft_value_t */
ft_value_t ft_from_int(ft_context_ref ft_ctx, int32_t val);
/** convert uint32_t to ft_value_t */
ft_value_t ft_from_uint(ft_context_ref ft_ctx, uint32_t val);
/** convert int64_t to ft_value_t */
ft_value_t ft_from_int64(ft_context_ref ft_ctx, int64_t val);
/** convert uint64_t to ft_value_t */
ft_value_t ft_from_uint64(ft_context_ref ft_ctx, uint64_t val);
/** convert double to ft_value_t */
ft_value_t ft_from_double(ft_context_ref ft_ctx, double val);
/** convert bool to ft_value_t */
ft_value_t ft_from_bool(ft_context_ref ft_ctx, bool val);

/**
 * @brief convert string to ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] val the string argment
 * @return ft_value_t
 * @note Developers need to note: the ft_value_t returned by this interface needs to be freed using `ft_free_value()`
 */
ft_value_t ft_from_string(ft_context_ref ft_ctx, const char* val);

/**
 * @brief convert native buffer to ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] buff a uint8_t* buffer
 * @param[in] size the size of the buffer
 * @return ft_value_t
 * @note Developers need to note: the ft_value_t returned by this interface needs to be freed using `ft_free_value()`
 */
ft_value_t ft_from_buffer(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size);

/**
 * @brief convert typed_array_buffer to ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] buff a uint8_t* buffer
 * @param[in] size the size of the buffer
 * @param[in] type buffer type @see FtTypedArrayType
 * @return ft_value_t
 * @note Developers need to note: the ft_value_t returned by this interface needs to be freed using `ft_free_value()`
 * @code
 * // get buff from js_ctx
 * uint8_t* buff = ft_to_buffer(ft_ctx, size, data);
 * // do something with buff
 * // convert typed_array_buffer to js_ctx
 * ft_value_t ret = ft_from_typed_buffer(ft_ctx, buff, size, FT_Uint8Array);
 * @endcode
 */
ft_value_t ft_from_typed_buffer(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size, FtTypedArrayType type);

/** convert int_array to ft_value_t */
ft_value_t ft_from_int_array(ft_context_ref ft_ctx, int32_t* val, uint32_t size);
/** convert uint_array to ft_value_t */
ft_value_t ft_from_uint_array(ft_context_ref ft_ctx, uint32_t* val, uint32_t size);
/** convert int64_array to ft_value_t */
ft_value_t ft_from_int64_array(ft_context_ref ft_ctx, int64_t* val, uint32_t size);
/** convert uint64_array to ft_value_t */
ft_value_t ft_from_uint64_array(ft_context_ref ft_ctx, uint64_t* val, uint32_t size);
/** convert bool_array to ft_value_t */
ft_value_t ft_from_bool_array(ft_context_ref ft_ctx, bool* val, uint32_t size);
/** convert double_array to ft_value_t */
ft_value_t ft_from_double_array(ft_context_ref ft_ctx, double* val, uint32_t size);
/** convert string_array to ft_value_t */
ft_value_t ft_from_string_array(ft_context_ref ft_ctx, const char** val, uint32_t size);

/**
 * @brief parse a json string to ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] buf a json string
 * @param[in] buf_len size of the json string
 * @param[in] filename the filename of the json string
 * @return ft_value_t
 * @note Developers need to note: the ft_value_t returned by this interface needs to be freed using `ft_free_value()`
 */
ft_value_t ft_parse_json(ft_context_ref ft_ctx, const char* buf, size_t buf_len, const char* filename);

/** convert ft_value_t to int32_t */
bool ft_to_int(ft_context_ref ft_ctx, ft_value_t f_val, int32_t* val);
/** convert ft_value_t to uint32_t */
bool ft_to_uint(ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* val);
/** convert ft_value_t to int64_t */
bool ft_to_int64(ft_context_ref ft_ctx, ft_value_t f_val, int64_t* val);
/** convert ft_value_t to uint64_t */
bool ft_to_uint64(ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* val);
/** convert ft_value_t to double */
bool ft_to_double(ft_context_ref ft_ctx, ft_value_t f_val, double* val);
/** convert ft_value_t to bool */
bool ft_to_bool(ft_context_ref ft_ctx, ft_value_t ft_val, bool* val);
/** convert ft_value_t to buffer */
uint8_t* ft_to_buffer(ft_context_ref ft_ctx, size_t* p_size, ft_value_t f_val);

/**
 * @brief convert ft_value_t to string
 *
 * @param[in] ft_ctx current feature context
 * @param[in] f_val a ft_value_t argument
 * @return const char*
 * @note The string obtained by `ft_to_string()` needs to be deleted by calling `ft_free_string()`
 */
const char* ft_to_string(ft_context_ref ft_ctx, ft_value_t f_val);

/** convert ft_value_t to buffer */
uint8_t* ft_to_buffer(ft_context_ref ft_ctx, size_t* p_size, ft_value_t f_val);

/**
 * @brief get size of the array
 *
 * @param[in] ft_ctx current feature context
 * @param[in] array a ft_value_t typed array
 * @return size of the array
 */
uint32_t ft_array_size(ft_context_ref ft_ctx, const ft_value_t array);

/**
 * @brief find the element of the array by index
 *
 * @param[in] ft_ctx current feature context
 * @param[in] array a ft_value_t typed array
 * @param[in] idx the index of the element
 * @return The array index corresponds to the position element
 * @note Developers need to note: the ft_value_t returned by this interface needs to be freed using `ft_free_value()`
 */
ft_value_t ft_array_at(ft_context_ref ft_ctx, const ft_value_t array, uint32_t idx);

/**
 * @brief create a new ft_object, developers can customize internal data
 *
 * @param[in] ft_ctx current feature context
 * @return ft_value_t
 * @note Developers need to note: the ft_value_t returned by this interface needs to be freed using `ft_free_value()`
 * @attention ft_object supports mounting sub-attributes. When releasing memory, only the ft_object corresponding to the root node needs to be released.
 */
ft_value_t ft_new_object(ft_context_ref ft_ctx);

/**
 * @brief get property value of the ft_val
 *
 * @param[in] ft_ctx current feature context
 * @param[in] ft_val a ft_value_t object
 * @param[in] prop prop name
 * @return prop value
 * @note Developers need to note: the ft_value_t returned by this interface needs to be freed using `ft_free_value()`
 */
ft_value_t ft_obj_get_property(ft_context_ref ft_ctx, ft_value_t ft_val, const char* prop);

/**
 * @brief set the value to the ft_val by property name
 *
 * @param[in] ft_ctx current feature context
 * @param[in] obj a ft_value_t object
 * @param[in] prop prop name
 * @param[in] val value
 * @return true set success
 * @return false set failed
 */
bool ft_obj_set_property(ft_context_ref ft_ctx, ft_value_t obj, const char* prop, ft_value_t val);

/**
 * @brief free resources of the ft_val
 *
 * @param[in] ft_ctx current feature context
 * @param[in] ft_val a ft_value_t object
 * @note if ft_value_t objects are not released correctly, memory leaks may occur.
 * @attention Calling `ft_free_value()` is required, not all situations require free \n
 * not need free: \n
 * (1) When ft_value_t is passed as a parameter to the feature implementation wrap function \n
 * (2) When the created ft_value_t object needs to be returned to the front end \n
 * need free: \n
 * (1) When calling the ft_from_xxx series of functions and the object created by `ft_new_object()` \n
 * (2) The object returned by `ft_array_at()` \n
 * (3) Object obtained by `ft_obj_get_property()` \n
 * (4) Object obtained from `ft_parse_json()` \n
 * @warning The string obtained by `ft_to_string()` needs to be deleted by calling `ft_free_string()`
 */
void ft_free_value(ft_context_ref ft_ctx, ft_value_t ft_val);

/**
 * @brief free string of the current feature context
 *
 * @param[in] ft_ctx current feature context
 * @param[in] str a string
 * @attention The feature framework does not guarantee that the object obtained by `ft_to_string()` is valid for a long time. \n
 * Developers should copy the string value in time.
 */
void ft_free_string(ft_context_ref ft_ctx, const char* str);
ft_value_t ft_undefined(ft_context_ref ft_ctx);

#ifdef __cplusplus
}
#endif

#endif // __FEATURE_CONTEXT_H__
