
#ifndef MIOT_SERVICE_AGENT_EVENT_H
#define MIOT_SERVICE_AGENT_EVENT_H

struct _EventAction EventAction;

typedef void (*POnEventCallback)(int type, const char* reply_key, const char* json_data, void* user_data);

struct _EventAction {
  void (*send)(EventAction* pself, const Miotmsg* msg, POnEventCallback cb, void* user_data);
  void (*listen)(EventAction* pself, const char* key, const Miotmsg* msg, POnEventCallback cb, void* user_data);
};

typedef struct _EventActionEntryInfo {
  const char* method_name;
  int (*buildMessage)(Miotmsg* msg, size_t msg_size, int argc, const char* argv[]);
} EventActionEntryInfo;

typedef struct _EventNotifyEntryInfo {
  const char* event_name;
  const char* (*buildMessage)(Miotmsg* msg, size_t msg_size, int argc, const char* argv[]);
} EventNotifyEntryInfo;

typedef struct _EventActionInfo {
  const char* name;
  EventActionEntryInfo *action_entries;
  EventActionNotifyInfo* notify_entries;
  size_t action_count;
  size_t notify_count;
} EventActionInfo;

const char* find_key_arg(const char* key_name, int argc, const char* argv[]);

#endif  // MIOT_SERVICE_AGENT_EVENT_H
