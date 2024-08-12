#include "Struct.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "feature_utils.h"
#include "test.pb-c.h"
#include <cstdint>
#include <protobuf-c/protobuf-c.h>

// FeatureCallbacks to be implemented
void Struct_onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("onReister: %s", feature_name);
}

void Struct_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("onCreate: proto handle=%p", handle);
}

void Struct_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("onRequired: instance handle=%p", handle);
}

void Struct_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    FEATURE_LOG_INFO("onDetached: instance handle=%p", handle);
}

void Struct_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("onDestroy: proto proto handle=%p", handle);
}

void Struct_onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("onUnregister: %s", feature_name);
}

// Function wrappers to be implemented
void Struct_wrap_foo(FeatureInstanceHandle feature, AppendData append_data, FtInt a, Struct_Chapter* b)
{
    FEATURE_LOG_INFO("a: %d, b: %p", a, b);
    FEATURE_LOG_INFO("{title: %p, page_count: %d, is_end: %d}", b->title, b->page_count, b->is_end);
}

Struct_Chapter* Struct_wrap_bar(FeatureInstanceHandle feature, AppendData append_data, FtInt a)
{
    Struct_Chapter* p = StructMallocChapter();
    char* title = (char*)FeatureMalloc(strlen("Chapter Title") + 1, FT_STRING);
    strcpy(title, "Chapter Title");
    p->title = title;
    p->is_end = true;
    p->page_count = a;
    return p;
}

void Struct_wrap_bar2(FeatureInstanceHandle feature, AppendData append_data, Struct_Book* a)
{
}

void Struct_wrap_print(FeatureInstanceHandle feature, AppendData append_data, FtVariParams vari_params)
{
}

void Struct_wrap_proto(FeatureInstanceHandle feature, AppendData append_data, FtInt a, Computer* b)
{
    FEATURE_LOG_INFO("a: %d, b: %p", a, b);
    FEATURE_LOG_INFO("{name: %s, price: %d, sn: %s, main_monitor w: %d, h:%d, color: %d}", b->name, b->price, b->sn_code.data, b->main_monitor->width, b->main_monitor->height, b->main_monitor->colordepth);
}

void Struct_proto_cb(void* feature, AppendData append_data, FtCallbackId cb)
{
    FEATURE_LOG_INFO("cid %d", cb);

    Computer computer;
    computer__init(&computer);

    computer.name = (char*)"computerrrrrr";
    computer.price = 456;
    computer.sn_code.data = (uint8_t*)"123456";
    computer.sn_code.len = 6;

    Monitor monitor;
    monitor__init(&monitor);

    monitor.width = 123;
    monitor.height = 456;
    monitor.colordepth = 4;
    computer.main_monitor = &monitor;

    FeatureInvokeCallback(feature, cb, &computer);
}