#pragma once
#include "feature_types.h"
#include "quickjs/quickjs.h"
#include <protobuf-c/protobuf-c.h>
#include <vector>

namespace proto_reflection {

/**
 * @brief convert protobuf message to JSValue
 *
 * @param ctx
 * @param value
 * @param pnative
 * @return true
 * @return false
 */
bool fromNative(JSContext* ctx, JSValue& value, void* pnative);

/**
 * @brief create protobuf message from JSValue
 *
 * @param ctx
 * @param value
 * @param descriptor
 * @param message
 * @return true
 * @return false
 */
bool toNative(FeatureInstanceHandle handle, JSContext* ctx, JSValue value, const ProtobufCMessageDescriptor* descriptor, ProtobufCMessage** message);
}
