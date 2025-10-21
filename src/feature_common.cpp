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

#include "feature_common.h"
#include "feature_context_private.h"
#include "feature_description.h"
#include "feature_exports.h"
#include <cstdint>

int getAlignedCount(const FeatureType param)
{
    if (sizeof(void*) == 8) {
        // in 64bit system
        return 1;
    }
    // 32bit system
    int size = 1;
    if (FT_IS_PRIMITIVE(param)) {
        // FT_INT means FT_INT32 currently.
        switch (param) {
        case FT_DOUBLE:
        case FT_INT64:
        case FT_UINT64:
            size = 2;
            break;
        default:
            break;
        }
    } else if (FT_IS_COMPLEX(param)) {
        ComplexTypeHeader* header = (ComplexTypeHeader*)FT_GET_COMPLEX(param);
        switch (header->type) {
        case COMPLEX_OPTIONAL: {
            OptionalType* opt_type = (OptionalType*)header;
            size = getAlignedCount(opt_type->type);
        } break;
        default: {
        } break;
        }
    }
    return size;
}

int getParamCount(const FeatureType* param, bool* hasRest, int* optional_size, int32_t* param_int_count)
{
    int count = 0;
    if (param_int_count) {
        *param_int_count = 0;
    }
    if (optional_size) {
        *optional_size = 0;
    }
    while (param && *param && *param != FT_PARAM_REST_END) {
        count++;
        if (param_int_count) {
            *param_int_count += getAlignedCount((FeatureType)*param);
        }
        if (optional_size && FT_IS_COMPLEX(*param)) {
            ComplexTypeHeader* complexHeader = (ComplexTypeHeader*)FT_GET_COMPLEX(*param);
            if (complexHeader->type == COMPLEX_OPTIONAL) {
                *optional_size = *optional_size + 1;
            }
        }
        param++;
    }
    if (hasRest) {
        *hasRest = param ? (*param == FT_PARAM_REST_END) : false;
    }
    return count;
}

int countMember(ObjectMember* member)
{
    int count = 0;
    while (member->name) {
        count++;
        member++;
    }
    return count;
}

int getValueSize(FeatureType featureType)
{
    featureType = FT_GET_REAL_TYPE(featureType);
    if (FT_IS_REFERENCE(featureType)) {
        return sizeof(uintptr_t);
    } else if (FT_IS_PRIMITIVE(featureType)) {
        switch (featureType) {
        case FT_VOID: {
            return 0;
        } break;
        case FT_INT: {
            return sizeof(int);
        } break;
        case FT_INT8: {
            return sizeof(int8_t);
        } break;
        case FT_UINT8: {
            return sizeof(uint8_t);
        } break;
        case FT_INT16: {
            return sizeof(int16_t);
        } break;
        case FT_UINT16: {
            return sizeof(uint16_t);
        } break;
        case FT_INT32: {
            return sizeof(int32_t);
        } break;
        case FT_UINT32: {
            return sizeof(uint32_t);
        } break;
        case FT_INT64: {
            return sizeof(int64_t);
        } break;
        case FT_UINT64: {
            return sizeof(uint64_t);
        } break;
        case FT_DOUBLE: {
            return sizeof(double);
        } break;
        case FT_FLOAT: {
            return sizeof(float);
        } break;
        case FT_BOOLEAN: {
            return sizeof(bool);
        } break;
        case FT_STRING: {
            return sizeof(char*);
        } break;
        case FT_ANY_REF: {
            return sizeof(ft_value_t*);
        } break;
        default: {
            FEATURE_LOG_WARN("unsupported type detected !");
            return 0;
        }
        }
    } else if (FT_IS_COMPLEX(featureType)) {
        // allocate complex type
        ComplexTypeHeader* complexType = (ComplexTypeHeader*)FT_GET_COMPLEX(featureType);
        if (complexType->type == COMPLEX_CALLBACK) {
            return sizeof(FtCallbackId);
        } else {
            FEATURE_LOG_WARN("unsupported complex type detected !");
            return 0;
        }
    }
    return 0;
}

int FeatureTypeGetValueSize(FeatureType ft)
{
    return FT_IS_REFERENCE(ft) ? sizeof(uintptr_t) : getValueSize(ft);
}

bool convertOptional(OptionalType* opt, void* out)
{
    FEATURE_CHECK_NE(out, nullptr);
    FeatureType ftype = opt->type;
    if (FT_IS_PRIMITIVE(ftype)) {
        switch (ftype) {
        case FT_VOID: {
            FEATURE_LOG_ERROR("void not supported !");
            return false;
        } break;
        case FT_BOOLEAN: {
            *((bool*)out) = opt->ival;
        } break;
        case FT_INT: {
            *((int32_t*)out) = opt->ival;
        } break;
        case FT_INT64: {
            *((int64_t*)out) = opt->lval;
        } break;
        case FT_UINT32: {
            *((uint32_t*)out) = opt->uval;
        } break;
        case FT_UINT64: {
            *((uint64_t*)out) = opt->ulval;
        } break;
        case FT_FLOAT: {
            *((float*)out) = (float)(opt->fval);
        } break;
        case FT_DOUBLE: {
            *((double*)out) = opt->fval;
        } break;
        case FT_STRING: {
            if (!opt->str)
                break;
            char* str = (char*)FeatureMalloc(strlen(opt->str) + 1, FT_STRING);
            strcpy(str, opt->str);
            *((const char**)out) = str;
        } break;
        case FT_ANY_REF: {
            *((void**)out) = opt->ptr;
        } break;
        case FT_JSON_OBJ: {
            *((void**)out) = opt->ptr;
        } break;
        default: {
            FEATURE_LOG_WARN("unsupported type detected !");
            return false;
        } break;
        }
    } else if (FT_IS_COMPLEX(ftype)) {
        *((void**)out) = opt->ptr;
    }
    return true;
}

FtCallbackId findCallbackIdByName(FeatureType ftype, void* pnative, const char* name)
{
    FEATURE_CHECK_NE(pnative, 0);
    if (FT_IS_PRIMITIVE(ftype)) {
        return 0;
    }

    ftype = FT_GET_REAL_TYPE(ftype);
    ComplexTypeHeader* cmplx_header = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
    if (cmplx_header->type != COMPLEX_STRUCT_MAP) {
        return 0;
    }

    void* ptr = *(void**)pnative;
    if (!ptr) {
        return 0;
    }

    ObjectMember* members = ((ObjectMapType*)cmplx_header)->members;
    auto count = countMember(members);
    for (int i = 0; i < count; i++) {
        auto member = &members[i];
        if (FT_IS_PRIMITIVE(member->type))
            continue;
        if (strcmp(member->name, name) != 0)
            continue;
        void* member_ptr = (void*)((char*)ptr + member->offset);
        ComplexTypeHeader* header = (ComplexTypeHeader*)FT_GET_COMPLEX(member->type);
        if (header->type == COMPLEX_OPTIONAL) {
            FeatureType opt_type = ((OptionalType*)header)->type;
            if (FT_IS_PRIMITIVE(opt_type)) {
                return 0;
            }
            ComplexTypeHeader* opt_header = (ComplexTypeHeader*)FT_GET_COMPLEX(opt_type);
            if (opt_header->type != COMPLEX_CALLBACK) {
                return 0;
            }
            return *(FtCallbackId*)member_ptr;
        } else if (header->type == COMPLEX_CALLBACK) {
            return *(FtCallbackId*)member_ptr;
        }
    }
    return 0;
}

void freeFtValue(ft_context_ref ft_ctx, FeatureType ftype, void* pnative)
{
    if (!pnative || !(*(void**)pnative)) {
        return;
    }
    if (FT_IS_PRIMITIVE(ftype) && ftype == FT_ANY_REF) {
        ft_free_value(ft_ctx, *(ft_value_t*)(*(void**)pnative));
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complex_type->type) {
        case COMPLEX_STRUCT_MAP: {
            ObjectMapType& obj_map_type = *(ObjectMapType*)complex_type;
            auto member = obj_map_type.members;
            auto member_count = countMember(member);
            void* struct_ptr = *(void**)pnative;
            for (int i = 0; i < member_count; i++) {
                FeatureType mtype = FT_GET_REAL_TYPE(member->type);
                if (FT_NEED_FREE(mtype)) {
                    void* member_ptr = (void*)((char*)struct_ptr + member->offset);
                    freeFtValue(ft_ctx, mtype, member_ptr);
                }
                member++;
            }
        } break;
        case COMPLEX_OPTIONAL: {
            FEATURE_LOG_ERROR("unreachable for COMPLEX_OPTIONAL in freeFtValue !");
        } break;
        case COMPLEX_ARRAY: {
            FtArray* array = *(FtArray**)pnative;
            if (!array) {
                return;
            }
            auto elem_type = ((ArrayType*)complex_type)->element_type;
            size_t elem_size = FT_IS_REFERENCE(elem_type) ? sizeof(uintptr_t) : getValueSize(elem_type);
            for (int32_t i = 0; i < array->_size; i++) {
                void* elem_ptr = ((char*)array->_element + elem_size * i);
                freeFtValue(ft_ctx, elem_type, elem_ptr);
            }
        } break;
        default:
            break;
        }
    }
}

void dupFtValue(ft_context_ref ft_ctx, FeatureType ftype, void* pnative)
{
    if (!pnative || !(*(void**)pnative)) {
        return;
    }
    if (FT_IS_PRIMITIVE(ftype) && ftype == FT_ANY_REF) {
        ft_dup_value(ft_ctx, *(ft_value_t*)(*(void**)pnative));
    } else if (FT_IS_COMPLEX(ftype)) {
        ComplexTypeHeader* complex_type = (ComplexTypeHeader*)FT_GET_COMPLEX(ftype);
        switch (complex_type->type) {
        case COMPLEX_STRUCT_MAP: {
            ObjectMapType& obj_map_type = *(ObjectMapType*)complex_type;
            auto member = obj_map_type.members;
            auto member_count = countMember(member);
            void* struct_ptr = *(void**)pnative;
            for (int i = 0; i < member_count; i++) {
                FeatureType mtype = FT_GET_REAL_TYPE(member->type);
                if (FT_NEED_FREE(mtype)) {
                    void* member_ptr = (void*)((char*)struct_ptr + member->offset);
                    dupFtValue(ft_ctx, mtype, member_ptr);
                }
                member++;
            }
        } break;
        case COMPLEX_OPTIONAL: {
            FEATURE_LOG_ERROR("unreachable for COMPLEX_OPTIONAL in freeFtValue !");
        } break;
        case COMPLEX_ARRAY: {
            FtArray* array = *(FtArray**)pnative;
            if (!array) {
                return;
            }
            auto elem_type = ((ArrayType*)complex_type)->element_type;
            size_t elem_size = FT_IS_REFERENCE(elem_type) ? sizeof(uintptr_t) : getValueSize(elem_type);
            for (int32_t i = 0; i < array->_size; i++) {
                void* elem_ptr = ((char*)array->_element + elem_size * i);
                dupFtValue(ft_ctx, elem_type, elem_ptr);
            }
        } break;
        default:
            break;
        }
    }
}
