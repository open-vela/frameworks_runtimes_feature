// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "event_test.h"

static const char* file_tag = "[jidl_feature] event_test_impl";

typedef struct _EventTestData {
    bool data_changed_added;
    bool state_changed_added;
    FeatureInterfaceHandle dog_interface;
} EventTestData;

static void on_event_status_change(FeatureInstanceHandle handle, FtEventId eid, FeatureEventStatus status)
{
    printf("%s::%s(), event_id: %" PRIi32 ", status: %d\n", file_tag, __FUNCTION__, eid, status);
    const char* event_name = FeatureGetEventName(handle, eid);
    if (!event_name) {
        printf("%s::%s(), error: event name is null\n", file_tag, __FUNCTION__);
        return;
    }
    printf("%s::%s(), event_name: %s\n", file_tag, __FUNCTION__, event_name);
    EventTestData* data = (EventTestData*)FeatureGetObjectData(handle);
    if (!data) {
        printf("%s::%s(), error, event test_data ptr is null\n", file_tag, __FUNCTION__);
        return;
    }
    int added = (status == FEATURE_EVENT_ADDED ? 1 : 0);
    if (strcmp(event_name, "data_changed") == 0) {
        data->data_changed_added = added;
    } else if (strcmp(event_name, "state_changed") == 0) {
        data->state_changed_added = added;
    }
}

// FeatureCallbacks to be implemented
void event_test_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void event_test_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void event_test_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureSetEventChangeListener(handle, on_event_status_change);
    EventTestData* data = (EventTestData*)malloc(sizeof(EventTestData));
    memset(data, 0, sizeof(EventTestData));
    FeatureSetObjectData(handle, data);
}

void event_test_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
    FeatureSetEventChangeListener(handle, NULL);
    EventTestData* data = (EventTestData*)FeatureGetObjectData(handle);
    if (data) {
        free(data);
    }
    FeatureSetObjectData(handle, NULL);
}

void event_test_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void event_test_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

// Function wrappers to be implemented
void event_test_wrap_invoke_event(FeatureInstanceHandle feature, AppendData append_data, FtString event_name)
{
    EventTestData* data = (EventTestData*)FeatureGetObjectData(feature);
    if (!data) {
        printf("%s::%s(), error, event test_data ptr is null\n", file_tag, __FUNCTION__);
        return;
    }
    if (strcmp(event_name, "data_changed") == 0 && data->data_changed_added) {
        FtEventId id = FeatureGetEventId(feature, event_name);
        int cb_count = FeatureGetEventCallbackCount(feature, id);
        printf("%s::%s(), event_name: %s, id: %" PRIi32 ", cb_count: %d\n", file_tag, __FUNCTION__, event_name, id, cb_count);
        FeatureEmitEvent(feature, id, "hello world");
    } else if (strcmp(event_name, "state_changed") == 0 && data->state_changed_added) {
        int cb_count = FeatureGetEventCallbackCountByName(feature, event_name);
        printf("%s::%s(), event_name: %s, cb_count: %d\n", file_tag, __FUNCTION__, event_name, cb_count);
        FeatureEmitEventByName(feature, event_name, 50);
    } else if (strcmp(event_name, "weight_changed") == 0) {
        if (data->dog_interface) {
            printf("%s::%s(), emit interface event, event_name: %s\n", file_tag, __FUNCTION__, event_name);
            FeatureEmitEventByName(data->dog_interface, event_name, 80);
        }
    }
}

FeatureInterfaceHandle event_test_wrap_createDog(FeatureInstanceHandle feature, AppendData append_data, FtInt type)
{
    FeatureInterfaceHandle handle = event_test_createDog_instance(feature);
    EventTestData* data = (EventTestData*)FeatureGetObjectData(feature);
    if (!data) {
        printf("%s::%s(), error, event test_data ptr is null\n", file_tag, __FUNCTION__);
        return handle;
    }
    data->dog_interface = handle;
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);
    return handle;
}

// vtable functions for interface constructor function 'createDog'
void event_test_Animal_interface_dog_finalize(FeatureInterfaceHandle handle)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
}

FtString event_test_Animal_interface_dog_get_name(FeatureInterfaceHandle handle, AppendData append_data)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "dog name is: %s", "tommy");
    return buf;
}

void event_test_Animal_interface_dog_set_name(FeatureInterfaceHandle handle, AppendData append_data, FtString name)
{
    printf("%s::%s(), interface: %p, set dog name: %s\n", file_tag, __FUNCTION__, handle, name);
}

FeatureInterfaceHandle event_test_wrap_createPigeon(FeatureInstanceHandle feature, AppendData append_data)
{
    FeatureInterfaceHandle handle = event_test_createPigeon_instance(feature);
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);
    return handle;
}

// vtable functions for interface constructor function 'createPigeon'
void event_test_Bird_interface_pigeon_finalize(FeatureInterfaceHandle handle)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
}

void event_test_Bird_interface_pigeon_fly(FeatureInterfaceHandle handle, AppendData append_data)
{
    printf("%s::%s(), pigeon is flying: %p\n", file_tag, __FUNCTION__, handle);
}

FeatureInterfaceHandle event_test_wrap_createCock(FeatureInstanceHandle feature, AppendData append_data)
{
    FeatureInterfaceHandle handle = event_test_createCock_instance(feature);
    printf("%s::%s(), feature: %p, interface: %p\n", file_tag, __FUNCTION__, feature, handle);
    return handle;
}

// vtable functions for interface constructor function 'createCock'
void event_test_Chicken_interface_cock_finalize(FeatureInterfaceHandle handle)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
}

FtString event_test_Chicken_interface_cock_get_name(FeatureInterfaceHandle handle, AppendData append_data)
{
    printf("%s::%s(), interface: %p\n", file_tag, __FUNCTION__, handle);
    char* buf = (char*)FeatureMalloc(128, FT_STRING);
    sprintf(buf, "cock name is: %s", "wowo");
    return buf;
}

void event_test_Chicken_interface_cock_set_name(FeatureInterfaceHandle handle, AppendData append_data, FtString name)
{
    printf("%s::%s(), interface: %p, set cock name: %s\n", file_tag, __FUNCTION__, handle, name);
}

void event_test_Chicken_interface_cock_fly(FeatureInterfaceHandle handle, AppendData append_data)
{
    printf("%s::%s(), cock cannot fly: %p\n", file_tag, __FUNCTION__, handle);
}

FtInt event_test_Chicken_interface_cock_get_weight(FeatureInterfaceHandle handle, AppendData append_data)
{
    printf("%s::%s(), cock get weight: %p\n", file_tag, __FUNCTION__, handle);
    return 30;
}

void event_test_Chicken_interface_cock_invoke(FeatureInterfaceHandle handle, AppendData append_data, FtString event_name)
{
    FtEventId id = FeatureGetEventId(handle, event_name);
    int cb_count = FeatureGetEventCallbackCountByName(handle, event_name);
    printf("%s::%s(), event_name: %s, id: %" PRIi32 ", cb_count: %d\n", file_tag, __FUNCTION__, event_name, id, cb_count);
    if (id <= 0) {
        printf("%s::%s(), invalid event id (%" PRIi32 ") for event: %s\n", file_tag, __FUNCTION__, id, event_name);
        return;
    }
    if (cb_count <= 0) {
        return;
    }
    if (strcmp(event_name, "weight_changed") == 0) {
        FeatureEmitEvent(handle, id, 60);
    } else if (strcmp(event_name, "ascended") == 0) {
        FeatureEmitEventByName(handle, event_name, 500);
    } else if (strcmp(event_name, "egg_layed") == 0) {
        FeatureEmitEventByName(handle, event_name, 2);
    }
}
