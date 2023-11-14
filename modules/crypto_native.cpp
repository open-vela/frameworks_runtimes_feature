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

#include "feature_log.h"
#include "feature_utils.h"
#include "uv_ext.h"

#include <alloca.h>
#include <stdio.h>

#define APP_PATH_PREFIX "internal://"
#define PATH_MAX_LENGTH CONFIG_PATH_MAX
#define arrayof(array) sizeof(array) / sizeof(array[0])

#ifdef __NuttX__
#define ABS_PATH_PREFIX "/data/quickapp"
#else
#define ABS_PATH_PREFIX "/quickapp"
#endif

static const char* file_tag = "[jidl_feature] crypto_native";

const char* crypto_err = NULL;

#define CHECK_ERR_RET(ptr, msg) \
    do { \
        if (ptr == NULL) { \
            crypto_err = msg; \
            return NULL;\
        } \
    } while (0)

#define CHECK_ERR_BREAK(ptr, msg) \
    if (ptr == NULL) { \
        crypto_err = msg; \
        break; \
    }

static bool setup_uv_aes(uv_aes_t* aes_ctx, int mode,
        const unsigned char* key, const unsigned char* iv, int iv_offset, int iv_len)
{
    unsigned int key_bitlen = aes_ctx->aes_context.cipher_info->key_bitlen;
    if (uv_aes_set_key_base64(aes_ctx, mode, key, key_bitlen) != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "crypto.aes set base64 key failed");
        return false;
    }

    unsigned int iv_size  = aes_ctx->aes_context.cipher_info->iv_size;
    if (iv_size != 0 && uv_aes_set_iv_base64(aes_ctx, iv, iv_offset, iv_len) != 0) {
        FEATURE_LOG_ERROR("%s::%s(), %s\n", file_tag, __FUNCTION__, "crypto.aes set base64 iv failed");
        return false;
    }

    return true;
}

static const char* type_list[] = { "cache", "file", "mass", "tmp" };
static char* app_relative_to_absolute_path(const char* pkg, const char* relative_path)
{
    char* absolute_path = NULL;
    char *path = NULL;
    char *offset, *type, *filename = NULL;

    if (!relative_path || !pkg) {
        return NULL;
    }

    if (relative_path == NULL) {
        return NULL;
    }
    path = strdup(relative_path);

    if (strstr(path, APP_PATH_PREFIX) == NULL) {
        free(path);
        return NULL;
    }
    offset = path + strlen(APP_PATH_PREFIX);

    type = strchr(offset, '/');
    if (type != NULL) {
        *type = '\0';
        filename = ++type;
    }

    type = offset;
    for (unsigned long i = 0; i < arrayof(type_list); i++) {
        if (!strncmp(type_list[i], type, strlen(type_list[i]))) {
            break;
        }

        if (i == arrayof(type_list) - 1) {
            free(path);
            return NULL;
        }
    }

    absolute_path = (char*)malloc(PATH_MAX_LENGTH);
    memset(absolute_path, 0, PATH_MAX_LENGTH);
#ifndef __NuttX__
    getcwd(absolute_path, PATH_MAX_LENGTH);
#endif
    offset = absolute_path + strlen(absolute_path);

    if (!strcmp(type, type_list[3])) {
        snprintf(offset, PATH_MAX_LENGTH - (offset - absolute_path), ABS_PATH_PREFIX "/%s", type);
    } else {
        snprintf(offset, PATH_MAX_LENGTH - (offset - absolute_path), ABS_PATH_PREFIX "/%s/%s", type, pkg);
    }
    if (filename != NULL) {
        strcat(offset, "/");
        strcat(offset, filename);
    }

    free(path);
    return absolute_path;
}


char* aes_encrypt(int mode, int padding, const char* key_str, const char* iv_str, int ivOffset, int ivLen, uint8_t* buff, size_t* size, bool* is_text)
{
    crypto_err = NULL;
    uv_aes_t aes_ctx = {};
    uv_buf_t text = { 0, 0 };
    char* ret_str = NULL;

    do {
        CHECK_ERR_BREAK(key_str, "invalid parameter key");
        CHECK_ERR_BREAK(iv_str, "invalid parameter iv");
        CHECK_ERR_BREAK(buff, "crypto.aes invalid parameter text");

        text.base = (char*)buff;
        text.len = *size;
        *size = 0;

        if (uv_aes_init(&aes_ctx, mode, padding) != 0) {
            CHECK_ERR_BREAK(NULL, "crypto.aes aes_ctx init failed");
        }

        if (!setup_uv_aes(&aes_ctx, 1, (const unsigned char*)key_str, (const unsigned char*)iv_str, ivOffset, ivLen)) {
            CHECK_ERR_BREAK(NULL, "crypto.aes set base64 key or iv failed");
        }

        size_t out_len = 0;
        size_t out_size = text.len * 2 + 16;
        unsigned char* out_buff = (unsigned char*)alloca(out_size);
        memset(out_buff, 0, out_size);

        if (is_text) {
            if (uv_aes_encrypt_base64(&aes_ctx, (const unsigned char*)text.base, text.len, out_buff, out_size, &out_len) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.aes encrypt_base64 failed");
            }
        } else {
            if (uv_aes_encrypt(&aes_ctx, (const unsigned char*)text.base, text.len, out_buff, &out_len) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.aes encrypt failed");
            }
        }
        out_buff[out_len] = '\0';
        *size = out_size;
        ret_str = (char*)malloc((*size) * sizeof(char));
        memcpy(ret_str, out_buff, out_len);
        uv_aes_free(&aes_ctx);
        return ret_str;
    }while (false);

    uv_aes_free(&aes_ctx);
    return ret_str;
}

char* aes_decrypt(int mode, int padding, const char* key_str, const char* iv_str, int ivOffset, int ivLen, uint8_t* buff, size_t* size, bool* is_text)
{
    crypto_err = NULL;
    uv_aes_t aes_ctx = {};
    uv_buf_t text = { 0, 0 };
    char* ret_str = NULL;

    do {
        CHECK_ERR_BREAK(key_str, "invalid parameter key");
        CHECK_ERR_BREAK(iv_str, "invalid parameter iv");
        CHECK_ERR_BREAK(buff, "crypto.aes invalid parameter text");
        text.base = (char*)buff;
        text.len = *size;
        *size = 0;

        if (uv_aes_init(&aes_ctx, mode, padding) != 0) {
            CHECK_ERR_BREAK(NULL, "crypto.aes aes_ctx init failed");
        }

        if (!setup_uv_aes(&aes_ctx, 0, (const unsigned char*)key_str, (const unsigned char*)iv_str, ivOffset, ivLen)) {
            CHECK_ERR_BREAK(NULL, "crypto.aes set base64 key or iv failed");
        }

        size_t out_len = 0;
        unsigned char* out_buff = NULL;
        size_t out_size = text.len + 16;
        out_buff = (unsigned char*)alloca(out_size);
        memset(out_buff, 0, out_size);

        if (is_text) {
            if (uv_aes_decrypt_base64(&aes_ctx, (const unsigned char*)text.base, text.len, out_buff, &out_len) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.aes decrypt_base64 failed");
            }
        } else {
            if (uv_aes_decrypt(&aes_ctx, (const unsigned char*)text.base, text.len, out_buff, &out_len) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.aes decrypt failed");
                break;
            }
        }
        out_buff[out_len] = '\0';

        *size = out_size;
        ret_str = (char*)malloc((*size) * sizeof(char));
        memcpy(ret_str, out_buff, out_len);
        uv_aes_free(&aes_ctx);
        return ret_str;
    }while (false);

    uv_aes_free(&aes_ctx);
    return ret_str;
}

char* rsa_encrypt(const char* key_str, uint8_t* buff, size_t* buff_size, bool* is_text)
{
    crypto_err = NULL;
    uv_buf_t text = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t output = { 0 };
    uv_buf_t ret = { 0 };
    char* ret_str = NULL;

    do {
        key.base = (char*)key_str;
        key.len = strlen(key_str);
        CHECK_ERR_BREAK(key_str, "crypto.rsa invalid parameter key");
        CHECK_ERR_BREAK(buff, "crypto.aes invalid parameter text");
        text.base = (char*)buff;
        text.len = *buff_size;
        *buff_size = 0;

        if (uv_rsa(key, text, &output, UV_EXT_ENCRYPT) != 0) {
            CHECK_ERR_BREAK(NULL, "crypto.rsa encrypt failed");
        }
        if (is_text) {
           if (uv_base64_encode(output, &ret) != 0) {
               CHECK_ERR_BREAK(NULL, "crypto.rsa encode base64 failed");
           }
            *buff_size = ret.len;
            ret_str = (char*)malloc(ret.len + 1);
            sprintf(ret_str, "%s", ret.base);
        } else {
            *buff_size = output.len;
            ret_str = (char*)malloc(output.len);
            memcpy(ret_str, output.base, output.len);
        }
        if (output.base) free(output.base);
        if (ret.base) free(ret.base);
        return ret_str;
    } while (false);

    if (output.base) free(output.base);
    if (ret.base) free(ret.base);
    return ret_str;
}

char* rsa_decrypt(const char* key_str, uint8_t* buff, size_t* buff_size, bool* is_text)
{
    crypto_err = NULL;
    uv_buf_t text = { 0 };
    uv_buf_t key = { 0 };
    uv_buf_t input = { 0 };
    uv_buf_t output = { 0 };
    char* ret_str = NULL;

    do {
        key.base = (char*)key_str;
        key.len = strlen(key_str);
        CHECK_ERR_BREAK(key_str, "crypto.rsa invalid parameter key");
        CHECK_ERR_BREAK(buff, "crypto.aes invalid parameter text");
        text.base = (char*)buff;
        text.len = *buff_size;
        *buff_size = 0;

        if (is_text) {
            if (uv_base64_decode(text, &input)) {
                CHECK_ERR_BREAK(NULL, "crypto.rsa decode base64 failed");
            }
            if (uv_rsa(key, input, &output, UV_EXT_DECRYPT) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.rsa decrypt failed");
                break;
            }
        } else {
            if (uv_rsa(key, text, &output, UV_EXT_DECRYPT) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.rsa decrypt failed");
            }
            *buff_size = output.len;
            ret_str = (char*)malloc(output.len);
            memcpy(ret_str, output.base, output.len);
        }
        *buff_size = output.len;
        ret_str = (char*)malloc(output.len);
        memcpy(ret_str, output.base, output.len);

        if (input.base) free(input.base);
        if (output.base) free(output.base);
        return ret_str;
    } while (false);

    if (input.base) free(input.base);
    if (output.base) free(output.base);
    return ret_str;
}

bool rsa_verify(const char* type_str, const char* key_str, uint8_t* buff, size_t buff_size, uint8_t* sig_buf, size_t seg_size,  bool sig_text)
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
            if (uv_base64_decode(sig, &md)) {
                CHECK_ERR_BREAK(NULL, "crypto.verify base64 failed");
            }
            res = uv_verify(type.base, key, text, md, UV_EXT_TYPE_BUFFER);
            free(md.base);
        } else {
            res = uv_verify(type.base, key, text, sig, UV_EXT_TYPE_BUFFER);
        }
        return res == 0;
    } while (false);

    if (sig_text)
        free(md.base);

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

        if (uv_base64_decode(md_64, &md) == 0) {
            ret = uv_verify(type.base, key, text, md, UV_EXT_TYPE_FILE) == 0;
        } else {
            CHECK_ERR_BREAK(NULL, "crypto.verify base64 failed");
        }
    } while (false);

    free(text.base);
    free(md.base);
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
            res = uv_base64_encode(text, &out);
        } else {
            res = uv_base64_decode(text, &out);
        }

        if (res != 0) {
            if (out.base)
                free(out.base);
            CHECK_ERR_RET(NULL, "crypto.base64 calculate failed");
        }

        char* ret_str = (char*)FeatureMalloc(out.len, FT_CHAR);
        sprintf(ret_str, "%s", out.base);
        if (out.base)
            free(out.base);
        return ret_str;
    } while (false);

    return NULL;
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
            if (uv_base64_encode(out, &ret) != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.sign base64 failed");
                break;
            }
            *buff_size = ret.len + 1;
            ret_str = (char*)malloc((*buff_size) * sizeof(char));
            sprintf(ret_str, "%s", ret.base);
        } else {
            *buff_size = out.len;
            ret_str = (char*)malloc(out.len * sizeof(char));
	    memcpy(ret_str, out.base, out.len);
        }

        if (out.base) free(out.base);
        if (ret.base) free(ret.base);
        return ret_str;
    }while (false);

    if (out.base) free(out.base);
    if (ret.base) free(ret.base);
    return NULL;
}

char* rsa_sign_file(const char* type_str, const char* key_str, const char* uri_str, const char* pkg_str)
{
    crypto_err = NULL;
    FEATURE_LOG_INFO("%s\n", file_tag);
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

        if (uv_sign(type.base, key, text, &out, UV_EXT_TYPE_FILE)) {
            CHECK_ERR_BREAK(NULL, "crypto.sign invalid parameter key");
        }
        if (uv_base64_encode(out, &ret) != 0) {
            CHECK_ERR_BREAK(NULL, "crypto.sign base64 failed");
            break;
        }
        char* ret_str = (char*)FeatureMalloc(ret.len, FT_CHAR);
        sprintf(ret_str, "%s", ret.base);

        if (text.base) free(text.base);
        if (out.base) free(out.base);
        if (ret.base) free(ret.base);
        return ret_str;
    } while (false);

    if (text.base) free(text.base);
    if (out.base) free(out.base);
    if (ret.base) free(ret.base);
    return NULL;
}

char* digest(const char* type_str, uint8_t* text_str, size_t text_size, const char* key_str)
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
            key.len = strlen(key_str);
            res = uv_md_hmac(type.base, text, &out, &key);
            if (res != 0) {
                CHECK_ERR_BREAK(NULL, "crypto.digest hmac calculate failed");
            }
        }
        uv_hexify(out, &ret);

        char* ret_str = (char*)FeatureMalloc(ret.len, FT_CHAR);
        sprintf(ret_str, "%s", ret.base);
        if (out.base) free(out.base);
        if (ret.base) free(ret.base);
        return ret_str;
    } while (false);

    if (out.base) free(out.base);
    if (ret.base) free(ret.base);
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
        char* ret_str = (char*)FeatureMalloc(ret.len, FT_CHAR);
        sprintf(ret_str, "%s", ret.base);

        free(abs_path);
        free(out.base);
        free(ret.base);
        return ret_str;
    } while (false);

    free(out.base);
    free(ret.base);
    return NULL;
}

