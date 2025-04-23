#include "Protobuf_impl.h"
#include "Protobuf.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "feature_utils.h"
#include "test.pb-c.h"
#include <cstdint>
#include <protobuf-c/protobuf-c.h>
#include <string>

namespace Feature_Protobuf {

void onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("onRegister: %s", feature_name);
}

void onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("onCreatre: FeatureProtoHandle %p", handle);
}

void onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("onDestroy: FeatureProtoHandle %p", handle);
}

void onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("onUnregister: %s", feature_name);
}

Protobuf::Protobuf(FeatureInstanceHandle hInstance)
    : ProtobufBase(hInstance)
{
}

void Protobuf::proto(FtInt a, const Pb_computer_p& b)
{
    FEATURE_LOG_INFO("a: %d, b: %p", a, b);
    std::string data = std::string((const char*)b->sn_code.data, b->sn_code.len);
    FEATURE_LOG_INFO("{name: %s, price: %d, sn: %s, main_monitor w: %d, h:%d, color: %d}", b->name, b->price, data.c_str(), b->main_monitor->width, b->main_monitor->height, b->main_monitor->colordepth);
}

void Protobuf::invoke_proto(FtCallbackId cb)
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

    computer.n_monitors = 2;
    computer.monitors = static_cast<Monitor**>(malloc(sizeof(Monitor*) * computer.n_monitors));
    for (size_t i = 0; i < computer.n_monitors; i++) {
        computer.monitors[i] = &monitor;
    }

    FeatureInvokeCallback(getHandle(), cb, &computer);
    free(computer.monitors);
}

}
