#include "Struct.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "feature_utils.h"

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