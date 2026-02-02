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
    { { "AES/CBC/NoPadding", AES_128, AES, AES_128_CBC, PADDING_NONE }, true },
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
typedef enum {
    ECP_DP_NONE = 0, /*!< Curve not defined. */
    ECP_DP_SECP192R1, /*!< Domain parameters for the 192-bit curve defined by FIPS 186-4 and SEC1. */
    ECP_DP_SECP224R1, /*!< Domain parameters for the 224-bit curve defined by FIPS 186-4 and SEC1. */
    ECP_DP_SECP256R1, /*!< Domain parameters for the 256-bit curve defined by FIPS 186-4 and SEC1. */
    ECP_DP_SECP384R1, /*!< Domain parameters for the 384-bit curve defined by FIPS 186-4 and SEC1. */
    ECP_DP_SECP521R1, /*!< Domain parameters for the 521-bit curve defined by FIPS 186-4 and SEC1. */
    ECP_DP_BP256R1, /*!< Domain parameters for 256-bit Brainpool curve. */
    ECP_DP_BP384R1, /*!< Domain parameters for 384-bit Brainpool curve. */
    ECP_DP_BP512R1, /*!< Domain parameters for 512-bit Brainpool curve. */
    ECP_DP_CURVE25519, /*!< Domain parameters for Curve25519. */
    ECP_DP_SECP192K1, /*!< Domain parameters for 192-bit "Koblitz" curve. */
    ECP_DP_SECP224K1, /*!< Domain parameters for 224-bit "Koblitz" curve. */
    ECP_DP_SECP256K1, /*!< Domain parameters for 256-bit "Koblitz" curve. */
    ECP_DP_CURVE448, /*!< Domain parameters for Curve448. */
} ecp_group_id;

typedef struct ECDHCfg {
    const char* curve;
    int group_id;
    size_t privateKey_size;
    size_t publicKey_size;
} ECDHCfg;

typedef struct ECDHSupported {
    ECDHCfg cfg_index;
    bool is_supported;
} ECDHSupported;

static ECDHSupported ECDHCfgSupported[] = {
    { { "secp192r1", ECP_DP_SECP192R1, 0, 0 }, false },
    { { "secp224r1", ECP_DP_SECP224R1, 0, 0 }, false },
    { { "secp256r1", ECP_DP_SECP256R1, 32, 65 }, true },
    { { "secp384r1", ECP_DP_SECP384R1, 0, 0 }, false },
    { { "secp521r1", ECP_DP_SECP521R1, 0, 0 }, false },
    { { "bp256r1", ECP_DP_BP256R1, 0, 0 }, false },
    { { "bp384r1", ECP_DP_BP384R1, 0, 0 }, false },
    { { "bp512r1", ECP_DP_BP512R1, 0, 0 }, false },
    { { "curve25519", ECP_DP_CURVE25519, 0, 0 }, false },
    { { "secp192k1", ECP_DP_SECP192K1, 0, 0 }, false },
    { { "secp224k1", ECP_DP_SECP224K1, 0, 0 }, false },
    { { "secp256k1", ECP_DP_SECP256K1, 0, 0 }, false },
    { { "curve448", ECP_DP_CURVE448, 0, 0 }, false },
};

#define ECDH_SUPPORTED_COUNT sizeof(ECDHCfgSupported) / sizeof(ECDHCfgSupported[0])

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
    16,
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

    return ft_from_typed_buffer(ft_ctx, (uint8_t*)data, size, FT_Uint8Array);
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

    const char* algo = options->algo;
    FtString result = NULL;
    // set default value SHA256
    if (!check_str(algo)) {
        algo = hash_types[SHA256];
    } else if (!has_type(hash_types, arrayof(hash_types), algo)) {
        FEATURE_LOG_ERROR("%s invalid algo param: %s", file_tag, algo);
        return NULL;
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
            result = digest(algo, buff, size, NULL, 0);
            if (!result && crypto_err) {
                FEATURE_LOG_ERROR("%s, native digest error: %s", file_tag, crypto_err);
            }
            FEATURE_LOG_DEBUG("%s, result: %s", file_tag, result);
            // Note: Do not free buff here!
            // When data is Uint8Array/ArrayBuffer, buff points to JS engine's internal memory
            // which is managed by JS GC, not by malloc/free.
            // Only string data might need special handling, but ft_to_string returns
            // a pointer managed by JS engine as well, so no manual free is needed.
        }
    } else if (!check_any(options->data) && check_str(options->uri)) {
        result = digest_file(algo, options->uri, pkg_name);
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
    if (options == NULL) {
        FEATURE_LOG_ERROR("%s options is NULL", file_tag);
        return;
    }

    FEATURE_LOG_INFO("%s, options: %p", file_tag, options);
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* msg = "";
    int code = 0;
    char* result = NULL;
    const char* algo = options->algo;

    // set default value SHA256
    bool algo_valid = true;
    if (!check_str(algo)) {
        algo = hash_types[SHA256];
    } else if (!has_type(hash_types, arrayof(hash_types), algo)) {
        FEATURE_LOG_ERROR("%s invalid algo param: %s", file_tag, algo);
        msg = "invalid algo param";
        code = ARGSERROR;
        algo_valid = false;
    }

    if (algo_valid) {
        if (!(check_str(options->data) && check_str(options->key))) {
            msg = "arguments data and key are needed";
            code = ARGSERROR;
        } else {
            result = digest(algo, (uint8_t*)(options->data), strlen(options->data), options->key, strlen(options->key));
            if (!result) {
                msg = crypto_err ? crypto_err : "digest error";
                code = GENERAL;
            } else {
                FEATURE_LOG_INFO("%s, result: %s", file_tag, result);
            }
        }
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
    REMOVE_ALL_CBS(options);

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
    const char* algo = options->algo;
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
    REMOVE_ALL_CBS(options);

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
    const char* algo = options->algo;
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
    REMOVE_ALL_CBS(options);

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
    uint8_t* key, size_t key_len,
    const unsigned char** iv, int* ivOffset, size_t* ivLen,
    uint8_t** aad, size_t* aadLen,
    uint8_t** tag_input, size_t* tagLen,
    bool is_auth_crypto)
{
    system_crypto_MixinCryptOption* opts = options->options;
    bool is_process_ok = true;

    // get iv, ivOffset and ivLen
    // Rules implemented:
    // - `iv` (if provided) must be a base64-encoded STRING and will be decoded.
    // - `ivLen` (if provided) must be a non-negative integer and represents the decoded byte length.
    // - For non-auth AES (is_auth_crypto == false):
    //     * if iv provided -> decode and use it
    //     * if iv not provided -> default iv to first `ivLen` bytes of `key` (default ivLen = 16)
    // - For auth AES (is_auth_crypto == true):
    //     * if iv provided -> decode and use it
    //     * if iv not provided -> iv == NULL and ivLen == 0
    // it’s extremely weird and problematic for both developers and callers, but we have to follow the shits for compatibility.

    *iv = NULL;
    *ivLen = 0;

    // iv provided: must be a string (base64 encoded)
    if (check_str(opts->iv)) {
        // ensure ivLen if provided is not negative
        if (opts->ivLen < 0) {
            FEATURE_LOG_ERROR("ivLen must be non-negative");
            is_process_ok = false;
            goto free;
        }

        // check base64 string validity
        const char* enc = (const char*)opts->iv;
        size_t enc_len = strlen(enc);
        if (enc_len == 0 || (enc_len % 4) != 0) {
            FEATURE_LOG_ERROR("invalid base64 iv string length");
            is_process_ok = false;
            goto free;
        }

        // compute expected decoded length from encoded length and padding
        size_t decoded_len = (enc_len / 4) * 3;
        if (enc_len >= 1 && enc[enc_len - 1] == '=') decoded_len--;
        if (enc_len >= 2 && enc[enc_len - 2] == '=') decoded_len--;

        // if user supplied ivLen, validate it matches decoded length
        if (opts->ivLen) {
            if ((size_t)opts->ivLen != decoded_len) {
                FEATURE_LOG_ERROR("ivLen mismatch with base64 iv string length");
                is_process_ok = false;
                goto free;
            }
        }

        *iv = (const unsigned char*)malloc(decoded_len);
        if (*iv == NULL) {
            FEATURE_LOG_ERROR("malloc iv failed");
            is_process_ok = false;
            goto free;
        }

        // get the real iv bytes and ivLen
        if (base64_decode(enc, enc_len, (char*)*iv, decoded_len, ivLen) != 0) {
            FEATURE_LOG_ERROR("base64 decode iv failed");
            is_process_ok = false;
            goto free;
        }

        if (is_auth_crypto) {
            if (*ivLen < 7 || *ivLen > 13) {
                FEATURE_LOG_ERROR("invalid CCM ivLen %zu, expect 7..13", *ivLen);
                is_process_ok = false;
                goto free;
            }
        }
    } else {
        // iv not provided
        if (is_auth_crypto) {
            // For auth crypto, iv is mandatory.
            FEATURE_LOG_ERROR("iv is required for authenticated encryption (GCM/CCM)");
            is_process_ok = false;
            goto free;
        } else {
            // non-auth modes: default ivLen is either provided by user or 16
            // validate user provided ivLen is not negative
            if (opts->ivLen < 0) {
                FEATURE_LOG_ERROR("ivLen must be non-negative");
                is_process_ok = false;
                goto free;
            }
            size_t default_iv_len = opts->ivLen ? (size_t)opts->ivLen : aes_default_values.ivLen;

            if (default_iv_len > key_len) {
                FEATURE_LOG_ERROR("ivLen %zu exceeds key length %zu", default_iv_len, key_len);
                is_process_ok = false;
                goto free;
            }

            *iv = (const unsigned char*)malloc(default_iv_len);
            if (*iv == NULL) {
                FEATURE_LOG_ERROR("malloc iv failed");
                is_process_ok = false;
                goto free;
            }

            // fill iv from key bytes (key is already in bytes)
            // copy up to default_iv_len bytes; if key shorter, pad the rest with 0
            // Note: `key` is expected to be at least 16 bytes for AES; we do not have key_size here,
            // so we conservatively copy default_iv_len bytes from `key`.
            memcpy((void*)*iv, key, default_iv_len);
            *ivLen = default_iv_len;
        }
    }

    // get ivOffset
    *ivOffset = opts->ivOffset ? opts->ivOffset : aes_default_values.ivOffset;

    if (*iv != NULL) {
        if (*ivOffset < 0) {
            FEATURE_LOG_ERROR("ivOffset must be non-negative");
            is_process_ok = false;
            goto free;
        }
        // allocated buffer size is *ivLen
        // read range is [*ivOffset, *ivOffset + *ivLen)
        // so we need *ivOffset + *ivLen <= *ivLen, which implies *ivOffset <= 0
        if ((size_t)*ivOffset > 0) {
            FEATURE_LOG_ERROR("ivOffset %d causes OOB read (buffer size: %zu)", *ivOffset, *ivLen);
            is_process_ok = false;
            goto free;
        }
    }

    // get aad, aadLen
    if (opts->aad && !translate_string_and_uint8array_to_byte(ft_ctx, *(opts->aad), aad, aadLen)) {
        FEATURE_LOG_ERROR("translate aad to bytes failed");
        is_process_ok = false;
        goto free;
    }

    // get tagLen, only encrypt need tagLen, so tagLen won't be update by following codes.
    if (opts->tagLen < 0) {
        FEATURE_LOG_ERROR(" tagLen can not be negative");
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
    Cipher_ErrorCode ret = GOOD;

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
        // if internal options is exist, parse transformation
        if (options->options) {
            // transformation is exist
            if (check_str(options->options->transformation)) {
                // if transformation is not null, parse it to update mode and padding
                // check the result since transformation represents what the caller expects.
                if (!parse_transformation(AESCipherSupported, AES_SUPPORTED_COUNT, (char*)options->options->transformation, key_size, &mode, &padding)) {
                    FEATURE_LOG_ERROR("unsupported AES cipher");
                    *msg = "unsupported algorithm cipher, please check transformation";
                    ret = UNSUPPORTED;
                    goto free;
                }
            }
        } else {
            // internal options is null, so must be non-auth AES, use default  iv / iv_offset / ivLen
            ivLen = aes_default_values.ivLen;

            if (ivLen > key_size) {
                FEATURE_LOG_ERROR("ivLen %zu exceeds key length %zu", ivLen, key_size);
                *msg = "ivLen exceeds key length";
                ret = ARGSERROR;
                goto free;
            }

            iv = (const unsigned char*)malloc(ivLen);
            if (iv == NULL) {
                FEATURE_LOG_ERROR("malloc iv failed");
                *msg = "malloc iv failed";
                ret = GENERAL;
                goto free;
            }
            ivOffset = aes_default_values.ivOffset;
            memcpy((void*)iv, key, ivLen);
        }

        // if transformation is empty string or not exist , use default transformation, must be AES/CBC/PKCS7Padding
        // get mode and padding
        if ((options->options && options->options->transformation && strlen((const char*)options->options->transformation) == 0) ||
            (!options->options)) {
            FEATURE_LOG_INFO("use default transformation : AES/CBC/PKCS7Padding");
            // set default mode and padding, very ugly hack but no other choice for compatibility
            padding = aes_default_values.padding;
            if (key_size == 16) {
                mode = AES_128_CBC;
            } else if (key_size == 24) {
                mode = AES_192_CBC;
            } else if (key_size == 32) {
                mode = AES_256_CBC;
            } else {
                FEATURE_LOG_ERROR("unsupported AES key size(%d) for default transformation", key_size);
                *msg = "unsupported AES key size for default transformation";
                ret = UNSUPPORTED;
                goto free;
            }
        }

        // check if mode is aes ccm
        if (mode == AES_128_CCM || mode == AES_192_CCM || mode == AES_256_CCM) {
            *is_auth_crypto = true;
        } else {
            *is_auth_crypto = false;
        }

        if (options->options) {
            // parse internal options to choose if use default value or not.
            if (!parse_internal_options(ft_ctx, options, mode, key, key_size, &iv, &ivOffset, &ivLen, &aad, &aadLen, &tag_input, &tagLen_input, *is_auth_crypto)) {
                FEATURE_LOG_ERROR("wrong internal options, please check arguments");
                *msg = "wrong internal options, please check arguments";
                ret = ARGSERROR;
                goto free;
            }
        }

        // excute native function
        // if aes ccm
        if (*is_auth_crypto) {
            if (operation == DECRYPT_OPERATION && (tag_input == NULL || tagLen_input == 0)) {
                FEATURE_LOG_ERROR("aes-ccm decrypt requires tag input");
                *msg = "aes-ccm decrypt requires tag input";
                ret = ARGSERROR;
                goto free;
            }
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
    } else {
        // When options->options is NULL, we allocated iv in the else branch above (line ~1021)
        // Must free it to avoid memory leak
        if (iv) {
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
        FEATURE_LOG_INFO("%s, result: %s", file_tag, result);
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
void system_crypto_wrap_hkdf(FeatureInstanceHandle feature, AppendData append_data,
    system_crypto_HkdfParam* options)
{
    ft_context_ref ft_ctx = FeatureGetContext(feature);
    FEATURE_CHECK_NE(ft_ctx, NULL);

    const char* algo = check_str(options->algo) ? (const char*)options->algo : "SHA256";
    uint8_t* key = NULL;
    size_t key_size = 0;
    uint8_t* salt = NULL;
    size_t salt_size = 0;
    uint8_t* info = NULL;
    size_t info_size = 0;
    uint8_t* output = NULL;
    size_t output_size = 0;
    const char* msg = "";
    int ret = 0;

    if (options->key == NULL || options->salt == NULL || options->info == NULL) {
        FEATURE_LOG_ERROR("system_crypto_wrap_hkdf key, salt and info must be valid");
        msg = "key, salt and info must be valid";
        ret = ARGSERROR;
        goto exit;
    }

    if (translate_string_and_uint8array_to_byte(ft_ctx, *(options->key), &key, &key_size) == false || translate_string_and_uint8array_to_byte(ft_ctx, *(options->salt), &salt, &salt_size) == false || translate_string_and_uint8array_to_byte(ft_ctx, *(options->info), &info, &info_size) == false || options->keyLen <= 0) {
        FEATURE_LOG_ERROR("system_crypto_wrap_hkdf translate_string_and_uint8array_to_byte failed ");
        msg = "wrong input parament";
        ret = ARGSERROR;
        goto exit;
    }

    if ((strcmp(algo, "SHA256") == 0) || (strcmp(algo, "SHA512") == 0)) {
        output_size = options->keyLen;
    } else {
        FEATURE_LOG_ERROR("system_crypto_wrap_hkdf wrong algorithm");
        msg = "wrong algorithm";
        ret = UNSUPPORTED;
        goto exit;
    }

    output = (uint8_t*)malloc(output_size);
    if (output == NULL) {
        FEATURE_LOG_ERROR("malloc output failed");
        msg = "malloc output failed";
        ret = GENERAL;
        goto exit;
    }

    if (hkdf_key_derivation(algo, salt, salt_size, key, key_size, info, info_size, output, output_size) != 0) {
        FEATURE_LOG_ERROR("crypto.hkdf_key_derivation failed");
        msg = "create hkdf key failed";
        ret = GENERAL;
        goto exit;
    }

exit:
    // deal with result
    if (output && options->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, (const char*)output, output_size, false);
        ft_obj_set_property(ft_ctx, ret_obj, "data", ret_data);
        INVOKE_SUCCESS_CB(options->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (options->fail) {
        INVOKE_FAIL_CB(options->fail, msg, ret);
    }

    if (options->complete) {
        INVOKE_COMPLET_CB(options->complete);
    }

    if (key) {
        free(key);
    }

    if (salt) {
        free(salt);
    }

    if (info) {
        free(info);
    }

    if (output) {
        free(output);
    }
}

typedef struct CryptoECDH {
    int group_id;
    unsigned char* privateKey;
    size_t privateKey_size;
    unsigned char* publicKey;
    size_t publicKey_size;
} CryptoECDH;

static int check_cruve_type(FtString type, int* group_id, size_t* privateKey_size, size_t* publicKey_size)
{
    *group_id = 0;
    for (size_t i = 0; i < ECDH_SUPPORTED_COUNT; i++) {
        if (strcmp(type, ECDHCfgSupported[i].cfg_index.curve) == 0 && ECDHCfgSupported[i].is_supported == true) {
            *group_id = ECDHCfgSupported[i].cfg_index.group_id;
            *privateKey_size = ECDHCfgSupported[i].cfg_index.privateKey_size;
            *publicKey_size = ECDHCfgSupported[i].cfg_index.publicKey_size;
            break;
        }
    }

    if (*group_id == 0) {
        return -1;
    }

    return 0;
}

static bool ECDH_allocate_keys(CryptoECDH* ecdh)
{
    if (ecdh == NULL) {
        return false;
    }

    if (ecdh->publicKey) {
        free(ecdh->publicKey);
    }

    ecdh->publicKey = (unsigned char*)malloc(ecdh->publicKey_size);
    if (ecdh->publicKey == NULL) {
        return false;
    }

    if (ecdh->privateKey) {
        free(ecdh->privateKey);
    }
    ecdh->privateKey = (unsigned char*)malloc(ecdh->privateKey_size);
    if (ecdh->privateKey == NULL) {
        free(ecdh->publicKey);
        ecdh->publicKey = NULL;
        return false;
    }

    return true;
}

static int initialize_ecdh(FtString type, CryptoECDH* ecdh)
{
    if (ecdh == NULL) {
        return -1;
    }

    if (check_cruve_type(type, &ecdh->group_id, &ecdh->privateKey_size, &ecdh->publicKey_size) != 0) {
        return -1;
    }

    ecdh->privateKey = NULL;
    ecdh->publicKey = NULL;

    if (!ECDH_allocate_keys(ecdh)) {
        return -1;
    }

    return 0;
}

static int ecdh_get_decoding_data(const char* encoding,
    const unsigned char* input, size_t input_size,
    unsigned char* output, size_t output_size, size_t* exact_size,
    bool* is_text)
{
    int ret = 0;
    if (encoding) {
        if (strcmp(encoding, "base64") == 0) {
            ret = base64_decode((const char*)input, input_size, (char*)output, output_size, exact_size);
            if (ret != 0) {
                FEATURE_LOG_ERROR("crypto.ecdh_get_decoding_data base64_encode failed");
                return -1;
            }
            *is_text = true;
            return 0;
        } else if (strcmp(encoding, "hex") == 0) {
            crypto_unhexify((const char*)input, input_size, output, exact_size);
            if (output == NULL || *exact_size == 0) {
                FEATURE_LOG_ERROR("crypto.ecdh_get_decoding_data crypto_unhexify failed");
                return -1;
            }
            *is_text = true;
            return 0;
        } else if (strcmp(encoding, "buffer") == 0 || strcmp(encoding, "") == 0) {
            memcpy(output, input, input_size);
            *exact_size = input_size;
            return 0;
        } else {
            FEATURE_LOG_ERROR("crypto.ecdh_get_decoding_data wrong encoding type");
            return -1;
        }
    } else {
        memcpy(output, input, input_size);
        *exact_size = input_size;
        return 0;
    }
}

static int ecdh_get_encoding_data(const char* encoding,
    const unsigned char* input, size_t input_size,
    unsigned char* output, size_t output_size, size_t* exact_size,
    bool* is_text)
{
    int ret = 0;
    if (encoding) {
        if (strcmp(encoding, "base64") == 0) {
            ret = base64_encode((const char*)input, input_size, (char*)output, output_size, exact_size);
            if (ret != 0) {
                FEATURE_LOG_ERROR("crypto.ecdh_get_encoding_data base64_encode failed");
                return -1;
            }
            *is_text = true;
            return 0;
        } else if (strcmp(encoding, "hex") == 0) {
            crypto_hexify((char*)input, input_size, (char*)output, exact_size);
            if (output == NULL || *exact_size == 0) {
                FEATURE_LOG_ERROR("crypto.ecdh_get_encoding_data crypto_hexify failed");
                return -1;
            }
            output[*exact_size] = '\0';
            *is_text = true;
            return 0;
        } else if (strcmp(encoding, "buffer") == 0 || strcmp(encoding, "") == 0) {
            memcpy(output, input, input_size);
            *exact_size = input_size;
            *is_text = false;
            return 0;
        } else {
            FEATURE_LOG_ERROR("crypto.ecdh_get_encoding_data wrong encoding type");
            return -1;
        }
    } else {
        memcpy(output, input, input_size);
        *exact_size = input_size;
        return 0;
    }
}

void system_crypto_ECDH_interface_ECDH_imp_finalize(FeatureInterfaceHandle handle)
{
    void* data = FeatureGetObjectData(handle);
    if (data == NULL) {
        return;
    }
    CryptoECDH* ecdh = static_cast<CryptoECDH*>(data);

    if (ecdh->privateKey) {
        free(ecdh->privateKey);
        ecdh->privateKey = NULL;
    }
    ecdh->privateKey_size = 0;

    if (ecdh->publicKey) {
        free(ecdh->publicKey);
        ecdh->publicKey = NULL;
    }
    ecdh->publicKey_size = 0;

    if (ecdh) {
        free(ecdh);
    }
}

void system_crypto_ECDH_interface_ECDH_imp_generateKeys(FeatureInterfaceHandle handle, AppendData append_data, system_crypto_CurveParam* param)
{
    int ret = 0;
    const char* msg = "";
    size_t publicKey_outSize = 0;
    size_t output_size = 0;
    unsigned char* output = NULL;
    bool is_text = false;
    CryptoECDH* ecdh = NULL;

    ft_context_ref ft_ctx = FeatureGetContext(handle);
    void* data = FeatureGetObjectData(handle);
    if (data == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys please create ECDH object first");
        msg = "please create ECDH object first";
        ret = GENERAL;
        goto exit;
    }
    ecdh = static_cast<CryptoECDH*>(data);

    if (!ECDH_allocate_keys(ecdh)) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys ECDH_allocate_keys failed");
        msg = "ECDH_allocate_keys failed";
        ret = GENERAL;
        return;
    }

    ret = ECDH_generate_key(ecdh->group_id, ecdh->publicKey, ecdh->publicKey_size, &publicKey_outSize, ecdh->privateKey, ecdh->privateKey_size);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys ECDH_generate_key failed");
        msg = "ECDH_generate_key failed";
        ret = GENERAL;
        goto exit;
    }
    ecdh->publicKey_size = publicKey_outSize;

    // make sure output is large enough when using "hex" encoding
    output = (unsigned char*)malloc(2 * (ecdh->publicKey_size) + 1);
    if (output == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys malloc output failed");
        msg = "malloc output failed";
        ret = GENERAL;
        goto exit;
    }

    ret = ecdh_get_encoding_data(param->encoding, ecdh->publicKey, ecdh->publicKey_size, output, 2 * (ecdh->publicKey_size) + 1, &output_size, &is_text);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys ecdh_get_encoding_data failed");
        msg = "ecdh_get_encoding_data failed";
        ret = GENERAL;
        goto exit;
    }

exit:
    // deal with result
    if (!ret && param->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, (const char*)output, output_size, is_text);
        ft_obj_set_property(ft_ctx, ret_obj, "publicKey", ret_data);
        INVOKE_SUCCESS_CB_INTERFACE(param->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (param->fail) {
        INVOKE_FAIL_CB_INTERFACE(param->fail, msg, ret);
    }

    if (param->complete) {
        INVOKE_COMPLET_CB_INTERFACE(param->complete);
    }
    REMOVE_ALL_CBS_INTERFACE(param);

    if (output) {
        free(output);
    }
}

FtAny system_crypto_ECDH_interface_ECDH_imp_getPrivateKey(FeatureInterfaceHandle handle, AppendData append_data, FtString encoding)
{
    int ret = 0;
    size_t output_size = 0;
    unsigned char* output = NULL;
    bool is_text = false;

    ft_context_ref ft_ctx = FeatureGetContext(handle);
    void* data = FeatureGetObjectData(handle);
    if (data == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys please generate keypair first");
        ret = GENERAL;
        return NULL;
    }
    CryptoECDH* ecdh = static_cast<CryptoECDH*>(data);

    // make sure output is large enough when using "hex" encoding
    output = (unsigned char*)malloc(2 * (ecdh->privateKey_size) + 1);
    if (output == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_getPrivateKey malloc output failed");
        return NULL;
    }

    ret = ecdh_get_encoding_data(encoding, ecdh->privateKey, ecdh->privateKey_size, output, 2 * (ecdh->privateKey_size) + 1, &output_size, &is_text);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_getPrivateKey ecdh_get_encoding_data failed");
        free(output);
        return NULL;
    }

    ft_value_t* ret_data = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *ret_data = from_buff(ft_ctx, (const char*)output, output_size, is_text);
    free(output);
    return ret_data;
}

FtAny system_crypto_ECDH_interface_ECDH_imp_getPublicKey(FeatureInterfaceHandle handle, AppendData append_data, FtString encoding)
{
    int ret = 0;
    size_t output_size = 0;
    unsigned char* output = NULL;
    bool is_text = false;

    ft_context_ref ft_ctx = FeatureGetContext(handle);
    void* data = FeatureGetObjectData(handle);
    if (data == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys please generate keypair first");
        ret = GENERAL;
        return NULL;
    }
    CryptoECDH* ecdh = static_cast<CryptoECDH*>(data);

    // make sure output is large enough when using "hex" encoding
    output = (unsigned char*)malloc(2 * (ecdh->publicKey_size) + 1);
    if (output == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_getPublicKey malloc output failed");
        return NULL;
    }

    ret = ecdh_get_encoding_data(encoding, ecdh->publicKey, ecdh->publicKey_size, output, 2 * (ecdh->publicKey_size) + 1, &output_size, &is_text);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_getPublicKey ecdh_get_encoding_data failed");
        free(output);
        return NULL;
    }

    ft_value_t* ret_data = (ft_value_t*)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *ret_data = from_buff(ft_ctx, (const char*)output, output_size, is_text);
    free(output);
    return ret_data;
}

void system_crypto_ECDH_interface_ECDH_imp_setPrivateKey(FeatureInterfaceHandle handle, AppendData append_data, system_crypto_setPrivateKeyParam* param)
{
    unsigned char* privateKey_data = NULL;
    size_t privateKey_len = 0;
    size_t pubkey_outsize = 0;
    bool is_text = false;
    const char* msg = "";
    Cipher_ErrorCode ret = GOOD;
    CryptoECDH* ecdh = NULL;

    ft_context_ref ft_ctx = FeatureGetContext(handle);
    void* data = FeatureGetObjectData(handle);
    if (data == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys please create ECDH object first");
        msg = "please create ECDH object first";
        ret = GENERAL;
        goto exit;
    }
    ecdh = static_cast<CryptoECDH*>(data);

    if (!param->privateKey) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_setPrivateKey privateKey is NULL");
        msg = "privateKey is NULL";
        ret = ARGSERROR;
        goto exit;
    }

    privateKey_data = get_buff(ft_ctx, *(param->privateKey), &privateKey_len, &is_text);
    if (privateKey_data == NULL || privateKey_len == 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_setPrivateKey get_buff failed");
        msg = "get_buff failed";
        ret = ARGSERROR;
        goto exit;
    }

    if (!ECDH_allocate_keys(ecdh)) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_setPrivateKey ECDH_allocate_keys failed");
        msg = "ECDH_allocate_keys failed";
        ret = GENERAL;
        goto exit;
    }

    if (is_text) {
        if (ECDH_generate_keypair_by_pem(ecdh->group_id,
                privateKey_data, privateKey_len,
                ecdh->privateKey, ecdh->privateKey_size,
                ecdh->publicKey, ecdh->publicKey_size, &pubkey_outsize)
            != 0) {
            FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_setPrivateKey ECDH_generate_keypair_by_pem failed");
            msg = "ECDH_generate_keypair_by_pem failed";
            ret = GENERAL;
            goto exit;
        }
    } else {
        if (ECDH_generate_keypair_by_binary(ecdh->group_id,
                privateKey_data, privateKey_len,
                ecdh->privateKey, &ecdh->privateKey_size,
                ecdh->publicKey, ecdh->publicKey_size, &pubkey_outsize)
            != 0) {
            FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_setPrivateKey ECDH_generate_keypair_by_binary failed");
            msg = "ECDH_generate_keypair_by_binary failed";
            ret = GENERAL;
            goto exit;
        }
    }

exit:
    // deal with result
    if (!ret && param->success) {
        INVOKE_SUCCESS_CB_INTERFACE(param->success, "");
    } else if (param->fail) {
        INVOKE_FAIL_CB_INTERFACE(param->fail, msg, ret);
    }

    if (param->complete) {
        INVOKE_COMPLET_CB_INTERFACE(param->complete);
    }
    REMOVE_ALL_CBS_INTERFACE(param);
}

void system_crypto_ECDH_interface_ECDH_imp_computeSecret(FeatureInterfaceHandle handle, AppendData append_data, system_crypto_ComputeParam* param)
{
    int ret = 0;
    const char* msg = "";
    unsigned char* publicKey_input = NULL;
    size_t publicKey_size = 0;
    unsigned char* publicKey_decode = NULL;
    size_t publicKey_decode_size = 0;
    unsigned char* secretKey = NULL;
    unsigned char* output = NULL;
    size_t output_size = 0;
    bool is_text = false;
    CryptoECDH* ecdh = NULL;
    size_t secretKey_size = 0;

    ft_context_ref ft_ctx = FeatureGetContext(handle);
    void* data = FeatureGetObjectData(handle);
    if (data == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_generateKeys please create ECDH object first");
        msg = "please create ECDH object first";
        ret = GENERAL;
        goto exit;
    }
    ecdh = static_cast<CryptoECDH*>(data);
    secretKey_size = ecdh->privateKey_size;

    if (!param->otherPublicKey) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret otherPublicKey is NULL");
        msg = "otherPublicKey is NULL";
        ret = ARGSERROR;
        goto exit;
    }

    publicKey_input = get_buff(ft_ctx, *(param->otherPublicKey), &publicKey_size, &is_text);
    if (publicKey_input == NULL || publicKey_size == 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret get_buff failed");
        msg = "wrong otherPublicKey type";
        ret = ARGSERROR;
        goto exit;
    }

    publicKey_decode = (unsigned char*)malloc(publicKey_size);
    if (publicKey_decode == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret malloc publicKey_decode failed");
        msg = "malloc publicKey_decode failed";
        ret = GENERAL;
        goto exit;
    }

    // decoding intput data to bytes
    ret = ecdh_get_decoding_data(param->inputEncoding, publicKey_input, publicKey_size, publicKey_decode, publicKey_size, &publicKey_decode_size, &is_text);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret ecdh_get_decoding_data failed");
        msg = "ecdh_get_decoding_data failed";
        ret = ARGSERROR;
        goto exit;
    }

    secretKey = (unsigned char*)malloc(ecdh->privateKey_size);
    if (secretKey == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret malloc secretKey failed");
        msg = "malloc secretKey failed";
        ret = GENERAL;
        goto exit;
    }

    ret = ECDH_compute_shared_key(ecdh->group_id,
        publicKey_decode, publicKey_decode_size,
        ecdh->privateKey, ecdh->privateKey_size,
        secretKey, &secretKey_size);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret ECDH_compute_shared_key failed");
        msg = "ECDH_compute_shared_key failed";
        ret = GENERAL;
        goto exit;
    }

    // make sure output is large enough when using "hex" encoding
    output = (unsigned char*)malloc(2 * secretKey_size + 1);
    if (output == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret malloc output failed");
        msg = "malloc output failed";
        ret = GENERAL;
        goto exit;
    }

    // encoding output data
    ret = ecdh_get_encoding_data(param->outputEncoding, secretKey, secretKey_size, output, 2 * secretKey_size + 1, &output_size, &is_text);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_ECDH_interface_ECDH_imp_computeSecret ecdh_get_encoding_data failed");
        msg = "ecdh_get_encoding_data failed";
        ret = ARGSERROR;
        goto exit;
    }

exit:
    // deal with result
    if (!ret && param->success) {
        ft_value_t ret_obj = ft_new_object(ft_ctx);
        ft_value_t ret_data = from_buff(ft_ctx, (const char*)output, output_size, is_text);
        ft_obj_set_property(ft_ctx, ret_obj, "shareKey", ret_data);
        INVOKE_SUCCESS_CB_INTERFACE(param->success, (&ret_obj));
        ft_free_value(ft_ctx, ret_obj);
    } else if (param->fail) {
        INVOKE_FAIL_CB_INTERFACE(param->fail, msg, ret);
    }

    if (param->complete) {
        INVOKE_COMPLET_CB_INTERFACE(param->complete);
    }
    REMOVE_ALL_CBS_INTERFACE(param);

    if (publicKey_decode) {
        free(publicKey_decode);
    }

    if (secretKey) {
        free(secretKey);
    }

    if (output) {
        free(output);
    }
}

FeatureInterfaceHandle system_crypto_wrap_createECDH(FeatureInstanceHandle feature, AppendData data, FtString type)
{
    FeatureInterfaceHandle handle = system_crypto_createECDH_instance(feature);
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);

    int ret;
    CryptoECDH* ecdh = (CryptoECDH*)malloc(sizeof(CryptoECDH));
    if (ecdh == NULL) {
        FEATURE_LOG_ERROR("crypto.system_crypto_wrap_createECDH malloc ecdh failed");
        return handle;
    }

    ret = initialize_ecdh(type, ecdh);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.system_crypto_wrap_createECDH initialize_ecdh failed");
        free(ecdh);
        return handle;
    }
    FeatureSetObjectData(handle, ecdh);

    return handle;
}
