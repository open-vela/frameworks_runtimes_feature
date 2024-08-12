#pragma once
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_types.h"
#include "protobuf-c/protobuf-c.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace proto_utils {

template <typename T, typename TOffset>
static inline T* getValuePtr(void* pnative, TOffset offset)
{
    return (T*)((uintptr_t)pnative + offset);
}

template <typename T, typename TOffset>
static inline T getValue(void* pnative, TOffset offset)
{
    return *(T*)((uintptr_t)pnative + offset);
}

inline ProtobufCMessage* create(FeatureInstanceHandle handle, const ProtobufCMessageDescriptor* descriptor)
{
    void* buffer = FeatureInstanceAllocProtobuf(handle, descriptor);
    protobuf_c_message_init(descriptor, buffer);
    return (ProtobufCMessage*)buffer;
}

/**
 * @brief free protobuf message pointer
 *
 * @param pMsg
 */
void release(ProtobufCMessage* pMsg);

inline bool isOneOfSkipped(const ProtobufCFieldDescriptor* pFieldDesc, void* pnative)
{
    if (pFieldDesc->flags & PROTOBUF_C_FIELD_FLAG_ONEOF) {
        unsigned int selected_id = proto_utils::getValue<unsigned int>(pnative, pFieldDesc->quantifier_offset);
        return selected_id != pFieldDesc->id;
    }
    return false;
}

/**
 * @brief Get the Proto Type Size
 *
 * @param type
 * @param isPtr
 * @return int
 */
int getProtoTypeSize(ProtobufCType type, bool* isPtr = nullptr);

}