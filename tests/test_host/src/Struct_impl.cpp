#include "Struct_impl.h"
#include "Struct.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "feature_types.h"
#include "utils/feature_utils.h"
#include <cstring>
#include <protobuf-c/protobuf-c.h>

namespace Feature_Struct {

void onRegister(const char* feature_name)
{
}

void onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
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

void Struct::foo(FtInt a, const ft_utils::RefPtr<Chapter>& b)
{
    FEATURE_LOG_INFO("a: %d, b: %p { page_count: %d, is_end: %d, title: %s }", a, b.ptr(), b->page_count(), b->is_end(), b->title().ptr());
}

ft_utils::RefPtr<Chapter> Struct::bar(FtInt a)
{
    ft_utils::RefPtr<Chapter> p = make<Chapter>();
    p->set_page_count(100);
    p->set_is_end(true);
    p->set_title(strdup("hello world 123"));
    return p;
}

void Struct::bar2(const ft_utils::RefPtr<Book>& a)
{
    printf("%s(), page_count: %d, title: %s\n",
        __FUNCTION__, a->page_count(), a->title().ptr());

    if (!a->chap_titles()) {
        FEATURE_LOG_INFO("chap_titles ptr is null!");
    } else {
        auto chapTitlesArray = a->chap_titles().getArray<const char*>();
        if (!chapTitlesArray) {
            FEATURE_LOG_ERROR("get chap_titles array failed !");
        }
        char buf[1024];
        char* pos = buf;
        pos += sprintf(pos, "chap_titles: [");
        for (int32_t i = 0; i < chapTitlesArray.size(); i++) {
            if (i > 0)
                pos += sprintf(pos, ", ");
            pos += sprintf(pos, "%s", chapTitlesArray[i]);
        }
        pos += sprintf(pos, "]");
        FEATURE_LOG_INFO("~~~~%s", buf);
    }

    if (!a->first_chap()) {
        FEATURE_LOG_INFO("first_chap ptr is null!");
    } else {
        FEATURE_LOG_INFO("first_chapter: [page_count: %d, title: %s]", a->first_chap()->page_count(), a->first_chap()->title().ptr());
    }

    if (!FeatureInvokeCallback(getHandle(), a->chap_changed(), 0, a->title().ptr())) {
        FEATURE_LOG_ERROR("invoke failed !");
        return;
    }
    FeatureRemoveCallback(getHandle(), a->chap_changed());
}

ft_utils::RefPtr<FtArray> Struct::getBooks(FtInt count)
{
    auto result = makeArray<Book*>(count);
    char buf[128];
    for (int i = 0; i < count; i++) {
        auto elem = make<Book>();
        elem->set_page_count(i + 5);
        elem->set_title(strdup("hello world"));
        auto titles = makeArray<FtString>(3);
        for (int j = 0; j < 3; j++) {
            sprintf(buf, "chapter: %d-%d", i, j);
            titles.append(strdup((const char*)buf));
            elem->set_chap_titles(titles.getShared());
        }
        auto chapter = make<Chapter>();
        chapter->set_page_count(i + 3);
        sprintf(buf, "title: %d", i);
        chapter->set_title(strdup((const char*)buf));
        chapter->set_is_end(false);
        elem->set_first_chap(chapter);
        elem->set_chap_changed(true);
        result.append(elem);
    }
    return result;
}

}