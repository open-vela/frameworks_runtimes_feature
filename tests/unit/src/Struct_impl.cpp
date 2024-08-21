#include "Struct_impl.h"
#include "Struct.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "utils/feature_utils.h"
#include <protobuf-c/protobuf-c.h>
#include <string>

namespace Feature_Struct {

void onRegister(const char* feature_name)
{
}

void onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
}

void onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    auto* p = Struct::newInstance(handle);
    FeatureSetObjectData(handle, p);
}

void onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    Struct* pStruct = ft_utils::From<Struct>(handle);
    if (pStruct) {
        delete pStruct;
        FeatureSetObjectData(handle, nullptr);
    }
}

void onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
}

void onUnregister(const char* feature_name)
{
}

Struct::Struct(FeatureInstanceHandle hInstance, int a, int b)
    : StructBase(hInstance)
    , a_(a)
    , b_(b)
{
}

Struct::~Struct()
{
}

void Struct::foo(AppendData append_data, FtInt a, struct Chapter* b)
{
    FEATURE_LOG_INFO("a: %d, b: %p { page_count: %d, is_end: %d, title: %s }", a, b, b->page_count(), b->is_end(), b->title().ptr());
}

class Chapter* Struct::bar(AppendData append_data, FtInt a)
{
    ft_utils::RefPtr<Chapter> p = make<Chapter>();
    p->set_page_count(100);
    p->set_is_end(true);
    char* str = (char*)FeatureInstanceAllocType(getHandle(), strlen("hello world 123") + 1, FT_STRING);
    strcpy(str, "hello world 123");
    // move str into title
    auto title = ft_utils::FtStringPtr::adopt(str);
    p->set_title(title);
    return p.drop();
}

void Struct::bar2(AppendData append_data, struct Book* a)
{
    FEATURE_LOG_INFO("a: %p, title: %s chap_changed: %d", a, a->title().ptr(), a->chap_changed());
    ft_utils::FeatureArray<FtString> arr(a->chap_titles().ptr());
    FEATURE_LOG_INFO("chap_titles size: %d", arr.size());
    for (int i = 0; arr.size(); i++) {
        FtString item = arr[i];
        FEATURE_LOG_INFO("====%d: %s", i, item);
    }
    auto chap = a->first_chap();
    FEATURE_LOG_INFO("first_chap: %p { page_count: %d, is_end: %d, title: %s }", chap.ptr(), chap->page_count(), chap->is_end(), chap->title().ptr());
}

void Struct::print(AppendData append_data, FtVariParams vari_params)
{
}

void Struct::proto(AppendData append_data, FtInt a, Computer* b)
{
    FEATURE_LOG_INFO("a: %d, b: %p", a, b);
    std::string sn_code = std::string((const char*)b->sn_code.data, b->sn_code.len);
    FEATURE_LOG_INFO("{name: %s, price: %d, sn: %s, main_monitor w: %d, h:%d, color: %d}", b->name, b->price, sn_code.c_str(), b->main_monitor->width, b->main_monitor->height, b->main_monitor->colordepth);
}

void Struct::proto_cb(AppendData append_data, FtCallbackId cb)
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

    FeatureInvokeCallback(getHandle(), cb, &computer);
}

}