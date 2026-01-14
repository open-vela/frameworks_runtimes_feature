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

#include "cipher.h"

#include "feature_log.h"
#include "feature_utils.h"

#include "crypto_native.h"
#include "crypto_utils.h"

#include <alloca.h>
#include <stdarg.h>
#include <stdio.h>

static const char* file_tag = "[system_cipher_impl]";

static const char* pkg_name = NULL;

// FeatureCallbacks
void system_cipher_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s\n", file_tag);
}

void system_cipher_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s\n", file_tag);
    pkg_name = FeatureGetPackageName(handle);
}

void system_cipher_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s\n", file_tag);
}

void system_cipher_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("%s\n", file_tag);
}

void system_cipher_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("%s\n", file_tag);
}

void system_cipher_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("%s\n", file_tag);
}

void system_cipher_wrap_rsa(FeatureInstanceHandle feature, AppendData append_data, system_cipher_RSAParam* opts)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;
    size_t result_size = 0;
    int operation = DECRYPT_OPERATION;
    uint8_t* buff = NULL;
    size_t buff_size = 0;
    uint8_t* key = NULL;
    uint8_t* output = NULL;
    size_t output_size = 0;

    // get operation
    if (check_str(opts->action) && strcmp(opts->action, "encrypt") == 0) {
        operation = ENCRYPT_OPERATION; // encrypt
    } else if (check_str(opts->action) && strcmp(opts->action, "decrypt") == 0) {
        operation = DECRYPT_OPERATION; // decrypt
    } else {
        FEATURE_LOG_ERROR("invalid action");
        msg = "invalid action";
        code = ARGSERROR;
        goto exit;
    }

    // get buff's bytes buff_size and is_text
    if (check_str(opts->text) && operation == DECRYPT_OPERATION) {
        // when decrypt, the buff should be base64 decoded
        // buff buffer should be big enough to hold the decoded data
        buff = (uint8_t*)malloc(strlen((const char*)opts->text));
        if (buff == NULL) {
            FEATURE_LOG_ERROR("malloc buff failed");
            msg = "malloc buff failed";
            goto exit;
        }
        if (base64_decode((const char*)opts->text, strlen(opts->text), (char*)buff, strlen((const char*)opts->text), &buff_size) != 0) {
            FEATURE_LOG_ERROR("base64 decode data failed");
            msg = "base64 decode data failed";
            goto exit;
        }
    } else if (check_str(opts->text) && operation == ENCRYPT_OPERATION) {
        buff = (uint8_t*)opts->text;
        buff_size = strlen((const char*)opts->text);
    } else {
        FEATURE_LOG_ERROR("wrong text param");
        msg = "wrong text param";
        goto exit;
    }

    if (check_str(opts->key)) {
        key = (uint8_t*)opts->key;
    } else {
        FEATURE_LOG_ERROR("wrong text param");
        msg = "wrong text param";
        goto exit;
    }

    // excute native function
    if (rsa_crypto(key, buff, buff_size, &output, &output_size, operation) != 0) {
        FEATURE_LOG_ERROR("rsa_crypto failed");
        msg = "rsa_crypto failed";
        code = GENERAL;
        goto exit;
    }

    // if encrypt, encoding output
    if (operation == ENCRYPT_OPERATION) {
        result = (char*)malloc(BASE64_ENCODED_LENGTH(output_size) + 1);
        if (result == NULL) {
            FEATURE_LOG_ERROR("malloc result failed");
            msg = "malloc result failed";
            code = GENERAL;
            goto exit;
        }

        if (base64_encode((const char*)output, output_size, result, BASE64_ENCODED_LENGTH(output_size) + 1, &result_size) != 0) {
            FEATURE_LOG_ERROR("base64 encode result failed");
            msg = "base64 encode result failed";
            code = GENERAL;
            goto exit;
        }
    } else {
        result = (char*)output;
    }

exit:
    if (result && opts->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, (const char*)result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "text", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }

    if (result) {
        free(result);
    }

    if (opts->text && operation == DECRYPT_OPERATION && buff) {
        free(buff);
    }
}

void system_cipher_wrap_sign(FeatureInstanceHandle feature, AppendData append_data, system_cipher_RSAParam* opts)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;

    if (!(check_str(opts->hashType) && check_str(opts->text) && check_str(opts->key))) {
        msg = "arguments hashType, text or key are needed";
        code = ARGSERROR;
    } else {
        size_t size = strlen(opts->text);
        bool is_text = true;
        result = rsa_sign(opts->hashType, opts->key, (uint8_t*)(opts->text), &size, &is_text);
        if (!result) {
            msg = crypto_err ? crypto_err : "rsa sign error";
            code = GENERAL;
        }
    }

    if (result && opts->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "text", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }

    if (result)
        free(result);
}

void system_cipher_wrap_verify(FeatureInstanceHandle feature, AppendData append_data, system_cipher_RSAVerifyParam* opts)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    FtBool result = false;
    size_t has_result = false;

    if (!(check_str(opts->hashType) && check_str(opts->signature)
            && check_str(opts->text) && check_str(opts->key))) {
        msg = "arguments hashType, signature, text or key are needed";
        code = ARGSERROR;
    } else {
        size_t size = strlen(opts->text);
        size_t sig_size = strlen(opts->signature);
        result = rsa_verify(opts->hashType, opts->key, (uint8_t*)(opts->text), size, (uint8_t*)(opts->signature), sig_size, true);
        if (crypto_err) {
            msg = crypto_err;
            code = GENERAL;
        } else {
            has_result = true;
        }
    }

    if (has_result && opts->success) {
        ft_value_t ret_data = ft_from_bool(ft_ctx, result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "valid", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }
}

void system_cipher_wrap_digest(FeatureInstanceHandle feature, AppendData append_data, system_cipher_DigestParam* opts)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;

    if (!(check_str(opts->hashType) && check_str(opts->text))) {
        msg = "arguments hashtype or text are needed";
        code = ARGSERROR;
    } else {
        result = digest(opts->hashType, (uint8_t*)(opts->text), strlen(opts->text), NULL, 0);
        if (!result && crypto_err) {
            msg = crypto_err;
            code = GENERAL;
        }
    }

    // deal with result
    if (result && opts->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "text", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }

    if (result)
        FeatureFreeValue(result);
}

void system_cipher_wrap_md5(FeatureInstanceHandle feature, AppendData append_data, system_cipher_Md5Param* opts)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;

    if (!check_str(opts->text)) {
        msg = "argument text is needed";
        code = ARGSERROR;
    } else {
        result = digest("MD5", (uint8_t*)(opts->text), strlen(opts->text), NULL, 0);
        if (!result && crypto_err) {
            msg = crypto_err;
            code = GENERAL;
        }
    }

    // deal with result
    if (result && opts->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "text", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }

    if (result)
        FeatureFreeValue(result);
}

void system_cipher_wrap_aes(FeatureInstanceHandle feature, AppendData append_data, system_cipher_AESParam* opts)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    uint8_t* key = NULL;
    size_t key_size = 0;
    uint8_t* buff = NULL;
    size_t buff_size = 0;
    int operation = ENCRYPT_OPERATION;
    uint8_t* iv = NULL;
    size_t ivLen = 0;
    size_t ivOffset = 0;
    uint8_t* output = NULL;
    size_t output_size = 0;
    uint8_t* result = NULL;
    size_t result_size = 0;
    int padding = 0;
    int mode = 0;

    // get operation
    if (check_str(opts->action) && strcmp(opts->action, "encrypt") == 0) {
        operation = ENCRYPT_OPERATION; // encrypt
    } else if (check_str(opts->action) && strcmp(opts->action, "decrypt") == 0) {
        operation = DECRYPT_OPERATION; // decrypt
    } else {
        FEATURE_LOG_ERROR("invalid action");
        msg = "invalid action";
        code = ARGSERROR;
        goto exit;
    }

    // get buff's bytes buff_size and is_text
    if (check_str(opts->text) && operation == DECRYPT_OPERATION) {
        // when decrypt, the buff should be base64 decoded
        // buff buffer should be big enough to hold the decoded data
        buff = (uint8_t*)malloc(strlen((const char*)opts->text));
        if (buff == NULL) {
            FEATURE_LOG_ERROR("malloc buff failed");
            msg = "malloc buff failed";
            goto exit;
        }
        if (base64_decode((const char*)opts->text, strlen(opts->text), (char*)buff, strlen((const char*)opts->text), &buff_size) != 0) {
            FEATURE_LOG_ERROR("base64 decode data failed");
            msg = "base64 decode data failed";
            goto exit;
        }
    } else if (check_str(opts->text) && operation == ENCRYPT_OPERATION) {
        buff = (uint8_t*)opts->text;
        buff_size = strlen((const char*)opts->text);
    } else {
        FEATURE_LOG_ERROR("wrong text param");
        msg = "wrong text param";
        goto exit;
    }

    // get key's bytes
    if (check_str(opts->key)) {
        // buff buffer should be big enough to hold the decoded data
        key = (uint8_t*)malloc(strlen((const char*)opts->key));
        if (key == NULL) {
            FEATURE_LOG_ERROR("malloc key failed");
            msg = "malloc key failed";
            goto exit;
        }

        if (base64_decode((const char*)opts->key, strlen((const char*)opts->key), (char*)key, strlen((const char*)opts->key), &key_size) != 0) {
            FEATURE_LOG_ERROR("base64 decode key failed");
            msg = "base64 decode key failed";
            goto exit;
        }
    } else {
        FEATURE_LOG_ERROR("invalid key");
        msg = "invalid key";
        code = ARGSERROR;
        goto exit;
    }

    // get padding and mode
    if (key_size == 16) {
        mode = 5; // AES_128_CBC
    } else if (key_size == 32) {
        mode = 7; // AES_256_CBC
    } else {
        FEATURE_LOG_ERROR("invalid key size");
        msg = "invalid key size";
        code = ARGSERROR;
        goto exit;
    }
    padding = 0; // PKCS5Padding

    // get iv
    if (check_str(opts->iv)) {
        // buff buffer should be big enough to hold the decoded data
        iv = (uint8_t*)malloc(strlen((const char*)opts->iv));
        if (iv == NULL) {
            FEATURE_LOG_ERROR("malloc iv failed");
            msg = "malloc iv failed";
            code = ARGSERROR;
            goto exit;
        }

        if (base64_decode((const char*)opts->iv, strlen((const char*)opts->iv), (char*)iv, strlen((const char*)opts->iv), &ivLen) != 0) {
            FEATURE_LOG_ERROR("base64 decode iv failed");
            msg = "base64 decode iv failed";
            code = ARGSERROR;
            goto exit;
        }
    } else {
        // use key bytes as default iv, use 16 as default iv length.
        ivLen = 16;
        iv = key;
    }

    // if opts hasa ivLen, update it.
    ivLen = opts->ivLen ? opts->ivLen : ivLen;

    // get ivOffset
    ivOffset = opts->ivOffset ? opts->ivOffset : 0;

    output_size = ROUND_UP(buff_size + 1, key_size);
    output = (unsigned char*)malloc(output_size);
    if (!output) {
        FEATURE_LOG_ERROR("malloc output failed");
        msg = "malloc output failed";
        code = GENERAL;
        goto exit;
    }

    if (aes_non_auth_crypto(mode,
            padding,
            (const uint8_t*)key, key_size,
            iv, ivOffset, ivLen,
            (const uint8_t*)buff, buff_size,
            output, &output_size,
            operation)
        != 0) {
        FEATURE_LOG_ERROR("aes_non_auth_crypto failed");
        msg = "aes non auth crypto error";
        code = GENERAL;
        goto exit;
    }

    // if encrypt, encoding output
    if (operation == ENCRYPT_OPERATION) {
        result = (uint8_t*)malloc(BASE64_ENCODED_LENGTH(output_size) + 1);
        if (result == NULL) {
            FEATURE_LOG_ERROR("malloc result failed");
            msg = "malloc result failed";
            code = GENERAL;
            goto exit;
        }

        if (base64_encode((const char*)output, output_size, (char*)result, BASE64_ENCODED_LENGTH(output_size) + 1, &result_size) != 0) {
            FEATURE_LOG_ERROR("base64 encode result failed");
            msg = "base64 encode result failed";
            code = GENERAL;
            goto exit;
        }
    } else {
        result = (uint8_t*)malloc(output_size);
        if (result == NULL) {
            FEATURE_LOG_ERROR("malloc result failed");
            msg = "malloc result failed";
            code = GENERAL;
            goto exit;
        }
        memcpy(result, output, output_size + 1);
    }

exit:
    // deal with result
    if (result && opts->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, (const char*)result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "text", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }

    if (output) {
        free(output);
    }

    if (result) {
        free(result);
    }

    if (opts->key && key) {
        free(key);
    }

    if (opts->text && operation == DECRYPT_OPERATION && buff) {
        free(buff);
    }

    if (opts->iv && iv) {
        free(iv);
    }
}
