// Copyright 2023 Xiaomi, Inc. All rights reserved.

#include "array_null.h"

static const char* file_tag = "[jidl_feature] array_null_impl";

// FeatureCallbacks to be implemented
void array_null_onRegister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void array_null_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void array_null_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void array_null_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void array_null_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

void array_null_onUnregister(const char* feature_name)
{
    printf("%s::%s()\n", file_tag, __FUNCTION__);
}

template <typename T>
class FTArrayHelper {
private:
    FtArray* _data;

public:
    FTArrayHelper(FtArray* data)
    {
        _data = data;
    }

    ~FTArrayHelper()
    {
    }

    T& operator[](int32_t index)
    {
        return ((T*)_data->_element)[index];
    }

    int32_t size() const { return _data->_size; }
};

// Function wrappers to be implemented
void array_null_wrap_set_pages(FeatureInstanceHandle feature, AppendData append_data, FtArray* pages)
{
    if (!pages) {
        printf("%s::%s(), pages ptr is null\n", file_tag, __FUNCTION__);
        return;
    }
    FTArrayHelper<int> page_array(pages);
    printf("%s::%s(), pages ptr: %p, page_array size: %" PRIi32 "\n", file_tag, __FUNCTION__, pages, page_array.size());
    if (page_array.size() <= 0) {
        return;
    }
    printf("page_array = [\n");
    for (int32_t i = 0; i < page_array.size(); i++) {
        printf("  index %" PRIi32 ": %d\n", i, page_array[i]);
    }
    printf("]\n");
}

void array_null_wrap_set_chapters(FeatureInstanceHandle feature, AppendData append_data, FtArray* chapters)
{
    if (!chapters) {
        printf("%s::%s(), chapters ptr is null\n", file_tag, __FUNCTION__);
        return;
    }
    FTArrayHelper<char*> chapter_array(chapters);
    printf("%s::%s(), chapters ptr: %p, chapter_array size: %" PRIi32 "\n", file_tag, __FUNCTION__, chapters, chapter_array.size());
    if (chapter_array.size() <= 0) {
        return;
    }
    printf("chapter_array = [\n");
    for (int32_t i = 0; i < chapter_array.size(); i++) {
        printf("  index %" PRIi32 ": %s\n", i, chapter_array[i]);
    }
    printf("]\n");
}

static void print_book(array_null_Book* book)
{
    printf("%s::%s(), page_count: %d, title: %s, chapter_pages: %p, chapters: %p\n",
        file_tag, __FUNCTION__, book->page_count, book->title, book->chapter_pages, book->chapters);
    if (book->chapter_pages) {
        FTArrayHelper<int> chap_page_array(book->chapter_pages);
        printf("%s::%s(), chap_page_array size: %" PRIi32 "\n", file_tag, __FUNCTION__, chap_page_array.size());
        if (chap_page_array.size() > 0) {
            printf("chap_page_array = [\n");
            for (int32_t i = 0; i < chap_page_array.size(); i++) {
                printf("  index %" PRIi32 ": %d\n", i, chap_page_array[i]);
            }
            printf("]\n");
        }
    }

    if (book->chapters) {
        FTArrayHelper<char*> chap_array(book->chapters);
        printf("%s::%s(), chap_array size: %" PRIi32 "\n", file_tag, __FUNCTION__, chap_array.size());
        if (chap_array.size() > 0) {
            printf("chap_array = [\n");
            for (int32_t i = 0; i < chap_array.size(); i++) {
                printf("  index %" PRIi32 ": %s\n", i, chap_array[i]);
            }
            printf("]\n");
        }
    }
}

void array_null_wrap_set_book(FeatureInstanceHandle feature, AppendData append_data, array_null_Book* book)
{
    if (!book) {
        printf("%s::%s(), book ptr is null!\n", file_tag, __FUNCTION__);
        return;
    }
    print_book(book);
}

void array_null_wrap_set_books(FeatureInstanceHandle feature, AppendData append_data, FtArray* books)
{
    if (!books) {
        printf("%s::%s(), books ptr is null\n", file_tag, __FUNCTION__);
        return;
    }
    FTArrayHelper<array_null_Book*> book_array(books);
    printf("%s::%s(), books ptr: %p, book_array size: %" PRIi32 "\n", file_tag, __FUNCTION__, books, book_array.size());
    if (book_array.size() <= 0) {
        return;
    }

    printf("book_array = [\n");
    for (int32_t i = 0; i < book_array.size(); i++) {
        array_null_Book* book = book_array[i];
        print_book(book);
    }
    printf("]\n");
}

static array_null_Book* make_book(int idx)
{
    array_null_Book* book = array_nullMallocBook();
    book->page_count = (idx + 2) * 100;
    char* title = (char*)FeatureMalloc(64, FT_STRING);
    sprintf(title, "book_%d", idx);
    book->title = title;

    FtArray* int_array = array_null_malloc_int_array();
    int_array->_size = 4;
    int_array->_element = malloc(sizeof(int) * int_array->_size);
    FTArrayHelper<int> chap_page_array(int_array);
    for (int32_t i = 0; i < chap_page_array.size(); i++) {
        chap_page_array[i] = (idx + i + 1) * 5;
    }
    book->chapter_pages = int_array;

    FtArray* str_array = array_null_malloc_string_array();
    str_array->_size = 4;
    str_array->_element = malloc(sizeof(char*) * str_array->_size);
    FTArrayHelper<char*> chap_array(str_array);
    for (int32_t i = 0; i < chap_array.size(); i++) {
        char* chapter = (char*)FeatureMalloc(64, FT_STRING);
        sprintf(chapter, "book_%d_chap_%" PRIi32, idx, i);
        chap_array[i] = chapter;
    }
    book->chapters = str_array;
    return book;
}

FtArray* array_null_wrap_get_books(FeatureInstanceHandle feature, AppendData append_data)
{
    FtArray* array = array_null_malloc_Book_struct_type_array();
    array->_size = 3;
    array->_element = malloc(sizeof(array_null_Book*) * array->_size);
    FTArrayHelper<array_null_Book*> book_array(array);
    for (int32_t i = 0; i < book_array.size(); i++) {
        book_array[i] = make_book(i);
    }
    return array;
}

static void print_nested(array_null_Nested* nested)
{
    printf("{\na: %d,\n", nested->a);
    if (!nested->sub) {
        printf("sub: null,\n");
    } else {
        printf("sub: ");
        print_nested(nested->sub);
        printf(",\n");
    }
    if (!nested->subs) {
        printf("subs: null \n}\n");
        return;
    }
    FTArrayHelper<array_null_Nested*> subs_array(nested->subs);
    if (subs_array.size() <= 0) {
        printf("subs: null \n}\n");
        return;
    }
    printf("subs: [\n");
    for (int32_t i = 0; i < subs_array.size(); i++) {
        array_null_Nested* sub = subs_array[i];
        if (sub)
            print_nested(sub);
    }
    printf("]\n}\n");
}

void array_null_wrap_set_nested(FeatureInstanceHandle feature, AppendData append_data, array_null_Nested* nested)
{
    if (!nested) {
        FEATURE_LOG_ERROR("%s, nested ptr is null", file_tag);
        return;
    }
    print_nested(nested);
}

static FtArray* make_nested_array(size_t size);

static array_null_Nested* make_nested(int depth)
{
    if (depth == 0)
        return nullptr;

    array_null_Nested* nested = array_nullMallocNested();
    nested->a = depth;
    nested->sub = make_nested(depth - 1);
    nested->subs = make_nested_array(depth);
    return nested;
}

static FtArray* make_nested_array(size_t size)
{
    if (size == 0)
        return nullptr;

    FtArray* array = array_null_malloc_string_array();
    array->_size = size;
    array->_element = malloc(sizeof(array_null_Nested*) * array->_size);
    FTArrayHelper<array_null_Nested*> nested_array(array);
    for (int32_t i = 0; i < nested_array.size(); i++) {
        nested_array[i] = make_nested(i);
    }
    return array;
}

array_null_Nested* array_null_wrap_get_nested(FeatureInstanceHandle feature, AppendData append_data)
{
    return make_nested(3);
}

static void print_nested2(array_null_Nested2* nested)
{
    printf("{\n a: %d,\n", nested->a);
    if (!nested->sub) {
        printf("sub: null \n}\n");
        return;
    }
    printf("sub: ");
    print_nested2(nested->sub);
    printf("}\n");
}

void array_null_wrap_set_nested2(FeatureInstanceHandle feature, AppendData append_data, array_null_Nested2* nested)
{
    if (!nested) {
        FEATURE_LOG_ERROR("%s, nested ptr is null", file_tag);
        return;
    }
    print_nested2(nested);
}

static array_null_Nested2* make_nested2(int depth)
{
    if (depth == 0)
        return nullptr;

    array_null_Nested2* nested = array_nullMallocNested2();
    nested->a = depth;
    nested->sub = make_nested2(depth - 1);
    return nested;
}

array_null_Nested2* array_null_wrap_get_nested2(FeatureInstanceHandle feature, AppendData append_data)
{
    return make_nested2(3);
}
