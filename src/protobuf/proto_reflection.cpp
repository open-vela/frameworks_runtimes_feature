#include "proto_reflection.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "feature_utils.h"
#include "proto_utils.h"
#include "protobuf-c/protobuf-c.h"
#include "quickjs/quickjs.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace proto_reflection {

/**
 * @brief convert protobuf message field to JSValue
 *
 * @param ctx
 * @param value
 * @param pnative
 * @param pFieldDesc
 * @param label
 * @return true
 * @return false
 */
bool fromProtoField(JSContext* ctx, JSValue& value, void* pnative, const ProtobufCFieldDescriptor* pFieldDesc, ProtobufCLabel label, bool needCaculateOffset);

/**
 * @brief create protobuf message from JSValue
 *
 * @param ctx
 * @param value
 * @param pnative
 * @param pFieldDesc
 * @param label
 * @param calcOffset
 * @return true
 * @return false
 */
bool toProtoField(FeatureInstanceHandle handle, JSContext* ctx, JSValue value, void* pnative, const ProtobufCFieldDescriptor* pFieldDesc, ProtobufCLabel label, bool calcOffset);

bool fromProtoField(JSContext* ctx, JSValue& value, void* pnative, const ProtobufCFieldDescriptor* pFieldDesc, ProtobufCLabel label, bool calcOffset)
{
    value = JS_UNDEFINED;
    switch (label) {
    case PROTOBUF_C_LABEL_REQUIRED: {
        // check required
    } break;
    case PROTOBUF_C_LABEL_OPTIONAL: {
        // check optional
    } break;
    case PROTOBUF_C_LABEL_REPEATED: {
        // if repeated, get container of pnative
        size_t num = proto_utils::getValue<size_t>(pnative, pFieldDesc->quantifier_offset);
        if (num) {
            // for repeated properties,
            value = JS_NewArray(ctx);
            uint8_t* valPtr = proto_utils::getValue<uint8_t*>(pnative, pFieldDesc->offset);
            int itemSize = proto_utils::getProtoTypeSize(pFieldDesc->type);
            for (size_t i = 0; i < num; i++) {
                // remove repeated label
                JSValue arrayItem = JS_UNDEFINED;
                void* itemPtr = valPtr + i * itemSize;
                if (!fromProtoField(ctx, arrayItem, itemPtr, pFieldDesc, PROTOBUF_C_LABEL_NONE, false)) {
                    FEATURE_LOG_ERROR("convert repeated field %ld failed !", i);
                    JS_FreeValue(ctx, arrayItem);
                    return false;
                }
                JS_SetPropertyUint32(ctx, value, i, arrayItem);
            }
        }
        return true;
    } break;
    case PROTOBUF_C_LABEL_NONE: {
        // none
    } break;
    default: {
        FEATURE_LOG_ERROR("invalid label !");
    }
    }
    switch (pFieldDesc->type) {
    case PROTOBUF_C_TYPE_MESSAGE: {
        ProtobufCMessageDescriptor* message_descriptor = (ProtobufCMessageDescriptor*)pFieldDesc->descriptor;
        FEATURE_CHECK_NE(message_descriptor, nullptr);
        ProtobufCMessage* pMsg = calcOffset ? proto_utils::getValue<ProtobufCMessage*>(pnative, pFieldDesc->offset) : *(ProtobufCMessage**)pnative;
        value = JS_NewObject(ctx);
        if (!fromNative(ctx, value, pMsg)) {
            FEATURE_LOG_ERROR("convert %s failed !", pFieldDesc->name);
            return false;
        }
    } break;
    case PROTOBUF_C_TYPE_INT32:
    case PROTOBUF_C_TYPE_SINT32:
    case PROTOBUF_C_TYPE_SFIXED32:
    case PROTOBUF_C_TYPE_UINT32:
    case PROTOBUF_C_TYPE_FIXED32: {
        int32_t val = calcOffset ? proto_utils::getValue<int32_t>(pnative, pFieldDesc->offset) : *(int32_t*)pnative;
        value = JS_NewInt32(ctx, val);
    } break;
    case PROTOBUF_C_TYPE_INT64:
    case PROTOBUF_C_TYPE_SINT64:
    case PROTOBUF_C_TYPE_SFIXED64:
    case PROTOBUF_C_TYPE_UINT64:
    case PROTOBUF_C_TYPE_FIXED64: {
        int64_t val = calcOffset ? proto_utils::getValue<int64_t>(pnative, pFieldDesc->offset) : *(int64_t*)pnative;
        value = JS_NewInt64(ctx, val);
    } break;

    case PROTOBUF_C_TYPE_FLOAT: {
        float val = calcOffset ? proto_utils::getValue<float>(pnative, pFieldDesc->offset) : *(float*)pnative;
        value = JS_NewFloat64(ctx, val);
    } break;
    case PROTOBUF_C_TYPE_DOUBLE: {
        double val = calcOffset ? proto_utils::getValue<double>(pnative, pFieldDesc->offset) : *(double*)pnative;
        value = JS_NewFloat64(ctx, val);
    } break;
    case PROTOBUF_C_TYPE_BOOL: {
        bool val = calcOffset ? proto_utils::getValue<bool>(pnative, pFieldDesc->offset) : *(bool*)pnative;
        value = JS_NewBool(ctx, val);
    } break;
    case PROTOBUF_C_TYPE_ENUM: {
        int enumVal = calcOffset ? proto_utils::getValue<int>(pnative, pFieldDesc->offset) : *(int*)pnative;
#ifdef CONFIG_PROTOBUF_VALIDATE
        ProtobufCEnumDescriptor* enumDescriptor = (ProtobufCEnumDescriptor*)pFieldDesc->descriptor;
        bool found = false;
        for (size_t i = 0; i < enumDescriptor->n_values; i++) {
            const ProtobufCEnumValue* enumValue = &enumDescriptor->values[i];
            if (enumValue->value == enumVal) {
                found = true;
                break;
            }
        }
        if (found) {
            value = JS_NewInt32(ctx, enumVal);
        }
#else
        value = JS_NewInt32(ctx, enumVal);
#endif
    } break;
    case PROTOBUF_C_TYPE_STRING: {
        const char* val = calcOffset ? proto_utils::getValue<const char*>(pnative, pFieldDesc->offset) : *(const char**)pnative;
        value = JS_NewString(ctx, val);
    } break;
    case PROTOBUF_C_TYPE_BYTES: { // array buffer????
        ProtobufCBinaryData* pBinaryData = calcOffset ? proto_utils::getValuePtr<ProtobufCBinaryData>(pnative, pFieldDesc->offset) : (ProtobufCBinaryData*)pnative;
        value = JS_NewArrayBufferCopy(ctx, pBinaryData->data, pBinaryData->len);
    } break;
    }
    return true;
}

bool fromNative(JSContext* ctx, JSValue& value, void* pnative)
{
    ProtobufCMessage* pProtoMsg = (ProtobufCMessage*)pnative;
    if (!protobuf_c_message_check(pProtoMsg)) {
        FEATURE_LOG_ERROR("invalid protobuf message !");
        return false;
    }
    const ProtobufCMessageDescriptor* pProtoDesc = pProtoMsg->descriptor;
    FEATURE_CHECK_NE(pProtoDesc, nullptr);
    for (size_t i = 0; i < pProtoDesc->n_fields; i++) {
        const ProtobufCFieldDescriptor* pFieldDesc = &pProtoDesc->fields[i];
        JSValue propValue = JS_UNDEFINED;
        if (proto_utils::isOneOfSkipped(pFieldDesc, pnative)) {
            continue;
        }
        if (!fromProtoField(ctx, propValue, pnative, pFieldDesc, pFieldDesc->label, true)) {
            FEATURE_LOG_ERROR("convert target field failed !");
            return false;
        }
        JS_SetPropertyStr(ctx, value, pFieldDesc->name, propValue);
    }
    return true;
}

bool toProtoField(FeatureInstanceHandle handle, JSContext* ctx, JSValue value, void* pnative, const ProtobufCFieldDescriptor* pFieldDesc, ProtobufCLabel label, bool calcOffset)
{
    switch (label) {
    case PROTOBUF_C_LABEL_REQUIRED: {
        if (JS_IsException(value)) {
            FEATURE_LOG_ERROR("required field: %s not found !", pFieldDesc->name);
            return false;
        }
    } break;
    case PROTOBUF_C_LABEL_OPTIONAL: {

    } break;
    case PROTOBUF_C_LABEL_REPEATED: {
        // process repeated array
        if (!JS_IsArray(ctx, value)) {
            FEATURE_LOG_ERROR("repeated value must be Array !");
            return false;
        }

        JSValue lengVal = JS_GetPropertyStr(ctx, value, "length");
        size_t* pSize = proto_utils::getValuePtr<size_t>(pnative, pFieldDesc->quantifier_offset);
        uint8_t** pMsgArray = proto_utils::getValuePtr<uint8_t*>(pnative, pFieldDesc->offset);
        uint32_t length;
        JS_ToUint32(ctx, &length, lengVal);
        JS_FreeValue(ctx, lengVal);
        *pSize = length;
        int itemSize = proto_utils::getProtoTypeSize(pFieldDesc->type);
        *pMsgArray = (uint8_t*)FeatureInstanceAlloc(nullptr, itemSize * length);
        memset(*pMsgArray, 0, itemSize * length);
        for (size_t i = 0; i < *pSize; i++) {
            JSValue itemVal = JS_GetPropertyUint32(ctx, value, i);
            void* itemPtr = (*pMsgArray + itemSize * i);
            bool ret = toProtoField(handle, ctx, itemVal, itemPtr, pFieldDesc, PROTOBUF_C_LABEL_NONE, false);
            JS_FreeValue(ctx, itemVal);
            if (!ret) {
                FEATURE_LOG_ERROR("convert repeated item %s[%lu] failed !", pFieldDesc->name, i);
                return false;
            }
        }
        return true;
    } break;
    case PROTOBUF_C_LABEL_NONE: {

    } break;
    }

    switch (pFieldDesc->type) {
    case PROTOBUF_C_TYPE_MESSAGE: {
        ProtobufCMessageDescriptor* pDescriptor = (ProtobufCMessageDescriptor*)pFieldDesc->descriptor;
        ProtobufCMessage** pMsg = calcOffset ? proto_utils::getValuePtr<ProtobufCMessage*>(pnative, pFieldDesc->offset) : (ProtobufCMessage**)pnative;
        if (!toNative(handle, ctx, value, pDescriptor, pMsg)) {
            FEATURE_LOG_ERROR("convert %s failed !", pFieldDesc->name);
            return false;
        }
    } break;
    case PROTOBUF_C_TYPE_INT32:
    case PROTOBUF_C_TYPE_SINT32:
    case PROTOBUF_C_TYPE_SFIXED32:
    case PROTOBUF_C_TYPE_UINT32:
    case PROTOBUF_C_TYPE_FIXED32: {
        int32_t* val = calcOffset ? proto_utils::getValuePtr<int32_t>(pnative, pFieldDesc->offset) : (int32_t*)pnative;
        JS_ToInt32(ctx, val, value);
    } break;
    case PROTOBUF_C_TYPE_INT64:
    case PROTOBUF_C_TYPE_SINT64:
    case PROTOBUF_C_TYPE_SFIXED64:
    case PROTOBUF_C_TYPE_UINT64:
    case PROTOBUF_C_TYPE_FIXED64: {
        int64_t* val = calcOffset ? proto_utils::getValuePtr<int64_t>(pnative, pFieldDesc->offset) : (int64_t*)pnative;
        JS_ToInt64(ctx, val, value);
    } break;

    case PROTOBUF_C_TYPE_FLOAT: {
        float* val = calcOffset ? proto_utils::getValuePtr<float>(pnative, pFieldDesc->offset) : (float*)pnative;
        double d;
        JS_ToFloat64(ctx, &d, value);
        *val = (float)d;
    } break;
    case PROTOBUF_C_TYPE_DOUBLE: {
        double* val = calcOffset ? proto_utils::getValuePtr<double>(pnative, pFieldDesc->offset) : (double*)pnative;
        JS_ToFloat64(ctx, val, value);
    } break;
    case PROTOBUF_C_TYPE_BOOL: {
        bool* val = calcOffset ? proto_utils::getValuePtr<bool>(pnative, pFieldDesc->offset) : (bool*)pnative;
        *val = JS_ToBool(ctx, value);
    } break;
    case PROTOBUF_C_TYPE_ENUM: {
        int* val = calcOffset ? proto_utils::getValuePtr<int>(pnative, pFieldDesc->offset) : (int*)pnative;
        int32_t i32 = 0;
        JS_ToInt32(ctx, &i32, value);
        *val = (int)i32;
    } break;
    case PROTOBUF_C_TYPE_STRING: {
        const char** pStr = calcOffset ? proto_utils::getValuePtr<const char*>(pnative, pFieldDesc->offset) : (const char**)pnative;
        const char* str = JS_ToCString(ctx, value);

        auto len = strlen(str);
        void* buf = FeatureInstanceAlloc(nullptr, len + 1);
        memcpy(buf, str, len + 1);

        *pStr = (char*)buf;
        JS_FreeCString(ctx, str);
    } break;
    case PROTOBUF_C_TYPE_BYTES: {
        ProtobufCBinaryData* pBinaryData = calcOffset ? proto_utils::getValuePtr<ProtobufCBinaryData>(pnative, pFieldDesc->offset) : (ProtobufCBinaryData*)pnative;
        if (JS_IsString(value)) {
            size_t len = 0;
            const char* buf = JS_ToCStringLen(ctx, &len, value);
            uint8_t* new_buf = (uint8_t*)FeatureInstanceAlloc(nullptr, len);
            memcpy(new_buf, buf, len);
            JS_FreeCString(ctx, buf);
            pBinaryData->data = new_buf;
            pBinaryData->len = len;
        } else {
            // ArrayBuffer, copy binary data
            size_t len = 0;
            uint8_t* buf = JS_GetArrayBuffer(ctx, &len, value);
            if (len && buf) {
                uint8_t* new_buf = (uint8_t*)FeatureInstanceAlloc(nullptr, len);
                memcpy(new_buf, buf, len);
                pBinaryData->data = new_buf;
                pBinaryData->len = len;
            }
        }
    } break;
    }

    return true;
}

static const ProtobufCFieldDescriptor* binarySearchField(const char* name, const ProtobufCMessageDescriptor* descriptor)
{
    if (descriptor->n_fields == 0)
        return nullptr;
    int low = 0, high = descriptor->n_fields - 1, mid = 0;
    while (low <= high) {
        mid = (low + high) / 2;
        auto pField = &descriptor->fields[descriptor->fields_sorted_by_name[mid]];
        int r = strcmp(pField->name, name);
        if (!r) {
            return pField;
        } else if (r < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return nullptr;
}

/**
 * @brief convert from js to protobuf
 *
 * @param ctx
 * @param value
 * @param descriptor
 * @param message
 * @return true
 * @return false
 */
bool toNative(FeatureInstanceHandle handle, JSContext* ctx, JSValue value, const ProtobufCMessageDescriptor* descriptor, ProtobufCMessage** message)
{
    bool ret = true;
    *message = nullptr;
    ProtobufCMessage* pProtoMsg = proto_utils::create(handle, descriptor);
    FEATURE_CHECK_NE(pProtoMsg, nullptr);
    JSPropertyEnum* ptab = nullptr;
    uint32_t len = 0;
    if (!JS_GetOwnPropertyNames(ctx, &ptab, &len, value, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK | JS_GPN_ENUM_ONLY)) {
        for (uint32_t i = 0; i < len; i++) {
            const char* propName = JS_AtomToCString(ctx, ptab[i].atom);
            const ProtobufCFieldDescriptor* pFieldDesc = binarySearchField(propName, descriptor);
            JS_FreeCString(ctx, propName);
            if (!pFieldDesc) {
                FEATURE_LOG_ERROR("propName: %s coudn't find protobuf field !", propName);
                ret = false;
                break;
            }
            if (pFieldDesc->flags & PROTOBUF_C_FIELD_FLAG_ONEOF) {
                // check if id = 0 (not set)
                int* pOneofTypeId = proto_utils::getValuePtr<int>(pProtoMsg, pFieldDesc->quantifier_offset);
                if (*pOneofTypeId) {
                    FEATURE_LOG_ERROR("current oneof field %s already set to %d!", pFieldDesc->name, *pOneofTypeId);
                    ret = false;
                    break;
                }
                *pOneofTypeId = pFieldDesc->id;
            }
            JSValue propValue = JS_GetProperty(ctx, value, ptab[i].atom);
            ret = toProtoField(handle, ctx, propValue, pProtoMsg, pFieldDesc, pFieldDesc->label, true);
            JS_FreeValue(ctx, propValue);
            if (!ret) {
                FEATURE_LOG_ERROR("convert field: %s failed !", pFieldDesc->name);
                break;
            }
        }
    }
    if (len && ptab) {
        for (uint32_t i = 0; i < len; i++) {
            JS_FreeAtom(ctx, ptab[i].atom);
        }
        free(ptab);
    }
    if (!ret) {
        proto_utils::release(pProtoMsg);
    } else {
        *message = pProtoMsg;
    }
    return ret;
}
}