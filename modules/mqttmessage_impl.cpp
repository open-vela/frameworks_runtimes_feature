
#include "mqtt_message.pb-c.h"
#include "mqttmessage.h"

#define MQTTMESSAGE_LOG_ERROR(fmt, ...) \
    FEATURE_LOG_ERROR("[mqtt message]" fmt, ##__VA_ARGS__)

#define MQTTMESSAGE_LOG_INFO(fmt, ...) \
    FEATURE_LOG_INFO("[mqtt message]" fmt, ##__VA_ARGS__)

#define MQTTMESSAGE_LOG_DEBUG(fmt, ...) \
    FEATURE_LOG_DEBUG("[mqtt message]" fmt, ##__VA_ARGS__)

void system_mqttmessage_onRegister(const char* feature_name)
{
    MQTTMESSAGE_LOG_DEBUG("onRegister: %s", feature_name);
}
void system_mqttmessage_onCreate(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    MQTTMESSAGE_LOG_DEBUG("onCreate");
}
void system_mqttmessage_onRequired(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    MQTTMESSAGE_LOG_DEBUG("onRequired");
}
void system_mqttmessage_onDetached(FeatureRuntimeContext ctx,
    FeatureInstanceHandle handle)
{
    MQTTMESSAGE_LOG_DEBUG("onDetached");
}
void system_mqttmessage_onDestroy(FeatureRuntimeContext ctx,
    FeatureProtoHandle handle)
{
    MQTTMESSAGE_LOG_DEBUG("onDestroy");
}
void system_mqttmessage_onUnregister(const char* feature_name)
{
    MQTTMESSAGE_LOG_DEBUG("onUnregister: %s", feature_name);
}

enum MessageType {
    kString,
    kInt64,
    kBytes,
};

struct FieldInfo {
    const char* name;
    MessageType type;
};
struct FieldInfo fields[] = { { "ts", kInt64 }, { "id", kString },
    { "carId", kString }, { "packageName", kString },
    { "action", kString }, { "data", kBytes } };

bool verifyType(ft_context_ref ctx, ft_value_t value, MessageType type)
{
    int ft_value_type = ft_get_type(ctx, value);
    switch (type) {
    case kInt64: {
        return ft_value_type == FT_TYPE_NUMBER;
        break;
    }
    case kString: {
        return ft_value_type == FT_TYPE_STRING;
        break;
    }
    case kBytes: {
        return ft_value_type == FT_TYPE_TYPED_BUFFER;
        break;
    }
    default: {
        MQTTMESSAGE_LOG_ERROR("unsupported type: %d", type);
    }
    }
    return false;
}

static bool verify(ft_context_ref ctx, ft_value_t obj)
{
    int type = ft_get_type(ctx, obj);
    if (type != FT_TYPE_OBJECT) {
        MQTTMESSAGE_LOG_ERROR("args is not object");
        return false;
    }

    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        ft_value_t value = ft_obj_get_property(ctx, obj, fields[i].name);
        if (!verifyType(ctx, value, fields[i].type)) {
            MQTTMESSAGE_LOG_ERROR("message verify failed, field: %s", fields[i].name);
            ft_free_value(ctx, value);
            return false;
        }
        ft_free_value(ctx, value);
    }
    return true;
}

FtBool system_mqttmessage_wrap_verify(FeatureInstanceHandle feature,
    AppendData append_data,
    FtAny obj)
{
    MQTTMESSAGE_LOG_INFO("message verify");
    if (!obj) {
        MQTTMESSAGE_LOG_ERROR("message verify failed, obj is not object");
        return false;
    }

    ft_context_ref ctx = FeatureGetContext(feature);
    return verify(ctx, *obj);
}
FtAny system_mqttmessage_wrap_decode(FeatureInstanceHandle feature,
    AppendData append_data,
    FtAny buffer)
{
    MQTTMESSAGE_LOG_INFO("message decode");
    ft_context_ref ctx = FeatureGetContext(feature);
    FtAny out = (FtAny)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *out = ft_undefined(ctx);
    if (!buffer) {
        MQTTMESSAGE_LOG_ERROR("message decode failed, arg is null");
        return out;
    }

    size_t sz;
    uint8_t* data = ft_to_buffer(ctx, &sz, *buffer);
    if (!data) {
        MQTTMESSAGE_LOG_ERROR("arg is not typed buffer");
        return out;
    }

    MqttMessage* mqtt_msg = mqtt_message__unpack(nullptr, sz, data);

    if (!mqtt_msg) {
        MQTTMESSAGE_LOG_ERROR("message unpack failed");
        return out;
    }

    ft_value_t obj = ft_new_object(ctx);
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        ft_value_t value = ft_undefined(ctx);
        switch (fields[i].type) {
        case kInt64: {
            value = ft_from_int64(ctx, mqtt_msg->ts);
            break;
        }
        case kString: {
            if (strcmp(fields[i].name, "id") == 0) {
                value = ft_from_string(ctx, mqtt_msg->id);
            } else if (strcmp(fields[i].name, "carId") == 0) {
                value = ft_from_string(ctx, mqtt_msg->carid);
            } else if (strcmp(fields[i].name, "packageName") == 0) {
                value = ft_from_string(ctx, mqtt_msg->packagename);
            } else if (strcmp(fields[i].name, "action") == 0) {
                value = ft_from_string(ctx, mqtt_msg->action);
            } else {
                assert(0);
            }
            break;
        }
        case kBytes: {
            value = ft_from_typed_buffer(ctx, mqtt_msg->data.data,
                mqtt_msg->data.len, FT_Uint8Array);
            break;
        }
        default: {
            MQTTMESSAGE_LOG_ERROR("unsupported type: %d", fields[i].type);
        }
        }
        ft_obj_set_property(ctx, obj, fields[i].name, value);
    }
    mqtt_message__free_unpacked(mqtt_msg, nullptr);

    *out = obj;
    return out;
}

FtAny system_mqttmessage_wrap_encode(FeatureInstanceHandle feature,
    AppendData append_data,
    FtAny obj)
{
    MQTTMESSAGE_LOG_INFO("message encode");
    ft_context_ref ctx = FeatureGetContext(feature);
    FtAny out = (FtAny)FeatureMalloc(sizeof(ft_value_t), FT_ANY_REF);
    *out = ft_undefined(ctx);

    if (!obj) {
        MQTTMESSAGE_LOG_ERROR("message encode failed, obj is null");
        return out;
    }

    if (!verify(ctx, *obj)) {
        MQTTMESSAGE_LOG_ERROR("message encode failed, verify failed");
        return out;
    }

    MqttMessage mqtt_msg = MQTT_MESSAGE__INIT;
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        ft_value_t value = ft_obj_get_property(ctx, *obj, fields[i].name);
        switch (fields[i].type) {
        case kInt64: {
            int64_t ts = 0;
            ft_to_int64(ctx, value, &ts);
            mqtt_msg.ts = ts;
            break;
        }
        case kString: {
            char* str = const_cast<char*>(ft_to_string(ctx, value));
            if (strcmp(fields[i].name, "id") == 0) {
                mqtt_msg.id = str;
            } else if (strcmp(fields[i].name, "carId") == 0) {
                mqtt_msg.carid = str;
            } else if (strcmp(fields[i].name, "packageName") == 0) {
                mqtt_msg.packagename = str;
            } else if (strcmp(fields[i].name, "action") == 0) {
                mqtt_msg.action = str;
            } else {
                assert(0);
            }
            break;
        }
        case kBytes: {
            size_t sz;
            uint8_t* data = ft_to_buffer(ctx, &sz, value);
            mqtt_msg.data.data = data;
            mqtt_msg.data.len = sz;
            break;
        }
        default: {
            MQTTMESSAGE_LOG_ERROR("unsupported type: %d", fields[i].type);
        }
        }
        ft_free_value(ctx, value);
    }

    int sz = mqtt_message__get_packed_size(&mqtt_msg);
    uint8_t* data = (uint8_t*)malloc(sz);
    mqtt_message__pack(&mqtt_msg, data);

    ft_value_t buffer = ft_from_typed_buffer(ctx, data, sz, FT_Uint8Array);
    free(data);
    *out = buffer;

    return out;
}