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
#include "feature_instance_qjs.h"
#include "feature_log.h"
#include "feature_utils.h"
#include "feature.h"
#include "feature_ffi.h"
#include "feature_context_private.h"
#include "feature_context_qjs.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <ffi.h>
#include <string.h>
#include <strings.h>
#include <tuple>

using namespace FEATURE;

#define CFUNCDATA_FN(f) ((feature_value_t(*)(feature_context_ref ctx, feature_value_t, int, feature_value_t*, int, feature_value_t*))f)

namespace ferry {

FeatureInstanceQjs::FeatureInstanceQjs(FeaturePrototype* proto)
    : FeatureInstance(proto)
{
}

FeatureInstanceQjs::~FeatureInstanceQjs()
{
}

int FeatureInstanceQjs::invokeFeatureCallback(
                    const ferry::CallbackType& callbackType,
                    feature_value_t callback,
                    va_list& ap,
                    int method_param_count,
                    int rest_param_count)
{
    JSContext* js_ctx = (JSContext*)ft_context_get_data(prototype()->ft_ctx);
    bool got_error = false;
    feature_value_t ret = FEATURE_VALUE_UNDEFINED;

    if (feature_is_undefined(callback)) {
        FEATURE_LOG_ERROR("callback in undefined !");
        return -1;
    }
    // create argv list and initialize to undefined
    feature_value_t* argv = new feature_value_t[method_param_count + rest_param_count];
    for (int i = 0; i < method_param_count + rest_param_count; i++) {
        argv[i] = FEATURE_VALUE_UNDEFINED;
    }

    do {
        // convert parameters to feature_value_t
        for (int i = 0; i < method_param_count; i++) {
            FeatureType featureType = callbackType.parameters[i];
            void* ptr = ferry::FeatureFFI::exactVariadicParameter(ap, featureType);
            if (!ptr) {
                got_error = true;
                break;
            }
            if (!ferry::FeatureFFI::convertValueToGuest(this, featureType, ptr, js_ctx, argv[i])) {
                FEATURE_LOG_ERROR("convert callback param failed !");
                free(ptr);
                got_error = true;
                break;
            }
            free(ptr);
        }
        if (got_error) {
            FEATURE_LOG_ERROR("invoke callback failed !");
            break;
        }
        // prepare for rest parameters
        for (int i = method_param_count; i < method_param_count + rest_param_count; i++) {
            // it must be FtMalloced.
            void* arg = va_arg(ap, void*);
            void* header_ptr = ((char*)arg - FT_OBJ_HEADER_SIZE);
            ferry::FTObjHeader* header = (ferry::FTObjHeader*)header_ptr;
            if (!ferry::FeatureFFI::convertValueToGuest(this, header->featureType, arg, js_ctx, argv[i])) {
                FEATURE_LOG_ERROR("convert callback rest param failed !");
                argv[i] = FEATURE_VALUE_UNDEFINED;
            }
        }

        ret = feature_call(js_ctx, callback, FEATURE_VALUE_UNDEFINED, method_param_count + rest_param_count, argv);
    } while (0);
    for (int i = 0; i < method_param_count + rest_param_count; i++) {
        feature_free_value(js_ctx, argv[i]);
    }
    delete[] argv;
    /*
    if (callbackType.return_type != ferry::FT_VOID && ret_value && !jse_is_undefined(ret)) {
        // allocate ret_value first
        ffi_type* ret_type = nullptr;
        if (!ferry::FeatureFFI::createTypeDeclaration(callbackType.return_type, ret_type)) {
            ferry::FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(*ret_value);
            *ret_value = nullptr;
            return -1;
        }
        if (!ferry::FeatureFFI::convertValueToHost(instance, callbackType.return_type, *ret_value, js_ctx, ret)) {
            ferry::FeatureFFI::freeTypeDeclaration(ret_type);
            FreeFeatureValue(*ret_value);
            *ret_value = nullptr;
            return -1;
        }
        ferry::FeatureFFI::freeTypeDeclaration(ret_type);
        // for reference type, remove the pointer's pointer.
        if (FT_IS_REFERENCE(callbackType.return_type)) {
            auto result = **(void***)ret_value;
            free(*ret_value);
            *ret_value = result;
        }
    }
*/
    feature_free_value(js_ctx, ret);

    return 0;
}

}

