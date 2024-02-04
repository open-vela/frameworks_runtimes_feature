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

typedef enum ErrorCode {
    GENERAL = 200,
    ARGSERROR = 202,
    IOERROR = 300,
    TIMEOUT = 204
} ErrorCode;

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

    if (!(check_str(opts->action) && check_str(opts->text) && check_str(opts->key))) {
        msg = "arguments action, text or key are needed";
        code = ARGSERROR;
    } else {
        size_t size = strlen(opts->text);
        bool is_text = true;
        if (strcmp(opts->action, "encrypt") == 0) {
            result = rsa_encrypt(opts->key, (uint8_t*)(opts->text), &size, &is_text);
            if (!result) {
                msg = crypto_err ? crypto_err : "rsa encrypt error";
                code = GENERAL;
            }
        } else if (strcmp(opts->action, "decrypt") == 0) {
            result = rsa_decrypt(opts->key, (uint8_t*)(opts->text), &size, &is_text);
            if (!result) {
                msg = crypto_err ? crypto_err : "rsa decrypt error";
                code = GENERAL;
            }
        } else {
            msg = "invalid action";
            code = ARGSERROR;
        }
    }

    if (result && opts->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "text", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }

    if (result)
        free(result);
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
        result = digest(opts->hashType, (uint8_t*)(opts->text), strlen(opts->text), NULL);
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
        result = digest("MD5", (uint8_t*)(opts->text), strlen(opts->text), NULL);
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
    char* result = NULL;

    if (!(check_str(opts->action) && check_str(opts->text) && check_str(opts->key))) {
        msg = "arguments action, text or key are needed";
        code = ARGSERROR;
    } else {
        const char* iv = check_str(opts->iv) ? opts->iv : opts->key;
        size_t ivOffset = opts->ivOffset ? opts->ivOffset : 0;
        size_t ivLen = opts->ivLen ? opts->ivLen : 16;

        size_t size = strlen(opts->text);
        bool is_text = true;
        if (ivOffset > strlen(iv)) {
            msg = "argument ivOffset shouldn\'t be larger than iv\'s length";
            code = ARGSERROR;
        } else if (strcmp(opts->action, "encrypt") == 0) {
            result = aes_encrypt(5, 0, opts->key, iv, ivOffset, ivLen, (uint8_t*)(opts->text), &size, &is_text);
            if (!result) {
                msg = crypto_err ? crypto_err : "aes encrypt error";
                code = GENERAL;
            }
        } else if (strcmp(opts->action, "decrypt") == 0) {
            result = aes_decrypt(5, 0, opts->key, iv, ivOffset, ivLen, (uint8_t*)(opts->text), &size, &is_text);
            if (!result) {
                msg = crypto_err ? crypto_err : "aes decrypt error";
                code = GENERAL;
            }
        } else {
            msg = "invalid action";
            code = ARGSERROR;
        }
    }

    // deal with result
    if (result && opts->success) {
        ft_value_t ret_data = ft_from_string(ft_ctx, result);
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_obj_set_property(ft_ctx, ret_obj, "text", ret_data);
        INVOKE_SUCCESS_CB(opts->success, (&ret_obj));
    } else if (opts->fail) {
        INVOKE_FAIL_CB(opts->fail, msg, code);
    }

    if (opts->complete) {
        INVOKE_COMPLET_CB(opts->complete);
    }

    if (result)
        free(result);
}
