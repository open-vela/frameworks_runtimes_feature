#include "proto_utils.h"
#include "feature_utils.h"
#include "protobuf-c/protobuf-c.h"

namespace proto_utils {

void releaseField(const ProtobufCFieldDescriptor* pFieldDesc, void* pnative, ProtobufCLabel label, bool calcOffset)
{
    if (!pnative)
        return;
    if (label == PROTOBUF_C_LABEL_REPEATED) {
        // free array and sub msg
        size_t len = proto_utils::getValue<size_t>(pnative, pFieldDesc->quantifier_offset);
        uint8_t* pArray = proto_utils::getValue<uint8_t*>(pnative, pFieldDesc->offset);
        if (len && pArray) {
            bool isPtr = false;
            int itemSize = getProtoTypeSize(pFieldDesc->type, &isPtr);
            if (isPtr) {
                for (size_t i = 0; i < len; i++) {
                    void* itemPtr = pArray + i * itemSize;
                    releaseField(pFieldDesc, itemPtr, PROTOBUF_C_LABEL_NONE, false);
                }
            }
        }
        free(pArray);
    } else {
        switch (pFieldDesc->type) {
        case PROTOBUF_C_TYPE_INT32:
        case PROTOBUF_C_TYPE_SINT32:
        case PROTOBUF_C_TYPE_SFIXED32:
        case PROTOBUF_C_TYPE_INT64:
        case PROTOBUF_C_TYPE_SINT64:
        case PROTOBUF_C_TYPE_SFIXED64:
        case PROTOBUF_C_TYPE_UINT32:
        case PROTOBUF_C_TYPE_FIXED32:
        case PROTOBUF_C_TYPE_UINT64:
        case PROTOBUF_C_TYPE_FIXED64:
        case PROTOBUF_C_TYPE_FLOAT:
        case PROTOBUF_C_TYPE_DOUBLE:
        case PROTOBUF_C_TYPE_BOOL:
        case PROTOBUF_C_TYPE_ENUM:
            break;
        case PROTOBUF_C_TYPE_STRING: {
            char* val = calcOffset ? proto_utils::getValue<char*>(pnative, pFieldDesc->offset) : *(char**)pnative;
            if (val) {
                FeatureInstanceFreeValue(val);
            }
        } break;
        case PROTOBUF_C_TYPE_BYTES: {
            ProtobufCBinaryData* binaryData = calcOffset ? proto_utils::getValuePtr<ProtobufCBinaryData>(pnative, pFieldDesc->offset) : (ProtobufCBinaryData*)pnative;
            if (binaryData->len && binaryData->data) {
                FeatureInstanceFreeValue(binaryData->data);
            }

        } break;
        case PROTOBUF_C_TYPE_MESSAGE: {
            ProtobufCMessageDescriptor* message_descriptor = (ProtobufCMessageDescriptor*)pFieldDesc->descriptor;
            FEATURE_CHECK_NE(message_descriptor, nullptr);
            if (pnative) {
                ProtobufCMessage* pMsg = calcOffset ? proto_utils::getValue<ProtobufCMessage*>(pnative, pFieldDesc->offset) : *(ProtobufCMessage**)pnative;
                release(pMsg);
            }
        } break;
        }
    }
}

void release(ProtobufCMessage* pMsg)
{
    if (!pMsg) {
        FEATURE_LOG_WARN("pMsg is nullptr !");
        return;
    }
    const ProtobufCMessageDescriptor* pDescriptor = pMsg->descriptor;
    for (size_t i = 0; i < pDescriptor->n_fields; i++) {
        const ProtobufCFieldDescriptor* pFieldDesc = &pDescriptor->fields[i];
        if (proto_utils::isOneOfSkipped(pFieldDesc, pMsg)) {
            continue;
        }
        releaseField(pFieldDesc, pMsg, pFieldDesc->label, true);
    }
    FeatureInstanceFreeValue(pMsg);
}

int getProtoTypeSize(ProtobufCType type, bool* isPtr)
{
    if (isPtr)
        *isPtr = false;
    switch (type) {
    case PROTOBUF_C_TYPE_INT32:
    case PROTOBUF_C_TYPE_SINT32:
    case PROTOBUF_C_TYPE_SFIXED32:
        return 4;
    case PROTOBUF_C_TYPE_INT64:
    case PROTOBUF_C_TYPE_SINT64:
    case PROTOBUF_C_TYPE_SFIXED64:
        return 8;
    case PROTOBUF_C_TYPE_UINT32:
    case PROTOBUF_C_TYPE_FIXED32:
        return 4;
    case PROTOBUF_C_TYPE_UINT64:
    case PROTOBUF_C_TYPE_FIXED64:
        return 8;
    case PROTOBUF_C_TYPE_FLOAT:
        return 4;
    case PROTOBUF_C_TYPE_DOUBLE:
        return 8;
    case PROTOBUF_C_TYPE_BOOL:
        return 1;
    case PROTOBUF_C_TYPE_ENUM:
        return sizeof(int);
    case PROTOBUF_C_TYPE_STRING:
        if (isPtr)
            *isPtr = true;
        return sizeof(uintptr_t);
    case PROTOBUF_C_TYPE_BYTES:
        if (isPtr)
            *isPtr = true;
        return sizeof(ProtobufCBinaryData);
    case PROTOBUF_C_TYPE_MESSAGE:
        if (isPtr)
            *isPtr = true;
        return sizeof(uintptr_t);
    }
    return 0;
}

}
