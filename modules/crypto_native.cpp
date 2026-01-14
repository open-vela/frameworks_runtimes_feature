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

#include "crypto_native.h"

#include "app_path.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "uv_ext.h"
#include <alloca.h>
#include <ctype.h>
#include <stdio.h>

static const char* file_tag = "[jidl_feature] crypto_native";

const char* crypto_err = NULL;

#define CHECK_ERR_RET(ptr, msg)                                               \
    do {                                                                      \
        if (ptr == NULL) {                                                    \
            FEATURE_LOG_ERROR("%s, check_err_ret, msg: %s\n", file_tag, msg); \
            crypto_err = msg;                                                 \
            return NULL;                                                      \
        }                                                                     \
    } while (0)

#define CHECK_ERR_BREAK(ptr, msg)                                           \
    if (ptr == NULL) {                                                      \
        FEATURE_LOG_ERROR("%s, check_err_break, msg: %s\n", file_tag, msg); \
        crypto_err = msg;                                                   \
        break;                                                              \
    }

static int hex_char_to_value(char c)
{
    if (isxdigit(c)) {
        if (c >= '0' && c <= '9') {
            return c - '0';
        } else if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
    }
    return -1;
}

static int setup_uv_aes_key(uv_aes_t* aes_ctx, int mode, const uint8_t* key, size_t key_size)
{
    if (uv_aes_set_key(aes_ctx, mode, key, key_size * 8) != 0) {
        FEATURE_LOG_ERROR("%s, %s\n", file_tag, "crypto.aes set key failed");
        return -1;
    }

    return 0;
}

static int setup_uv_aes_iv(uv_aes_t* aes_ctx, const unsigned char* iv, int iv_offset, int iv_len)
{
    if (uv_aes_set_iv(aes_ctx, iv, iv_offset, iv_len) != 0) {
        FEATURE_LOG_ERROR("%s, %s\n", file_tag, "crypto.aes set base64 iv failed");
        return -1;
    }

    return 0;
}

int aes_non_auth_crypto(int mode,
    int padding,
    const uint8_t* key, size_t key_size,
    const uint8_t* iv_str, int ivOffset, int ivLen,
    const uint8_t* buff, size_t buff_size,
    uint8_t* output, size_t* output_size,
    int operation)
{
    uv_aes_t aes_ctx = {};
    int ret = 0;

    ret = uv_aes_init(&aes_ctx, mode, padding);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.aes aes_ctx init failed");
        return -1;
    }

    ret = setup_uv_aes_key(&aes_ctx, operation, key, key_size);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.aes set key failed");
        goto exit;
    }

    ret = setup_uv_aes_iv(&aes_ctx, iv_str, ivOffset, ivLen);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.aes set iv failed");
        goto exit;
    }

    if (operation == ENCRYPT_OPERATION) {
        ret = uv_aes_encrypt(&aes_ctx, buff, buff_size, output, output_size);
        if (ret != 0) {
            FEATURE_LOG_ERROR("crypto.aes encrypt failed");
            goto exit;
        }
    } else if (operation == DECRYPT_OPERATION) {
        ret = uv_aes_decrypt(&aes_ctx, buff, buff_size, output, output_size);
        if (ret != 0) {
            FEATURE_LOG_ERROR("crypto.aes decrypt failed");
            goto exit;
        }
        // when decrypt operation, the output_data is utf-8 encoded
        (output)[*output_size] = '\0';

    } else {
        FEATURE_LOG_ERROR("crypto.aes wrong operation");
        goto exit;
    }

exit:
    uv_aes_free(&aes_ctx);

    return ret;
}

int aes_auth_crypto(int mode,
    int padding,
    const uint8_t* key, size_t key_size,
    const uint8_t* iv_str, int ivLen,
    uint8_t* aad, size_t aadLen,
    uint8_t* tag_input, int tagLen_input,
    const uint8_t* buff, size_t buff_size,
    uint8_t* output, size_t output_size, size_t* out_size,
    int operation)
{
    uv_aes_t aes_ctx = {};
    int ret = 0;

    ret = uv_aes_init(&aes_ctx, mode, padding);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.aes aes_ctx init failed");
        return -1;
    }

    ret = setup_uv_aes_key(&aes_ctx, operation, key, key_size);
    if (ret != 0) {
        FEATURE_LOG_ERROR("crypto.aes set key failed");
        goto exit;
    }

    if (operation == ENCRYPT_OPERATION) {
        ret = uv_aes_auth_encrypt(&aes_ctx,
            iv_str, ivLen,
            aad, aadLen,
            tagLen_input,
            buff, buff_size,
            output, output_size,
            out_size);
        if (ret != 0) {
            FEATURE_LOG_ERROR("crypto.aes auth encrypt failed");
            goto exit;
        }
    } else if (operation == DECRYPT_OPERATION) {
        size_t buffTag_size = buff_size + tagLen_input;
        uint8_t* buff_tag = (uint8_t*)malloc(buffTag_size);
        if (buff_tag == NULL) {
            FEATURE_LOG_ERROR("crypto.aes malloc buff_tag failed");
            ret = -1;
            goto exit;
        }
        memcpy(buff_tag, buff, buff_size);
        memcpy(buff_tag + buff_size, tag_input, tagLen_input);
        ret = uv_aes_auth_decrypt(&aes_ctx,
            iv_str, ivLen,
            aad, aadLen,
            tagLen_input,
            buff_tag, buffTag_size,
            output, output_size,
            out_size);
        if (ret != 0) {
            FEATURE_LOG_ERROR("crypto.aes auth decrypt failed");
            goto exit;
        }
    } else {
        FEATURE_LOG_ERROR("crypto.aes wrong operation");
        ret = -1;
        goto exit;
    }

exit:
    uv_aes_free(&aes_ctx);

    return ret;
}

int rsa_crypto(const unsigned char* key_str,
    uint8_t* buff, size_t buff_size,
    uint8_t** output_data, size_t* output_size,
    int operation)
{
    if (key_str == NULL || buff == NULL || output_data == NULL || output_size == NULL) {
        FEATURE_LOG_ERROR("crypto.rsa key_str, buff or output_size is NULL");
        return -1;
    }

    uv_buf_t text = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t output = { 0 };
    int ret = 0;

    key.base = (char*)key_str;
    key.len = strlen((const char*)key_str);
    text.base = (char*)buff;
    text.len = buff_size;

    ret = uv_rsa(key, text, &output, operation);
    if (ret != 0 || output.base == NULL || output.len == 0) {
        FEATURE_LOG_ERROR("crypto.rsa encrypt failed");
        goto exit;
    }

    *output_data = (uint8_t*)malloc(output.len + 1);
    if (*output_data == NULL) {
        *output_size = 0;
        FEATURE_LOG_ERROR("crypto.rsa malloc output_data failed");
        ret = -1;
        goto exit;
    }
    memset(*output_data, 0, output.len + 1);
    memcpy(*output_data, output.base, output.len);
    *output_size = output.len;
    // when decrypt operation, the output_data is utf-8 encoded
    if (operation == DECRYPT_OPERATION) {
        (*output_data)[*output_size] = '\0';
    }

exit:
    if (output.base) {
        free(output.base);
    }

    return ret;
}

bool rsa_verify(const char* type_str, const char* key_str, uint8_t* buff, size_t buff_size, uint8_t* sig_buf, size_t seg_size, bool sig_text)
{
    crypto_err = NULL;
    uv_buf_t type = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t text = { 0 };
    uv_buf_t sig = { 0 };
    uv_buf_t md = { 0 };
    int res;

    do {
        type.base = (char*)type_str;
        type.len = strlen(type_str);
        key.base = (char*)key_str;
        key.len = strlen(key_str);
        text.base = (char*)buff;
        text.len = buff_size;
        CHECK_ERR_BREAK(type_str, "crypto.rsa invalid parameter type");
        CHECK_ERR_BREAK(key_str, "crypto.rsa invalid parameter key");
        CHECK_ERR_BREAK(buff, "crypto.aes invalid parameter text");
        CHECK_ERR_BREAK(sig_buf, "crypto.aes invalid parameter signature");
        sig.base = (char*)sig_buf;
        sig.len = seg_size;

        if (sig_text) {
            md.len = sig.len;
            md.base = (char*)malloc(md.len);
            if (md.base == NULL) {
                CHECK_ERR_BREAK(NULL, "crypto.verify malloc md failed");
            }
            memset(md.base, 0, md.len);
            if (uv_base64_decode(sig.base, sig.len, md.base, md.len, &md.len)) {
                CHECK_ERR_BREAK(NULL, "crypto.verify base64 failed");
            }
            res = uv_verify(type.base, key, text, md, UV_EXT_TYPE_BUFFER);
            free(md.base);
        } else {
            res = uv_verify(type.base, key, text, sig, UV_EXT_TYPE_BUFFER);
        }
        return res == 0;
    } while (false);

    if (sig_text) {
        if (md.base) {
            free(md.base);
        }
    }

    return false;
}

bool rsa_verify_file(const char* type_str, const char* key_str, const char* uri_str, const char* sig_str, const char* pkg_str)
{
    crypto_err = NULL;
    uv_buf_t type = { 0 };
    uv_buf_t text = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t md = { 0 };
    uv_buf_t md_64 = { 0 };
    bool ret = false;

    do {
        type.base = (char*)type_str;
        type.len = strlen(type_str);
        key.base = (char*)key_str;
        key.len = strlen(key_str);
        md_64.base = (char*)sig_str;
        md_64.len = strlen(sig_str);
        CHECK_ERR_BREAK(type_str, "crypto.rsa invalid parameter type");
        CHECK_ERR_BREAK(uri_str, "crypto.rsa invalid parameter path");
        CHECK_ERR_BREAK(key_str, "crypto.rsa invalid parameter key");
        CHECK_ERR_BREAK(pkg_str, "crypto.sign invalid parameter pkg");
        CHECK_ERR_BREAK(sig_str, "crypto.aes invalid parameter md");
        text.base = app_relative_to_absolute_path(pkg_str, uri_str);
        CHECK_ERR_BREAK(text.base, "crypto.sign convert to absoluate path failed");
        text.len = strlen(text.base);
        FEATURE_LOG_INFO("abs_path: %s\n", text.base);

        md.len = md_64.len;
        md.base = (char*)malloc(md.len);
        if (md.base == NULL) {
            CHECK_ERR_BREAK(NULL, "crypto.verify malloc md failed");
        }
        memset(md.base, 0, md.len);
        if (uv_base64_decode(md_64.base, md_64.len, md.base, md.len, &md.len) == 0) {
            ret = uv_verify(type.base, key, text, md, UV_EXT_TYPE_FILE) == 0;
        } else {
            CHECK_ERR_BREAK(NULL, "crypto.verify base64 failed");
        }
    } while (false);

    if (text.base) {
        free(text.base);
    }

    if (md.base) {
        free(md.base);
    }

    return ret;
}

char* base64(const char* type_str, const char* text_str)
{
    crypto_err = NULL;
    CHECK_ERR_RET(type_str, "crypto.base64 invalid parameter type");
    CHECK_ERR_RET(text_str, "crypto.base64 invalid parameter text");

    int type;
    uv_buf_t buf = { 0 };
    uv_buf_t text = { 0 };
    uv_buf_t out = { 0 };
    buf.base = (char*)type_str;
    buf.len = strlen(type_str);
    text.base = (char*)text_str;
    text.len = strlen(text_str);

    do {
        if (strcasecmp(buf.base, "encrypt") == 0) {
            type = UV_EXT_ENCRYPT;
        } else if (strcasecmp(buf.base, "decrypt") == 0) {
            type = UV_EXT_DECRYPT;
        } else {
            CHECK_ERR_RET(NULL, "crypto.base64 invalid parameter type");
        }

        int res;
        if (type == UV_EXT_ENCRYPT) {
            out.len = BASE64_ENCODED_LENGTH(text.len) + 1;
            out.base = (char*)malloc(out.len);
            if (out.base == NULL) {
                CHECK_ERR_RET(NULL, "crypto.base64 malloc failed");
            }
            memset(out.base, 0, out.len);
            res = uv_base64_encode(text.base, text.len, out.base, out.len, &out.len);
        } else {
            out.len = text.len;
            out.base = (char*)malloc(out.len);
            if (out.base == NULL) {
                CHECK_ERR_RET(NULL, "crypto.base64 malloc failed");
            }
            memset(out.base, 0, out.len);
            res = uv_base64_decode(text.base, text.len, out.base, out.len, &out.len);
        }

        if (res != 0) {
            if (out.base) {
                free(out.base);
            }
            CHECK_ERR_RET(NULL, "crypto.base64 calculate failed");
        }

        char* ret_str = (char*)FeatureMalloc(out.len + 1, FT_STRING);
        memcpy(ret_str, out.base, out.len);
        if (out.base) {
            free(out.base);
        }
        return ret_str;
    } while (false);

    return NULL;
}

int base64_encode(const char* input, size_t input_size, char* output, size_t output_size, size_t* exact_size)
{
    if (input == NULL || input_size == 0 || output == NULL || output_size == 0 || exact_size == NULL) {
        return -1;
    }

    if (uv_base64_encode(input, input_size, output, output_size, exact_size) != 0) {
        *exact_size = 0;
        FEATURE_LOG_ERROR("%s, uv_base64_encode failed\n", file_tag);
        return -1;
    }

    return 0;
}

int base64_decode(const char* input, size_t input_size, char* output, size_t output_size, size_t* exact_size)
{
    if (input == NULL || input_size == 0 || output == NULL || output_size == 0 || exact_size == NULL) {
        return -1;
    }

    if (uv_base64_decode(input, input_size, output, output_size, exact_size) != 0) {
        *exact_size = 0;
        FEATURE_LOG_ERROR("%s, uv_base64_decode failed\n", file_tag);
        return -1;
    }

    return 0;
}

char* rsa_sign(const char* type_str, const char* key_str, uint8_t* buff, size_t* buff_size, bool* is_text)
{
    crypto_err = NULL;
    uv_buf_t type = { 0 };
    uv_buf_t text = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t out = { 0 };
    uv_buf_t ret = { 0 };
    do {
        type.base = (char*)type_str;
        type.len = strlen(type_str);
        key.base = (char*)key_str;
        key.len = strlen(key_str);
        text.base = (char*)buff;
        text.len = *buff_size;
        *buff_size = 0;
        CHECK_ERR_BREAK(type_str, "crypto.sign invalid parameter type");
        CHECK_ERR_BREAK(key_str, "crypto.sign invalid parameter key");
        CHECK_ERR_BREAK(buff, "crypto.sign invalid parameter text");

        if (uv_sign(type.base, key, text, &out, UV_EXT_TYPE_BUFFER)) {
            CHECK_ERR_BREAK(NULL, "crypto.sign invalid parameter key");
            break;
        }

        char* ret_str = NULL;
        if (*is_text) {
            ret.len = BASE64_ENCODED_LENGTH(out.len) + 1;
            ret.base = (char*)malloc(ret.len);
            if (ret.base == NULL) {
                CHECK_ERR_BREAK(NULL, "crypto.sign malloc ret failed");
            }
            memset(ret.base, 0, ret.len);
            if (uv_base64_encode(out.base, out.len, ret.base, ret.len, &ret.len) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.sign base64 failed");
                break;
            }
            *buff_size = ret.len;
            ret_str = (char*)malloc((ret.len + 1) * sizeof(char));
            memset(ret_str, 0, ret.len + 1);
            memcpy(ret_str, ret.base, ret.len);
        } else {
            *buff_size = out.len;
            ret_str = (char*)malloc(out.len * sizeof(char));
            memcpy(ret_str, out.base, out.len);
        }

        if (out.base)
            free(out.base);
        if (ret.base)
            free(ret.base);
        return ret_str;
    } while (false);

    if (out.base)
        free(out.base);
    if (ret.base)
        free(ret.base);
    return NULL;
}

char* rsa_sign_file(const char* type_str, const char* key_str, const char* uri_str, const char* pkg_str)
{
    crypto_err = NULL;
    uv_buf_t type = { 0 };
    uv_buf_t text = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t out = { 0 };
    uv_buf_t ret = { 0 };

    do {
        CHECK_ERR_BREAK(type_str, "crypto.sign invalid parameter type");
        CHECK_ERR_BREAK(key_str, "crypto.sign invalid parameter key");
        CHECK_ERR_BREAK(uri_str, "crypto.sign invalid parameter uri");
        CHECK_ERR_BREAK(pkg_str, "crypto.sign invalid parameter pkg");

        type.base = (char*)type_str;
        type.len = strlen(type_str);
        key.base = (char*)key_str;
        key.len = strlen(key_str);
        text.base = app_relative_to_absolute_path(pkg_str, uri_str);
        CHECK_ERR_BREAK(text.base, "crypto.sign convert to absoluate path failed");
        text.len = strlen(text.base);
        FEATURE_LOG_INFO("abs_path: %s\n", text.base);

        if (uv_sign(type.base, key, text, &out, UV_EXT_TYPE_FILE)) {
            CHECK_ERR_BREAK(NULL, "crypto.sign invalid parameter key");
        }

        ret.len = BASE64_ENCODED_LENGTH(out.len) + 1;
        ret.base = (char*)malloc(ret.len);
        if (ret.base == NULL) {
            CHECK_ERR_BREAK(NULL, "crypto.sign malloc ret failed");
        }
        memset(ret.base, 0, ret.len);
        if (uv_base64_encode(out.base, out.len, ret.base, ret.len, &ret.len) != 0) {
            CHECK_ERR_BREAK(NULL, "crypto.sign base64 failed");
            break;
        }
        char* ret_str = (char*)malloc((ret.len + 1) * sizeof(char));
        memset(ret_str, 0, ret.len + 1);
        memcpy(ret_str, ret.base, ret.len);

        if (text.base)
            free(text.base);
        if (out.base)
            free(out.base);
        if (ret.base)
            free(ret.base);
        return ret_str;
    } while (false);

    if (text.base)
        free(text.base);
    if (out.base)
        free(out.base);
    if (ret.base)
        free(ret.base);
    return NULL;
}

char* digest(const char* type_str, uint8_t* text_str, size_t text_size, const char* key_str, size_t key_len)
{
    crypto_err = NULL;
    uv_buf_t type = { 0 };
    uv_buf_t text = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t out = { 0 };
    uv_buf_t ret = { 0 };

    do {
        type.base = (char*)type_str;
        type.len = strlen(type_str);
        text.base = (char*)text_str;
        text.len = text_size;
        CHECK_ERR_BREAK(type_str, "crypto.digest invalid parameter type");
        CHECK_ERR_BREAK(text_str, "crypto.digest invalid parameter text");

        int res;
        key.base = (char*)key_str;
        if (key.base == NULL) {
            res = uv_md(type.base, text, &out);
            if (res != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.digest calculate failed");
            }
        } else {
            key.len = key_len;
            res = uv_md_hmac(type.base, text, &out, &key);
            if (res != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.digest hmac calculate failed");
            }
        }
        uv_hexify(out, &ret);

        char* ret_str = (char*)FeatureMalloc(ret.len + 1, FT_STRING);
        memcpy(ret_str, ret.base, ret.len);
        if (out.base)
            free(out.base);
        if (ret.base)
            free(ret.base);
        return ret_str;
    } while (false);

    if (out.base)
        free(out.base);
    if (ret.base)
        free(ret.base);
    return NULL;
}

char* digest_file(const char* type_str, const char* uri_str, const char* pkg_str)
{
    crypto_err = NULL;
    FEATURE_CHECK_NE(type_str, NULL);
    FEATURE_CHECK_NE(uri_str, NULL);
    FEATURE_CHECK_NE(pkg_str, NULL);

    uv_buf_t type = { 0 };
    uv_buf_t out = { 0 };
    uv_buf_t ret = { 0 };
    type.base = (char*)type_str;
    type.len = strlen(type_str);

    do {
        char* abs_path = app_relative_to_absolute_path(pkg_str, uri_str);
        FEATURE_LOG_INFO("abs_path: %s\n", abs_path);
        if (abs_path == NULL) {
            free(abs_path);
            FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "crypto.digest convert to absolute failed");
            break;
        }

        int res = uv_md_file(type.base, (const char*)abs_path, 1024, &out);
        if (res != 0) {
            FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "crypto.digest calculate failed");
            break;
        }

        uv_hexify(out, &ret);
        char* ret_str = (char*)FeatureMalloc(ret.len + 1, FT_STRING);
        memcpy(ret_str, ret.base, ret.len);

        free(abs_path);
        free(out.base);
        free(ret.base);
        return ret_str;
    } while (false);

    free(out.base);
    free(ret.base);
    return NULL;
}

int hkdf_key_derivation(const char* algo,
    uint8_t* salt, size_t salt_len,
    const uint8_t* ikm, size_t ikm_len,
    const unsigned char* info, size_t info_len,
    uint8_t* okm, size_t okm_len)
{
    if (uv_hkdf_key_derivation(algo, salt, salt_len, ikm, ikm_len, info, info_len, okm, okm_len) != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "crypto.hkdf_key_derivation failed");
        return -1;
    }

    return 0;
}

int ECDH_generate_key(int group_id,
    unsigned char* pubKey, size_t pubKey_len, size_t* pubKey_outSize,
    unsigned char* privKey, size_t privKey_outSize)
{
    uv_ecp_t ctx;
    int ret;

    ret = uv_ecdh_init(&ctx, group_id);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_init failed");
        return -1;
    }

    ret = uv_ecdh_gen_keypair(&ctx);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_gen_keypair failed");
        goto free_and_exit;
    }

    ret = uv_ecdh_get_privkey(&ctx, privKey, privKey_outSize);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_get_privkey failed");
        goto free_and_exit;
    }

    ret = uv_ecdh_get_pubkey(&ctx, group_id, pubKey, pubKey_len, pubKey_outSize);
    if (ret != 0 || pubKey_len != *pubKey_outSize) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_get_pubkey failed");
        goto free_and_exit;
    }

free_and_exit:
    uv_ecdh_free(&ctx);

    return ret;
}

int ECDH_compute_shared_key(int group_id,
    unsigned char* pubKey_input, size_t pubKey_size,
    unsigned char* privKey_self, size_t privKey_self_size,
    unsigned char* secretKey, size_t* secretKey_size)
{
    uv_ecp_t ctx;
    int ret;

    ret = uv_ecdh_init(&ctx, group_id);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_init failed");
        return -1;
    }

    ret = uv_ecdh_gen_keypair_from_binary(&ctx, MBEDTLS_ECP_DP_SECP256R1, privKey_self, privKey_self_size);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_gen_keypair_from_binary failed\n");
        goto free_and_exit;
    }

    ret = uv_ecdh_compute_sharedkey(&ctx, pubKey_input, pubKey_size, secretKey, secretKey_size);
    if (ret != 0 || privKey_self_size != *secretKey_size) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_compute_sharedkey failed");
        goto free_and_exit;
    }

free_and_exit:
    uv_ecdh_free(&ctx);

    return ret;
}

void crypto_hexify(char* input_data, size_t input_size, char* output_string, size_t* output_len)
{
    uv_buf_t input = { 0 };
    uv_buf_t output = { 0 };

    input.base = input_data;
    input.len = input_size;

    uv_hexify(input, &output);
    if (output.base == NULL || output.len == 0) {
        return;
    }

    memcpy(output_string, (char*)output.base, output.len);
    *output_len = output.len;
}

void crypto_unhexify(const char* hex_str, size_t hex_len, unsigned char* output, size_t* output_size)
{
    if (hex_len % 2 != 0 || hex_str == NULL || output == NULL || output_size == NULL) {
        FEATURE_LOG_ERROR("Invalid input parameters");
        *output_size = 0;
        return;
    }

    size_t byte_len = hex_len / 2;
    unsigned char* obuf = (unsigned char*)malloc(byte_len);
    if (obuf == NULL) {
        FEATURE_LOG_ERROR("malloc obuf failed");
        *output_size = 0;
        return;
    }

    const char* ibuf = hex_str;
    size_t byte_index = 0;

    while (byte_index < byte_len && *ibuf != '\0') {
        int high_nibble = hex_char_to_value(*ibuf++);
        int low_nibble = hex_char_to_value(*ibuf++);

        if (high_nibble == -1 || low_nibble == -1) {
            free(obuf);
            FEATURE_LOG_ERROR("Non-hexadecimal character encountered");
            *output_size = 0;
            return;
        }

        obuf[byte_index++] = (high_nibble << 4) | low_nibble;
    }

    if (byte_index != byte_len) {
        free(obuf);
        FEATURE_LOG_ERROR("Incomplete hex string or non-hex character encountered");
        *output_size = 0;
        return;
    }

    memcpy(output, obuf, byte_len);
    *output_size = byte_len;

    free(obuf);
}

int ECDH_generate_keypair_by_pem(int group_id,
    unsigned char* priKey_pem, size_t priKey_pem_len,
    unsigned char* priKey_data, size_t priKey_size,
    unsigned char* pubkey, size_t pubkey_size, size_t* pubkey_outsize)
{
    int ret;

    ret = uv_ecdh_gen_keypair_from_pem(group_id, (const char*)priKey_pem, priKey_pem_len, priKey_data, priKey_size, pubkey, pubkey_size, pubkey_outsize);
    if (ret != 0 || pubkey_size != *pubkey_outsize) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_gen_keypair_from_pem failed\n");
        return -1;
    }

    return 0;
}

int ECDH_generate_keypair_by_binary(int group_id,
    unsigned char* priKey_input, size_t priKey_input_len,
    unsigned char* priKey_data, size_t* priKey_size,
    unsigned char* pubkey, size_t pubkey_size, size_t* pubkey_outsize)
{
    int ret;
    uv_ecp_t ctx;

    ret = uv_ecdh_init(&ctx, group_id);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_init failed\n");
        return -1;
    }

    ret = uv_ecdh_gen_keypair_from_binary(&ctx, MBEDTLS_ECP_DP_SECP256R1, priKey_input, priKey_input_len);
    if (ret != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_gen_keypair_from_binary failed\n");
        goto free_and_exit;
    }

    ret = uv_ecdh_get_pubkey(&ctx, group_id, pubkey, pubkey_size, pubkey_outsize);
    if (ret != 0 || pubkey_size != *pubkey_outsize) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "uv_ecdh_get_pubkey failed\n");
        goto free_and_exit;
    }

    memcpy(priKey_data, priKey_input, priKey_input_len);
    *priKey_size = priKey_input_len;

free_and_exit:
    uv_ecdh_free(&ctx);

    return ret;
}