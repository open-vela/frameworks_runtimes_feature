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
#include "trace_utils.h"

#include <alloca.h>
#include <stdarg.h>
#include <stdio.h>

static const char* file_tag = "[system_crypto_impl]";

static const char* pkg_name = NULL;

typedef enum {
    GOOD = 0,
    GENERAL = 200,
    ARGSERROR = 202,
    UNSUPPORTED = 203,
    TIMEOUT = 204,
    IOERROR = 300
} ErrorCode;

typedef enum {
    RSA,
    AES
} AlgoType;

typedef enum {
    NONE = 0,
    AES_128_ECB = 2,
    AES_192_ECB = 3,
    AES_256_ECB = 4,
    AES_128_CBC = 5,
    AES_192_CBC = 6,
    AES_256_CBC = 7,
    AES_128_CCM = 43,
    AES_192_CCM = 44,
    AES_256_CCM = 45
} ModeType;

/* PKCS7Padding : PKCS7 padding (default).        */
/* PKCS5Padding : PKCS5 padding.                  */
/* PADDING_ONE_AND_ZEROS: ISO/IEC 7816-4 padding. */
/* PADDING_ZEROS_AND_LEN: ANSI X.923 padding.     */
/* PADDING_ZEROS: Zero padding (not reversible).  */
/* PADDING_NONE: Never pad (full blocks only).    */
/* ISO10126Padding: unsupported now               */
typedef enum {
    PKCS7Padding = 0,
    PKCS5Padding = 0,
    PADDING_ONE_AND_ZEROS,
    PADDING_ZEROS_AND_LEN,
    PADDING_ZEROS,
    PADDING_NONE,
    ISO10126Padding,
    PKCS_1_V15 = 999,
} PaddingType;

typedef enum {
    MD5,
    SHA1,
    SHA256,
    SHA512
} HashType;

typedef enum {
    RSA_MD5,
    RSA_SHA1,
    RSA_SHA256,
    RSA_SHA512
} SignHashType;

typedef enum {
    AES_128 = 16,
    AES_192 = 24,
    AES_256 = 32,
    RSA_NO_NEED_CHECK_LENGTH_HERE,
} KeyLength;

typedef enum {
    MD5_LENGTH = 16,
    SHA1_LENGTH = 20,
    SHA256_LENGTH = 32,
    SHA512_LENGTH = 64
} HashLength;

typedef struct CipherCfg {
    const char* cfg;
    KeyLength key_size;
    AlgoType algo;
    ModeType mode;
    PaddingType padding;
} CipherCfg;

typedef struct CipherSupported {
    CipherCfg cfg_index;
    bool is_supported;
} CipherSupported;

static CipherSupported AESCipherSupported[] = {
    { { "AES/CBC/PKCS5Padding", AES_128, AES, AES_128_CBC, PKCS5Padding }, true },
    { { "AES/CBC/PKCS5Padding", AES_192, AES, AES_192_CBC, PKCS5Padding }, true },
    { { "AES/CBC/PKCS5Padding", AES_256, AES, AES_256_CBC, PKCS5Padding }, true },
    { { "AES/CBC/PKCS7Padding", AES_128, AES, AES_128_CBC, PKCS7Padding }, true },
    { { "AES/CBC/PKCS7Padding", AES_192, AES, AES_192_CBC, PKCS7Padding }, true },
    { { "AES/CBC/PKCS7Padding", AES_256, AES, AES_256_CBC, PKCS7Padding }, true },
    { { "AES/CBC/ZeroPadding", AES_128, AES, AES_128_CBC, PADDING_ZEROS }, false },
    { { "AES/CBC/NoPadding", AES_128, AES, AES_128_CBC, PADDING_NONE }, false },
    { { "AES/CBC/ISO10126Padding", AES_128, AES, AES_128_CBC, ISO10126Padding }, false },
    { { "AES/ECB/PKCS5Padding", AES_128, AES, AES_128_ECB, PKCS5Padding }, true },
    { { "AES/ECB/PKCS7Padding", AES_128, AES, AES_128_ECB, PKCS7Padding }, true },
    { { "AES/ECB/ZeroPadding", AES_128, AES, AES_128_CCM, PADDING_ZEROS }, false },
    { { "AES/ECB/NoPadding", AES_128, AES, AES_128_CCM, PADDING_NONE }, false },
    { { "AES/ECB/ISO10126Padding", AES_128, AES, AES_128_CCM, ISO10126Padding }, false },
    { { "AES/CCM/NoPadding", AES_128, AES, AES_128_CCM, PADDING_NONE }, true },
    { { "AES/CCM/NoPadding", AES_192, AES, AES_192_CCM, PADDING_NONE }, true },
    { { "AES/CCM/NoPadding", AES_256, AES, AES_256_CCM, PADDING_NONE }, true },
};

#define AES_SUPPORTED_COUNT sizeof(AESCipherSupported) / sizeof(AESCipherSupported[0])

static CipherSupported RSACipherSupported[] = {
    { { "RSA/None/PKCS1Padding", RSA_NO_NEED_CHECK_LENGTH_HERE, RSA, NONE, PADDING_NONE }, true },
    { { "RSA/None/PKCS1Padding", RSA_NO_NEED_CHECK_LENGTH_HERE, RSA, NONE, PADDING_NONE }, true },
    { { "RSA/None/PKCS1Padding", RSA_NO_NEED_CHECK_LENGTH_HERE, RSA, NONE, PADDING_NONE }, true },
    { { "RSA/None/PKCS1Padding", RSA_NO_NEED_CHECK_LENGTH_HERE, RSA, NONE, PADDING_NONE }, true },
    { { "RSA/None/PKCS1Padding", RSA_NO_NEED_CHECK_LENGTH_HERE, RSA, NONE, PADDING_NONE }, true },
    { { "RSA/None/PKCS1Padding", RSA_NO_NEED_CHECK_LENGTH_HERE, RSA, NONE, PADDING_NONE }, true },
};

#define RSA_SUPPORTED_COUNT sizeof(RSACipherSupported) / sizeof(RSACipherSupported[0])

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
typedef struct cipher_aes_default_value {
    ModeType mode;
    PaddingType padding;
    const unsigned char* iv;
    int ivOffset;
    int ivLen;
    const char* aad;
    int aad_len;
    int tagLen;
} cipher_aes_default_value;

cipher_aes_default_value aes_default_values = {
    AES_128_CBC,
    PKCS7Padding,
    NULL,
    0,
    0,
    NULL,
    0,
    4
};

static uint8_t* get_buff(ft_context_ref ft_ctx, ft_value_t data, size_t* size, bool* is_text)
{
    ft_type type = ft_get_type(ft_ctx, data);
    if (type == FT_TYPE_BUFFER || type == FT_TYPE_TYPED_BUFFER) {
        *is_text = false;
        uint8_t* buff = ft_to_buffer(ft_ctx, size, data);
        return buff;
    } else if (type == FT_TYPE_STRING) {
        const char* str = ft_to_string(ft_ctx, data);
        *is_text = true;
        *size = strlen(str);
        return (uint8_t*)str;
    }
    return NULL;
}

static ft_value_t from_buff(ft_context_ref ft_ctx, const char* data, size_t size, bool is_text)
{
    if (is_text)
        return ft_from_string(ft_ctx, data);

    return ft_from_typed_buffer(ft_ctx, (uint8_t*)data, size, 1);
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
            FEATURE_LOG_ERROR("%s %s", file_tag, "invalid data type!");
        } else {
            result = digest(options->algo, buff, size, NULL);
            if (!result && crypto_err) {
                FEATURE_LOG_ERROR("%s, native digest error: %s", file_tag, crypto_err);
            }
            FEATURE_LOG_INFO("%s, result: %s", file_tag, result);
        }
    } else if (!check_any(options->data) && check_str(options->uri)) {
        result = digest_file(options->algo, options->uri, pkg_name);
        if (!result && crypto_err) {
            FEATURE_LOG_ERROR("%s, native digest_file error: %s", file_tag, crypto_err);
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
        ft_free_value(ft_ctx, ret_obj);
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
        ft_free_value(ft_ctx, ret_obj);
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
            if (crypto_err) {
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

static bool is_valid_utf8(const unsigned char* data, size_t length)
{
    size_t i = 0;
    while (i <= length) {
        unsigned char c = data[i];

        if (c <= 0x7F) {
            i++;
            continue;
        }

        size_t bytes_needed = 0;
        if (c >= 0xC0 && c <= 0xDF)
            bytes_needed = 1;
        else if (c >= 0xE0 && c <= 0xEF)
            bytes_needed = 2;
        else if (c >= 0xF0 && c <= 0xF7)
            bytes_needed = 3;
        else {
            return false;
        }

        for (size_t j = 0; j < bytes_needed; j++) {
            if (i + j + 1 > length) {
                return false;
            }
            unsigned char follow_byte = data[i + j + 1];
            if (follow_byte < 0x80 || follow_byte > 0xBF) {
                return false;
            }
        }

        i += bytes_needed + 1;
    }

    return true;
}

static bool translate_string_and_uint8array_to_byte(ft_context_ref ft_ctx, ft_value_t input_key, uint8_t** output, size_t* output_size)
{
    size_t data_size;
    bool is_data_text;
    uint8_t* data = get_buff(ft_ctx, input_key, &data_size, &is_data_text);
    if (data == NULL || data_size == 0) {
        FEATURE_LOG_ERROR("invalid data type");
        return false;
    }

    // buff buffer should be big enough to hold the decoded data
    *output = (uint8_t*)malloc(data_size);
    if (*output == NULL) {
        FEATURE_LOG_ERROR("malloc output failed");
        return false;
    }

    // when decrypt, the buff should be base64 decoded
    if (is_data_text) {
        if (base64_decode((const char*)data, data_size, (char*)*output, data_size, output_size) != 0) {
            FEATURE_LOG_ERROR("base64 decode data failed");
            free(*output);
            *output = NULL;
            return false;
        }
    } else {
        memcpy(*output, data, data_size);
        *output_size = data_size;
    }

    return true;
}

static bool parse_transformation(CipherSupported entry[], int entry_count, char* transformation, size_t key_size, int* mode, int* padding)
{
    // example transformation : AES/CBC/PKCS5Padding
    for (int i = 0; i < entry_count; i++) {
        // check if the transformation is supported
        if (strncmp(entry[i].cfg_index.cfg, transformation, strlen(entry[i].cfg_index.cfg)) == 0 && entry[i].is_supported) {
            // AES neet to check if the key size is supported, then get mode and padding
            if (AES == entry[i].cfg_index.algo && key_size == entry[i].cfg_index.key_size) {
                if (mode && padding) {
                    *mode = entry[i].cfg_index.mode;
                    *padding = entry[i].cfg_index.padding;
                }
                return true;
            } else if (RSA == entry[i].cfg_index.algo) {
                // RSA don't need to those above, just check if supported.
                return true;
            }
        }
    }

    return false;
}

static bool parse_internal_options(ft_context_ref ft_ctx, system_crypto_CryptParam* options,
    int mode,
    uint8_t* key,
    const unsigned char** iv, int* ivOffset, size_t* ivLen,
    uint8_t** aad, size_t* aadLen,
    uint8_t** tag_input, size_t* tagLen,
    bool is_auth_crypto)
{
    system_crypto_MixinCryptOption* opts = options->options;
    bool is_process_ok = true;

    // get iv, ivOffset and ivLen
    // HACK for non-auth AES :
    // 1. need to set default iv equal to key which is bytes list, and set default ivLen equal to 16
    // 2. Now the key is already uint8Array
    if (check_str(opts->iv) && opts->ivLen) {
        *iv = (const unsigned char*)malloc(opts->ivLen);
        if (*iv == NULL) {
            FEATURE_LOG_ERROR("malloc iv failed");
            is_process_ok = false;
            goto free;
        }
        if (base64_decode((const char*)opts->iv, BASE64_ENCODED_LENGTH(opts->ivLen), (char*)*iv, opts->ivLen, ivLen) != 0) {
            FEATURE_LOG_ERROR("base64 decode iv failed");
            is_process_ok = false;
            goto free;
        }
    } else if (!check_str(opts->iv) && !opts->ivLen) {
        if (is_auth_crypto) {
            *iv = NULL;
            *ivLen = aes_default_values.ivLen;
        } else {
            *iv = key;
            *ivLen = 16;
        }
    } else {
        FEATURE_LOG_ERROR("missing iv or ivLen, they are both needed");
        is_process_ok = false;
        goto free;
    }

    // get ivOffset
    *ivOffset = opts->ivOffset ? opts->ivOffset : aes_default_values.ivOffset;

    // get aad, aadLen
    if (opts->aad && !translate_string_and_uint8array_to_byte(ft_ctx, *(opts->aad), aad, aadLen)) {
        FEATURE_LOG_ERROR("translate aad to bytes failed");
        is_process_ok = false;
        goto free;
    }

    // get tagLen, only encrypt need tagLen, so tagLen won't be update by following codes.
    if (opts->tagLen == 0) {
        FEATURE_LOG_ERROR(" tagLen can not be 0");
        is_process_ok = false;
        goto free;
    } else {
        *tagLen = opts->tagLen ? opts->tagLen : aes_default_values.tagLen;
    }

    // get tag, decrypt need both tag and tagLen, so update the above tagLen.
    if (opts->tag && !translate_string_and_uint8array_to_byte(ft_ctx, *(opts->tag), tag_input, tagLen)) {
        FEATURE_LOG_ERROR("translate tag to bytes failed");
        is_process_ok = false;
        goto free;
    }

free:
    /* something wrong happened when prase internal options */
    if (!is_process_ok) {
        if (*iv) {
            free((void*)*iv);
            *iv = NULL;
        }
        if (*aad) {
            free(*aad);
            *aad = NULL;
        }
        if (*tag_input) {
            free(*tag_input);
            *tag_input = NULL;
        }
    }

    return is_process_ok;
}

static bool parse_top_options(ft_context_ref ft_ctx, system_crypto_CryptParam* options,
    uint8_t** key, size_t* key_size,
    const char** algo,
    uint8_t** buff, size_t* buff_size,
    bool* is_buff_text,
    int operation)
{
    bool is_process_ok = true;
    // get algorithm type
    *algo = check_str(options->algo) ? (const char*)options->algo : "RSA";
    if (strcmp(*algo, "RSA") != 0 && strcmp(*algo, "AES") != 0) {
        FEATURE_LOG_ERROR("wrong algorithm cipher");
        is_process_ok = false;
        goto free;
    }

    // get key, if key is encoded with base64, translate it to uint8Array
    if (options->key) {
        size_t keyBuff_size;
        bool is_key_text;
        uint8_t* key_buff = get_buff(ft_ctx, *(options->key), &keyBuff_size, &is_key_text);
        if (key_buff == NULL || keyBuff_size == 0) {
            FEATURE_LOG_ERROR("invalid key type");
            is_process_ok = false;
            goto free;
        }

        // key buffer should be big enough to hold the decoded data
        // RSA key is the pem format, don't forget to preserve one byte for '\0'
        *key = (uint8_t*)malloc(keyBuff_size + 1);
        if (*key == NULL) {
            FEATURE_LOG_ERROR("malloc key failed");
            is_process_ok = false;
            goto free;
        }
        memset(*key, 0, keyBuff_size + 1);

        if (strcmp(*algo, "AES") == 0) {
            if (is_key_text) {
                if (base64_decode((const char*)key_buff, keyBuff_size, (char*)*key, keyBuff_size, key_size) != 0) {
                    FEATURE_LOG_ERROR("base64 decode key failed");
                    is_process_ok = false;
                    goto free;
                }
            } else {
                memcpy(*key, key_buff, keyBuff_size);
                *key_size = keyBuff_size;
            }
        } else if (strcmp(*algo, "RSA") == 0) {
            memcpy(*key, key_buff, keyBuff_size);
            *key_size = keyBuff_size;
        } else {
            FEATURE_LOG_ERROR("wrong algo");
            is_process_ok = false;
            goto free;
        }
    } else {
        FEATURE_LOG_ERROR("key is needed");
        is_process_ok = false;
        goto free;
    }

    // get buff, buff_size and is_text
    if (options->data) {
        size_t data_size;
        uint8_t* data = get_buff(ft_ctx, *(options->data), &data_size, is_buff_text);
        if (data == NULL || data_size == 0) {
            FEATURE_LOG_ERROR("invalid data type");
            is_process_ok = false;
            goto free;
        }

        // buff buffer should be big enough to hold the decoded data
        *buff = (uint8_t*)malloc(data_size);
        if (*buff == NULL) {
            FEATURE_LOG_ERROR("malloc buff failed");
            is_process_ok = false;
            goto free;
        }

        // when decrypt, the buff should be base64 decoded
        if (*is_buff_text && operation == DECRYPT_OPERATION) {
            if (base64_decode((const char*)data, data_size, (char*)*buff, data_size, buff_size) != 0) {
                FEATURE_LOG_ERROR("base64 decode data failed");
                is_process_ok = false;
                goto free;
            }
        } else {
            memcpy(*buff, data, data_size);
            *buff_size = data_size;
        }
    } else {
        FEATURE_LOG_ERROR("data is needed");
        is_process_ok = false;
        goto free;
    }

free:
    /* something wrong happened when prase top options */
    if (!is_process_ok) {
        if (*key) {
            free((void*)*key);
            *key = NULL;
        }
        if (*buff) {
            free(*buff);
            *buff = NULL;
        }
    }

    return is_process_ok;
}

static int system_crypto_operation_handle(FeatureInstanceHandle feature, system_crypto_CryptParam* options,
    uint8_t** result, size_t* result_size,
    uint8_t** tag_output, size_t* tagLen_output,
    bool* is_auth_crypto, bool* is_buff_text,
    const char** msg,
    int operation)
{
    uint8_t* key = NULL;
    size_t key_size = 0;
    const char* algo = NULL;
    uint8_t* buff = NULL;
    size_t buff_size = 0;
    uint8_t* output = NULL;
    size_t output_size = 0;
    size_t out_size = 0;
    size_t tagLen_input = 0;
    uint8_t* tag_input = NULL;
    int mode = 0;
    int padding = 0;
    const unsigned char* iv = NULL;
    int ivOffset = 0;
    size_t ivLen = 0;
    uint8_t* aad = NULL;
    size_t aadLen = 0;
    ErrorCode ret = GOOD;

    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    // get key, algorithm type, buff, buff_size, is_buff_text
    if (!parse_top_options(ft_ctx, options, &key, &key_size, &algo, &buff, &buff_size, is_buff_text, operation)) {
        *msg = "wrong top options, please check arguments";
        ret = ARGSERROR;
        return ret;
    }

    if (strcmp(algo, "RSA") == 0) {
        // if transformation is exist, check if transformation is supported.
        // check the result since transformation represents what the caller expects.
        if (options->options) {
            if (check_str(options->options->transformation)) {
                if (!parse_transformation(RSACipherSupported, RSA_SUPPORTED_COUNT, (char*)options->options->transformation, key_size, NULL, NULL)) {
                    FEATURE_LOG_ERROR("unsupported RSA cipher");
                    *msg = "unsupported algorithm cipher, please check transformation";
                    ret = UNSUPPORTED;
                    goto free;
                }
            }
        }

        // excute native function
        if (rsa_crypto(key, buff, buff_size, &output, &out_size, operation) != 0) {
            FEATURE_LOG_ERROR("rsa_crypto failed");
            *msg = "rsa_crypto failed";
            ret = GENERAL;
            goto free;
        }
        ret = GOOD;
    } else if (strcmp(algo, "AES") == 0) {
        // get mode and padding
        mode = aes_default_values.mode;
        padding = aes_default_values.padding;

        // if transformation is not null, parse it to update mode and padding
        // check the result since transformation represents what the caller expects.
        if (options->options) {
            if (check_str(options->options->transformation)) {
                if (!parse_transformation(AESCipherSupported, AES_SUPPORTED_COUNT, (char*)options->options->transformation, key_size, &mode, &padding)) {
                    FEATURE_LOG_ERROR("unsupported AES cipher");
                    *msg = "unsupported algorithm cipher, please check transformation";
                    ret = UNSUPPORTED;
                    goto free;
                }
            }
        }

        if (mode == AES_128_CCM || mode == AES_192_CCM || mode == AES_256_CCM) {
            *is_auth_crypto = true;
        } else {
            *is_auth_crypto = false;
        }

        // parse internal options to choose if use default value or not.
        if (!parse_internal_options(ft_ctx, options, mode, key, &iv, &ivOffset, &ivLen, &aad, &aadLen, &tag_input, &tagLen_input, *is_auth_crypto)) {
            FEATURE_LOG_ERROR("wrong internal options, please check arguments");
            *msg = "wrong internal options, please check arguments";
            ret = ARGSERROR;
            goto free;
        }

        // excute native function
        // if aes ccm
        if (*is_auth_crypto) {
            // NOTE : 'output_size' is the size of the 'output'
            //        'output' should be big enough to hold the encrypted data with padding data.
            //        'out_size' is less than or equal to 'output_size'
            output_size = ROUND_UP(buff_size + tagLen_input + 1, key_size);
            output = (unsigned char*)malloc(output_size);
            if (!output) {
                FEATURE_LOG_ERROR("malloc output failed");
                *msg = "malloc output failed";
                ret = GENERAL;
                goto free;
            }
            memset(output, 0, output_size);

            if (aes_auth_crypto(mode,
                    padding,
                    (const uint8_t*)key, key_size,
                    (const uint8_t*)iv, ivLen,
                    aad, aadLen,
                    tag_input, tagLen_input,
                    (const uint8_t*)buff, buff_size,
                    output, output_size,
                    &out_size,
                    operation)
                != 0) {
                *msg = "aes auth crypto error";
                ret = GENERAL;
                goto free;
            }
        } else {
            // if not aes ccm
            //  NOTE : 'output_size' is the size of the 'output'
            //         'output' should be big enough to hold the encrypted data with padding data.
            output_size = ROUND_UP(buff_size + 1, key_size);
            output = (unsigned char*)malloc(output_size);
            if (!output) {
                FEATURE_LOG_ERROR("malloc output failed");
                *msg = "malloc output failed";
                ret = GENERAL;
                goto free;
            }
            memset(output, 0, output_size);

            if (aes_non_auth_crypto(mode,
                    padding,
                    (const uint8_t*)key, key_size,
                    (const uint8_t*)iv, ivOffset, ivLen,
                    (const uint8_t*)buff, buff_size,
                    output, &out_size,
                    operation)
                != 0) {
                *msg = "aes non auth crypto error";
                ret = GENERAL;
                goto free;
            }
        }
    } else {
        FEATURE_LOG_ERROR("invalid algo param");
        *msg = "invalid algo param";
        ret = ARGSERROR;
        goto free;
    }

    // make sure the if-else is clear
    // if is_buff_text is true, the encrypt result should be base64 encoded, the decrypt result should be UTF-8 encoded.
    if (*is_buff_text == true) {
        // handle the encrypt operation's result
        if (operation == ENCRYPT_OPERATION) {
            // the result buffer should be big enough to hold the encrypted data with padding data.
            *result = (uint8_t*)malloc(BASE64_ENCODED_LENGTH(out_size) + 1);
            if (*result == NULL) {
                FEATURE_LOG_ERROR("malloc result failed");
                *msg = "malloc result failed";
                ret = GENERAL;
                goto free;
            }
            memset(*result, 0, BASE64_ENCODED_LENGTH(out_size) + 1);
            if (*is_auth_crypto == true) {
                // when aes-ccm encryption , need to depart the data and tag
                // base64 encode the encrypted data
                if (base64_encode((const char*)output, out_size - tagLen_input, (char*)*result, BASE64_ENCODED_LENGTH(out_size) + 1, result_size) != 0) {
                    FEATURE_LOG_ERROR("base64 encode result failed");
                    *msg = "base64 encode result failed";
                    ret = GENERAL;
                    goto free;
                }

                *tag_output = (uint8_t*)malloc(BASE64_ENCODED_LENGTH(tagLen_input) + 1);
                if (*tag_output == NULL) {
                    FEATURE_LOG_ERROR("malloc tag_output failed");
                    *msg = "malloc tag_output failed";
                    ret = GENERAL;
                    goto free;
                }
                memset(*tag_output, 0, BASE64_ENCODED_LENGTH(tagLen_input) + 1);

                // base64 encode the tag data
                if (base64_encode((const char*)output + out_size - tagLen_input, tagLen_input, (char*)*tag_output, BASE64_ENCODED_LENGTH(tagLen_input) + 1, tagLen_output) != 0) {
                    FEATURE_LOG_ERROR("base64 encode tag_output failed");
                    *msg = "base64 encode tag_output failed";
                    ret = GENERAL;
                    goto free;
                }
            } else if (*is_auth_crypto == false) {
                // when not aes-ccm encryption, just return base64 encode the encrypted data
                if (base64_encode((const char*)output, out_size, (char*)*result, BASE64_ENCODED_LENGTH(out_size) + 1, result_size) != 0) {
                    FEATURE_LOG_ERROR("base64 encode result failed");
                    *msg = "base64 encode result failed";
                    ret = GENERAL;
                    goto free;
                }
            }
            // handle the decrypt operation's result
            // check the result if is belong to utf-8, then return.
        } else if (operation == DECRYPT_OPERATION) {
            if (is_valid_utf8((const unsigned char*)output, out_size) == false) {
                FEATURE_LOG_ERROR("the decrypt result is not utf-8");
                *msg = "the decrypt result is not utf-8";
                ret = GENERAL;
                goto free;
            }
            *result = output;
            *result_size = out_size;
        }
        // when is_buff_text is false, just return the encrypted data
    } else if (*is_buff_text == false) {
        if (operation == ENCRYPT_OPERATION) {
            // when aes-ccm encryption , need to depart the data and tag
            if (*is_auth_crypto == true) {
                *result = output;
                *result_size = out_size - tagLen_input;
                *tag_output = output + out_size - tagLen_input;
                *tagLen_output = tagLen_input;
            } else if (*is_auth_crypto == false) {
                *result = output;
                *result_size = out_size;
            }
        } else if (operation == DECRYPT_OPERATION) {
            *result = output;
            *result_size = out_size;
        }
    }

free:
    if (ret != GOOD) {
        if (output) {
            free(output);
            output = NULL;
        }
        if (*result) {
            free(*result);
            *result = NULL;
        }
        if (*tag_output) {
            free(*tag_output);
            *tag_output = NULL;
        }
    }

    if (options->options) {
        if (options->options->aad && aad) {
            free(aad);
            aad = NULL;
        }
        if (options->options->tag && tag_input) {
            free(tag_input);
            tag_input = NULL;
        }
        if (check_str(options->options->iv) && options->options->ivLen) {
            free((void*)iv);
            iv = NULL;
        }
    }

    return ret;
}

void system_crypto_wrap_encrypt(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_CryptParam* options)
{
    if (!options) {
        FEATURE_LOG_ERROR("%s options is null!", file_tag);
        return;
    }
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);
    uint8_t* result = NULL;
    size_t result_size = 0;
    uint8_t* tag_output = NULL;
    size_t tagLen_output = 0;
    bool is_buff_text = false;
    bool is_auth_crypto = false;
    int operation = ENCRYPT_OPERATION;
    const char* msg = "";
    int ret = 0;

    ret = system_crypto_operation_handle(feature, options, &result, &result_size, &tag_output, &tagLen_output, &is_auth_crypto, &is_buff_text, &msg, operation);
    if (ret != 0) {
        FEATURE_LOG_ERROR("system_crypto_operation_handle failed, operation : %d \n", operation);
    }

    // deal with result
    if (result && options->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, (const char*)result, result_size, is_buff_text);
        ft_obj_set_property(ft_ctx, ret_obj, "data", ret_data);
        ft_value_t ret_tag;
        if (is_auth_crypto) {
            ret_tag = from_buff(ft_ctx, (const char*)tag_output, tagLen_output, is_buff_text);
            ft_obj_set_property(ft_ctx, ret_obj, "tag", ret_tag);
        }
        INVOKE_SUCCESS_CB(options->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, ret);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    if (ret == GOOD && is_buff_text && tag_output) {
        free(tag_output);
        tag_output = NULL;
    }

    if (ret == GOOD && result) {
        free(result);
        result = NULL;
    }
}

void system_crypto_wrap_decrypt(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_CryptParam* options)
{
    if (!options) {
        FEATURE_LOG_ERROR("%s options is null!", file_tag);
        return;
    }
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);
    uint8_t* result = NULL;
    size_t result_size = 0;
    uint8_t* tag_output = NULL;
    size_t tagLen_output = 0;
    bool is_buff_text = false;
    bool is_auth_crypto = false;
    int operation = DECRYPT_OPERATION;
    const char* msg = "";

    int ret = 0;
    ret = system_crypto_operation_handle(feature, options, &result, &result_size, &tag_output, &tagLen_output, &is_auth_crypto, &is_buff_text, &msg, operation);
    if (ret != 0) {
        FEATURE_LOG_ERROR("system_crypto_operation_handle failed, operation : %d \n", operation);
    }

    // deal with result
    if (result && options->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, (const char*)result, result_size, is_buff_text);
        ft_obj_set_property(ft_ctx, ret_obj, "data", ret_data);
        INVOKE_SUCCESS_CB(options->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, ret);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    if (ret == GOOD && result) {
        free(result);
        result = NULL;
    }
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
