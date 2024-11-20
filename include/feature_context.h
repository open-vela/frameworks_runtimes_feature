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
 * @brief Feature开发者无法直接使用JSValue or Wasm等前端对象, \n
 * 该文件定义了ft_value_t和一系列通用的接口函数帮助开发者处理这些值，而无须关心前端对象的差异。
 */
#ifndef __FEATURE_CONTEXT_H__
#define __FEATURE_CONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/** ft_value_t类型枚举 */
typedef enum ft_type {
    FT_TYPE_NULL = -2, /**< 空值 */
    FT_TYPE_UNDEF = -1, /**< 未定义 */
    FT_TYPE_NONE = 0, /**< 未识别类型 */
    FT_TYPE_NUMBER, /**< 数字 */
    FT_TYPE_BOOL, /**< bool值 */
    FT_TYPE_STRING, /**< 字符串 */
    FT_TYPE_ARRAY, /**< 数组 */
    FT_TYPE_BUFFER, /**< buffer */
    FT_TYPE_TYPED_BUFFER, /**< 带类型buffer */
    FT_TYPE_OBJECT /**< obj */
} ft_type;

/** ft_array数组类型枚举 */
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
 * @brief 服务于feature运行时上下文的结构体定义
 *
 * @note 虽然Feature框架提供了一组API和工具, 帮助开发者隔离JS环境 \n
 * 但是毕竟有一些场景下, 需要传递非常复杂的数据结构, 而这些结构很难映射到具体的C/C++结构体或者对象上. \n
 * 为了解决该问题, 框架对js对象进行包装并定义为ft_value_t, 并且开放一些接口, 方便开发者使用.
 */
typedef struct ft_value_t {
#if INTPTR_MAX >= INT64_MAX
    uint64_t val[2]; /**< 真实值 */
#else
    uint64_t val; /**< 真实值 */
#endif
} ft_value_t;

typedef ft_value_t* ft_value_ref;
typedef const ft_value_t ft_value_const;

struct FeatureContext;

/**
 * @brief 是一个上下文对象，用于保存和管理特定数据。
 *
 * @note ft_value_t需要一个伴生对象ft_context_ref（*FeatureContext）,该对象代表着一个上下文，是ft_value_t必须的
 * @image html ft_context.svg JS层和feature框架接口关系 width=900px
 */
typedef struct FeatureContext* ft_context_ref;

/**
 * @brief 从FeatureContext中获取数据
 *
 * @param[in] ft_ctx current feature context
 * @return 用户数据
 * @note 该函数开发者并未用到，可通过`FeatureGetObjectData()`获取用户数据
 */
void* ft_context_get_data(ft_context_ref ft_ctx);

/**
 * @brief 获取ft_value_t类型
 *
 * @param[in] ft_ctx current feature context
 * @param[in] ft_val a ft_value_t argv
 * @return 该ft_value_t对象类型 @see ft_type
 * @note 开发者使用object对象时，可以通过该接口拿到更确切的参数类型
 */
ft_type ft_get_type(ft_context_ref ft_ctx, ft_value_t ft_val);

// feature type creation from native types
ft_value_t ft_from_int(ft_context_ref ft_ctx, int32_t val);
ft_value_t ft_from_uint(ft_context_ref ft_ctx, uint32_t val);
ft_value_t ft_from_int64(ft_context_ref ft_ctx, int64_t val);
ft_value_t ft_from_uint64(ft_context_ref ft_ctx, uint64_t val);
ft_value_t ft_from_double(ft_context_ref ft_ctx, double val);
ft_value_t ft_from_bool(ft_context_ref ft_ctx, bool val);

/**
 * @brief 将native字符转换成ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] val a const char* argv
 * @return ft_value_t
 * @note 开发者需要注意：通过该接口返回的ft_value_t需要用`ft_free_value()`释放内存
 */
ft_value_t ft_from_string(ft_context_ref ft_ctx, const char* val);

/**
 * @brief 将native buffer转换成ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] buff a uint8_t* buffer
 * @param[in] size the size of the buffer
 * @return ft_value_t
 * @note 开发者需要注意：通过该接口返回的ft_value_t需要用`ft_free_value()`释放内存
 */
ft_value_t ft_from_buffer(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size);

/**
 * @brief 将native buffer转换成指定类型的array ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] buff a uint8_t* buffer
 * @param[in] size the size of the buffer
 * @param[in] type 指定buffer类型 @see FtTypedArrayType
 * @return ft_value_t
 * @note 开发者需要注意：通过该接口返回的ft_value_t需要用`ft_free_value()`释放内存
 * @code
 * // get buff from js_ctx
 * uint8_t* buff = ft_to_buffer(ft_ctx, size, data);
 * // do something with buff
 * // convert typed_array_buffer to js_ctx
 * ft_value_t ret = ft_from_typed_buffer(ft_ctx, buff, size, FT_Uint8Array);
 * @endcode
 */
ft_value_t ft_from_typed_buffer(ft_context_ref ft_ctx, uint8_t* buff, uint32_t size, FtTypedArrayType type);

ft_value_t ft_from_int_array(ft_context_ref ft_ctx, int32_t* val, uint32_t size);
ft_value_t ft_from_uint_array(ft_context_ref ft_ctx, uint32_t* val, uint32_t size);
ft_value_t ft_from_int64_array(ft_context_ref ft_ctx, int64_t* val, uint32_t size);
ft_value_t ft_from_uint64_array(ft_context_ref ft_ctx, uint64_t* val, uint32_t size);
ft_value_t ft_from_bool_array(ft_context_ref ft_ctx, bool* val, uint32_t size);
ft_value_t ft_from_double_array(ft_context_ref ft_ctx, double* val, uint32_t size);
ft_value_t ft_from_string_array(ft_context_ref ft_ctx, const char** val, uint32_t size);

/**
 * @brief parse a json string to ft_value_t
 *
 * @param[in] ft_ctx current feature context
 * @param[in] buf a json string
 * @param[in] buf_len size of the json string
 * @param[in] filename the filename of the json string
 * @return ft_value_t
 * @note 开发者需要注意：通过该接口返回的ft_value_t需要用`ft_free_value()`释放内存
 */
ft_value_t ft_parse_json(ft_context_ref ft_ctx, const char* buf, size_t buf_len, const char* filename);

// feature type to native types
bool ft_to_int(ft_context_ref ft_ctx, ft_value_t f_val, int32_t* val);
bool ft_to_uint(ft_context_ref ft_ctx, ft_value_t f_val, uint32_t* val);
bool ft_to_int64(ft_context_ref ft_ctx, ft_value_t f_val, int64_t* val);
bool ft_to_uint64(ft_context_ref ft_ctx, ft_value_t f_val, uint64_t* val);
bool ft_to_double(ft_context_ref ft_ctx, ft_value_t f_val, double* val);
bool ft_to_bool(ft_context_ref ft_ctx, ft_value_t ft_val, bool* val);

/**
 * @brief convert ft_value_t to string
 *
 * @param[in] ft_ctx current feature context
 * @param[in] f_val a ft_value_t argument
 * @return const char*
 * @note `ft_to_string()`得到的字符串，需要调用`ft_free_string()`来删除
 */
const char* ft_to_string(ft_context_ref ft_ctx, ft_value_t f_val);

/** convert ft_value_t to buffer */
uint8_t* ft_to_buffer(ft_context_ref ft_ctx, size_t* p_size, ft_value_t f_val);

// array operations
uint32_t ft_array_size(ft_context_ref ft_ctx, const ft_value_t array);

/**
 * @brief 通过下标查找数组中一个元素
 *
 * @param[in] ft_ctx current feature context
 * @param[in] array a ft_value_t typed array
 * @param[in] idx the index of the element
 * @return 数组下标对应位置元素
 * @note 开发者需要注意：通过该接口返回的ft_value_t需要用`ft_free_value()`释放内存
 */
ft_value_t ft_array_at(ft_context_ref ft_ctx, const ft_value_t array, uint32_t idx);

/**
 * @brief 创建一个新的ft_object对象,可以自定义内部数据
 *
 * @param[in] ft_ctx current feature context
 * @return ft_value_t
 * @note 开发者需要注意：通过该接口返回的ft_value_t需要用`ft_free_value()`释放内存
 * @attention ft_object支持挂载子属性，释放内存时仅需释放根节点对应的ft_object
 */
ft_value_t ft_new_object(ft_context_ref ft_ctx);

/**
 * @brief 从ft_val中拿到属性名对应的值
 *
 * @param[in] ft_ctx current feature context
 * @param[in] ft_val a ft_value_t object
 * @param[in] prop 属性名称
 * @return 与属性名绑定的值
 * @note 开发者需要注意：通过该接口返回的ft_value_t需要用`ft_free_value()`释放内存
 */
ft_value_t ft_obj_get_property(ft_context_ref ft_ctx, ft_value_t ft_val, const char* prop);

/**
 * @brief set the value to the ft_val by property name
 *
 * @param[in] ft_ctx current feature context
 * @param[in] obj a ft_value_t object
 * @param[in] prop 属性名称
 * @param[in] val 属性值
 * @return true 设置成功
 * @return false 设置失败
 */
bool ft_obj_set_property(ft_context_ref ft_ctx, ft_value_t obj, const char* prop, ft_value_t val);

/**
 * @brief free resources of the ft_val
 *
 * @param[in] ft_ctx current feature context
 * @param[in] ft_val a ft_value_t object
 * @note 如果不能正确释放ft_value_t对象，就可能导致内存泄漏。
 * @attention 调用`ft_free_value()`是有要求的，不是所有的场合都需要free \n
 * 下列场合不需要free: \n
 * (1) 当ft_value_t作为参数传递给feature实现wrap函数时 \n
 * (2) 当创建的ft_value_t对象需要返回给前端的时候 \n
 * 下列场合需要free: \n
 * (1) 当调用ft_from_xxx系列函数，`ft_new_object()`创建的对象 \n
 * (2) `ft_array_at()`返回的对象 \n
 * (3) `ft_obj_get_property()`获得的对象 \n
 * (4) `ft_parse_json()`获得的对象 \n
 * @warning `ft_to_string()`得到的字符串，需要调用`ft_free_string()`来删除
 */
void ft_free_value(ft_context_ref ft_ctx, ft_value_t ft_val);

/**
 * @brief free string of the current feature context
 *
 * @param[in] ft_ctx current feature context
 * @param[in] str a string
 * @attention `ft_to_string()` 这两个函数必须成对出现 \n
 * feature框架不保证`ft_to_string()`获取的对象长期有效，开发者应该及时copy字符串的值
 */
void ft_free_string(ft_context_ref ft_ctx, const char* str);
ft_value_t ft_undefined(ft_context_ref ft_ctx);

#ifdef __cplusplus
}
#endif

#endif // __FEATURE_CONTEXT_H__
