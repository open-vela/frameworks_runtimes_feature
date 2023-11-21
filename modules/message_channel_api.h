#ifndef MESSAGE_CHANNEL_API_H
#define MESSAGE_CHANNEL_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* MessageChannelHandle;
typedef int32_t ReplyId;

typedef void (*RequestCb)(const char* data);
typedef void (*ServiceMsgCb)(MessageChannelHandle handle, ReplyId id, const char* data);
typedef void (*SubscribeCb)(const char* name, const char* data);

MessageChannelHandle message_channel_init();
void message_channel_uninit(MessageChannelHandle handle);

void message_channel_send_async_request(MessageChannelHandle handle, const char* name, const char* data, RequestCb cb);
void message_channel_send_async_response(MessageChannelHandle handle, ReplyId id, const char* data);
// using the interface is the binder server
void message_channel_add_async_service(MessageChannelHandle handle, const char* name, ServiceMsgCb cb);

void message_channel_publish(MessageChannelHandle handle, const char* topic_name, const char* data);
void message_channel_subscribe(MessageChannelHandle handle, const char* topic_name, SubscribeCb cb);
void message_channel_unsubscribe(MessageChannelHandle handle, const char* topic_name);

#ifdef __cplusplus
}
#endif

#endif