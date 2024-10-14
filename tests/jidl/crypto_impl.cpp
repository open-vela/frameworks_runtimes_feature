/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "crypto.h"

#include "feature_log.h"
#include "feature_utils.h"

#include "crypto_native.h"
#include "crypto_utils.h"

#include <alloca.h>
#include <stdarg.h>
#include <stdio.h>

static const char* file_tag = "[system_crypto_impl]";

static const char* pkg_name = NULL;

typedef enum ErrorCode {
    GENERAL = 200,
    ARGSERROR = 202,
    IOERROR = 300,
    TIMEOUT = 204
} ErrorCode;

typedef enum AlgoType {
    RSA,
    AES
} AlgoType;

typedef enum HashType {
    MD5,
    SHA1,
    SHA256,
    SHA512
} HashType;

typedef enum SignHashType {
    RSA_MD5,
    RSA_SHA1,
    RSA_SHA256,
    RSA_SHA512
} SignHashType;

static const char encryptCfgs[] = "{\
    'RSA': {\
        'mode': {\
            'None': 0\
        },\
        'padding': {}\
    },\
    'AES': {\
        'mode': {\
            'CBC': 5,\
            'ECB': 2\
        },\
        'padding': {\
            'PKCS7Padding': 0\
        }\
    }\
}";

static const char* hash_types[] = {
    "MD5",
    "SHA1",
    "SHA256",
    "SHA512",
};

static const char* sign_hash_types[] = {
    "RSA-MD5",
    "RSA-SHA1",
    "RSA-SHA256",
    "RSA-SHA512",
};

static uint8_t* get_buff(ft_context_ref ft_ctx, ft_value_t data, size_t* size, bool* is_text)
{
    ft_type type = ft_get_type(ft_ctx, data);
    if (type == FT_TYPE_BUFFER || type == FT_TYPE_TYPED_BUFFER) {
        *is_text = false;
        uint8_t* buff = ft_to_buffer(ft_ctx, size, data);
        FEATURE_LOG_ERROR("%s, got buffer, type: %d, size: %ld", file_tag, type, *size);
        return buff;
    } else if (type == FT_TYPE_STRING) {
        const char* str = ft_to_string(ft_ctx, data);
        *is_text = true;
        *size = strlen(str);
        FEATURE_LOG_ERROR("%s, got string: %s", file_tag, str);
        return (uint8_t*)str;
    }
    return NULL;
}

static ft_value_t from_buff(ft_context_ref ft_ctx, const char* data, size_t size, bool is_text)
{
    FEATURE_LOG_ERROR("%s, is_text: %d", file_tag, is_text);
    if (is_text)
        return ft_from_string(ft_ctx, data);

    return ft_from_typed_buffer(ft_ctx, (uint8_t*)data, size, 0);
}

// FeatureCallbacks
void system_crypto_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void system_crypto_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
    pkg_name = FeatureGetPackageName(handle);
}

void system_crypto_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void system_crypto_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void system_crypto_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

void system_crypto_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s", file_tag);
}

FtString system_crypto_wrap_hashDigest(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_HashDigestParam* options)
{
    FEATURE_LOG_INFO("%s, options: %p", file_tag, options);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    FtString result = NULL;
    // set default value SHA256
    if (!check_str(options->algo)) {
        options->algo = hash_types[SHA256];
    } else if (!has_type(hash_types, arrayof(hash_types), options->algo)) {
        FEATURE_LOG_ERROR("%s invalid algo param: %s", file_tag, options->algo);
    }

    if (!(check_any(options->data) || check_str(options->uri))) {
        FEATURE_LOG_ERROR("%s %s", file_tag, "arguments data or uri is needed");
    } else if (check_any(options->data) && !check_str(options->uri)) {
        size_t size;
        bool is_text;
        uint8_t* buff = get_buff(ft_ctx, *(options->data), &size, &is_text);
        if (!buff) {
            FEATURE_LOG_ERROR("%s %s, %s", file_tag, "invalid data type!");
        } else {
            result = digest(options->algo, buff, size, NULL);
            if (!result && crypto_err) {
                FEATURE_LOG_ERROR("%s %s, native digest error: %s", file_tag, crypto_err);
            }
            FEATURE_LOG_INFO("%s, result: %s", file_tag, result);
        }
    } else if (!check_any(options->data) && check_str(options->uri)) {
        result = digest_file(options->algo, options->uri, pkg_name);
        if (!result && crypto_err) {
            FEATURE_LOG_ERROR("%s %s, native digest_file error: %s", file_tag, crypto_err);
        }
    } else {
        FEATURE_LOG_ERROR("%s %s", file_tag, "arguments data and uri are only needed for one'");
    }

    return result;
}

void system_crypto_wrap_hmacDigest(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_HmacDigestParam* options)
{
    FEATURE_LOG_INFO("%s, options: %p", file_tag, options);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;
    if (!(check_str(options->data) && check_str(options->key))) {
        msg = "arguments data and key are needed";
        code = ARGSERROR;
    } else {
        result = digest(options->algo, (uint8_t*)(options->data), strlen(options->data), options->key);
        if (!result) {
            msg = crypto_err ? crypto_err : "digest error";
            code = GENERAL;
        }
        FEATURE_LOG_INFO("%s, result: %s", file_tag, result);
    }

    if (result && options->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "data", ret_data);
        INVOKE_SUCCESS_CB(options->success, (&ret_obj));
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, code);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    if (result)
        FeatureFreeValue(result);
}

void system_crypto_wrap_sign(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_SignParam* options)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;
    const char* algo = NULL;
    int seg_count;
    char** algo_segs = split_str(options->algo, "-", &seg_count);
    // set default value for algo and deal with value
    if (!check_str(options->algo)) {
        algo = "SHA256";
    } else if (has_type(sign_hash_types, arrayof(sign_hash_types), options->algo)) {
        algo = algo_segs[1];
    } else {
        msg = "invalid algo param!";
        code = ARGSERROR;
    }

    // excute native function
    bool is_text = true;
    size_t size = 0;
    if (!check_str(options->privateKey)) {
        msg = "arguments privateKey is needed";
        code = ARGSERROR;
    } else if (!(check_any(options->data) || check_str(options->uri))) {
        msg = "arguments data or uri is needed";
        code = ARGSERROR;
    } else if (check_any(options->data) && !check_str(options->uri)) {
        // judge data type
        uint8_t* buff = get_buff(ft_ctx, *(options->data), &size, &is_text);
        if (!buff || size == 0) {
            msg = "invalid data type!";
            code = ARGSERROR;
        } else {
            result = rsa_sign(algo, options->privateKey, buff, &size, &is_text);
            if (!result) {
                msg = crypto_err ? crypto_err : "rsa sign error";
                code = GENERAL;
            }
        }
    } else if (!check_any(options->data) && check_str(options->uri)) {
        result = rsa_sign_file(algo, options->privateKey, options->uri, pkg_name);
        if (!result) {
            msg = crypto_err ? crypto_err : "rsa sign file error";
            code = GENERAL;
        }
        is_text = true;
    } else {
        msg = "arguments data and uri are only needed for one";
        code = ARGSERROR;
    }
    FEATURE_LOG_INFO("%s, result: %p", file_tag, result);

    // deal with result
    if (result && options->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, result, size, is_text);
        ft_obj_set_property(ft_ctx, ret_obj, "data", ret_data);
        INVOKE_SUCCESS_CB(options->success, (&ret_obj));
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, code);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    free_str_array(algo_segs, seg_count);
    if (result)
        free(result);
}

void system_crypto_wrap_verify(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_RSAVerifyParam* options)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    bool result = false;
    const char* algo = NULL;
    int seg_count;
    char** algo_segs = split_str(options->algo, "-", &seg_count);
    // set default value for algo and deal with value
    if (!check_str(options->algo)) {
        algo = "SHA256";
    } else if (has_type(sign_hash_types, arrayof(sign_hash_types), options->algo)) {
        algo = algo_segs[1];
    } else {
        msg = "invalid algo param!";
        code = ARGSERROR;
    }

    size_t size = 0;
    size_t sig_size = 0;
    bool is_text = false;
    size_t has_result = false;
    // excute native function
    if (!(check_str(options->publicKey) && check_any(options->signature))) {
        msg = "arguments publicKey and signature are needed";
        code = ARGSERROR;
    } else if (!(check_any(options->data) || check_str(options->uri))) {
        msg = "arguments data or uri is needed";
        code = ARGSERROR;
    } else if (check_any(options->data) && !check_str(options->uri)) {
        // deal with data type of data param
        uint8_t* buff = get_buff(ft_ctx, *(options->data), &size, &is_text);
        if (!buff || size == 0) {
            msg = "data: invalid data type!";
            code = ARGSERROR;
        } else if (!check_any(options->signature)) {
            msg = "signature: invalid data type!";
            code = ARGSERROR;
        } else {
            uint8_t* sig_buff = get_buff(ft_ctx, *(options->signature), &sig_size, &is_text);
            result = rsa_verify(algo, options->publicKey, buff, size, sig_buff, sig_size, is_text);
            if (crypto_err) {
                msg = crypto_err;
                code = GENERAL;
            } else {
                has_result = true;
            }
        }
    } else if (!check_any(options->data) && check_str(options->uri)) {
        // deal with data type of signature
        if (!check_any(options->signature)) {
            msg = "signature: invalid data type!";
            code = ARGSERROR;
        } else {
            char* sig_buff = (char*)get_buff(ft_ctx, *(options->signature), &sig_size, &is_text);
            result = rsa_verify_file(algo, options->publicKey, options->uri, sig_buff, pkg_name);
            if (!crypto_err) {
                msg = crypto_err;
                code = GENERAL;
            } else {
                has_result = true;
            }
        }
    } else {
        msg = "arguments data and uri are only needed for one";
        code = ARGSERROR;
    }
    FEATURE_LOG_INFO("%s, result: %d", file_tag, result);

    // deal with result
    if (has_result && options->success) {
        INVOKE_SUCCESS_CB(options->success, result);
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, code);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    free_str_array(algo_segs, seg_count);
}

void system_crypto_wrap_encrypt(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_CryptParam* options)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;
    const char* algo = check_str(options->algo) ? options->algo : "RSA";
    bool is_text = true;
    size_t size = 0;

    const char* transformation = "";
    const char* iv = options->key;
    int ivOffset = 0;
    int ivLen = 16;
    int mode = 5; // encryptCfgs.AES.mode.CBC
    int padding = 0; // encryptCfgs.AES.padding.PKCS7Padding
    // excute native function
    if (!(check_any(options->data) && check_str(options->key))) {
        msg = "arguments data and key are needed";
        code = ARGSERROR;
    } else {
        // judge data type
        uint8_t* buff = get_buff(ft_ctx, *(options->data), &size, &is_text);
        if (!buff || size == 0) {
            msg = "invalid data type!";
            code = ARGSERROR;
        } else {
            if (strcmp(algo, "RSA") == 0) {
                result = rsa_encrypt(options->key, buff, &size, &is_text);
                if (!result) {
                    msg = crypto_err ? crypto_err : "rsa encrypt error";
                    code = GENERAL;
                }
            } else if (strcmp(algo, "AES") == 0) {
                // deal with default value of options
                if (options->options) {
                    system_crypto_MixinCryptOption* opts = options->options;
                    transformation = opts->transformation;
                    iv = opts->iv ? opts->iv : iv;
                    ivOffset = opts->ivOffset ? opts->ivOffset : ivOffset;
                    ivLen = opts->ivLen ? opts->ivLen : ivLen;
                }

                if (transformation) {
                    int seg_count;
                    char** cfg_keys = split_str(transformation, "/", &seg_count);
                    ft_value_t cfgs_json = ft_parse_json(ft_ctx, encryptCfgs, strlen(encryptCfgs), NULL);
                    ft_value_t ft_enc_type = ft_obj_get_property(ft_ctx, cfgs_json, cfg_keys[0]);
                    if (seg_count == 3 && ft_get_type(ft_ctx, ft_enc_type) != FT_TYPE_NONE) {
                        ft_value_t ft_mode = ft_obj_get_property(ft_ctx, ft_enc_type, "mode");
                        if (ft_get_type(ft_ctx, ft_mode) != FT_TYPE_NONE) {
                            ft_value_t ft_mode_val = ft_obj_get_property(ft_ctx, ft_mode, cfg_keys[1]);
                            int32_t int_val;
                            if (ft_to_int(ft_ctx, ft_mode_val, &int_val))
                                mode = int_val;
                        }
                        ft_value_t ft_padding = ft_obj_get_property(ft_ctx, ft_enc_type, "padding");
                        if (ft_get_type(ft_ctx, ft_padding) != FT_TYPE_NONE) {
                            ft_value_t ft_padding_val = ft_obj_get_property(ft_ctx, ft_padding, cfg_keys[2]);
                            int32_t int_val;
                            if (ft_to_int(ft_ctx, ft_padding_val, &int_val))
                                padding = int_val;
                        }
                    }
                    free_str_array(cfg_keys, seg_count);
                }

                result = aes_encrypt(mode, padding, options->key, iv, ivOffset, ivLen, buff, &size, &is_text);
                if (!result) {
                    msg = crypto_err ? crypto_err : "aes encrypt error";
                    code = GENERAL;
                }
            } else {
                msg = "invalid algo param";
                code = ARGSERROR;
            }
        }
        FEATURE_LOG_INFO("%s, result: %p", file_tag, result);
    }

    // deal with result
    if (result && options->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, result, size, is_text);
        ft_obj_set_property(ft_ctx, ret_obj, "data", ret_data);
        INVOKE_SUCCESS_CB(options->success, (&ret_obj));
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, code);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    if (result)
        free(result);
}

void system_crypto_wrap_decrypt(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_CryptParam* options)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;
    const char* algo = check_str(options->algo) ? options->algo : "RSA";
    bool is_text = true;
    size_t size = 0;

    const char* iv = options->key;
    int ivOffset = 0;
    int ivLen = 16;
    int mode = 5; // encryptCfgs.AES.mode.CBC
    int padding = 0; // encryptCfgs.AES.padding.PKCS7Padding
    // excute native function
    if (!(check_any(options->data) && check_str(options->key))) {
        msg = "arguments data and key are needed";
        code = ARGSERROR;
    } else {
        // judge data type
        uint8_t* buff = get_buff(ft_ctx, *(options->data), &size, &is_text);
        if (!buff || size == 0) {
            msg = "invalid data type!";
            code = ARGSERROR;
        } else {
            if (strcmp(algo, "RSA") == 0) {
                result = rsa_decrypt(options->key, buff, &size, &is_text);
                if (!result) {
                    msg = crypto_err ? crypto_err : "rsa encrypt error";
                    code = GENERAL;
                }
            } else if (strcmp(algo, "AES") == 0) {
                if (options->options) {
                    system_crypto_MixinCryptOption* opts = options->options;
                    iv = opts->iv ? opts->iv : iv;
                    ivOffset = opts->ivOffset ? opts->ivOffset : ivOffset;
                    ivLen = opts->ivLen ? opts->ivLen : ivLen;
                }
                result = aes_decrypt(mode, padding, options->key, iv, ivOffset, ivLen, buff, &size, &is_text);
                if (!result) {
                    msg = crypto_err ? crypto_err : "aes encrypt error";
                    code = GENERAL;
                }
            }
            FEATURE_LOG_INFO("%s, result: %p", file_tag, result);
        }
    }

    // deal with result
    if (result && options->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, result, size, is_text);
        ft_obj_set_property(ft_ctx, ret_obj, "data", ret_data);
        INVOKE_SUCCESS_CB(options->success, (&ret_obj));
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, code);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    if (result)
        free(result);
}

FtString system_crypto_wrap_btoa(FeatureInstanceHandle feature, AppendData append_data, FtString text)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    char* result = NULL;
    if (!check_str(text)) {
        FEATURE_LOG_ERROR("text param is needed!");
    } else {
        // excute native function
        result = base64("encrypt", text);
        if (!result) {
            FEATURE_LOG_ERROR("native base64 error: %s", crypto_err);
        }
        FEATURE_LOG_INFO("%s, wjf result: %s", file_tag, result);
    }

    return result;
}

FtString system_crypto_wrap_atob(FeatureInstanceHandle feature, AppendData append_data, FtString text)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    char* result = NULL;
    if (!check_str(text)) {
        FEATURE_LOG_ERROR("text param is needed!");
    } else {
        // excute native function
        result = base64("decrypt", text);
        if (!result) {
            FEATURE_LOG_ERROR("native base64 error: %s", crypto_err);
        }
        FEATURE_LOG_INFO("%s, result: %s", file_tag, result);
    }

    return result;
}
