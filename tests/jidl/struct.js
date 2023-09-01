let struct = require('Struct_1_0');

struct.foo(1,
    {
        'page_count' : 100,
         'title': 'chapter one'
    })

struct.bar2(
   {
       'page_count' : 500,
        'title': 'my book',
        'chap_titles': ['chapter 1', 'chapter 2', 'chapter 3'],
        'first_chap': {
            'page_count' : 100,
            'title': 'chapter one'
        },
        'chap_changed': function (index, title) {
            struct.print('chap_changed, index: ', index, ', title: ', title)
        }
    })
struct.print('\n\n')

let first_chapter = {
    'page_count' : 200,
    'title': 'chapter two'
}
struct.foo(5, first_chapter);

let book = {
    'page_count' : 600,
    'title': 'your book',
    'chap_titles': ['section 1', 'section 2', 'section 3'],
    'first_chap': first_chapter,
    'chap_changed': function (index, title) {
        struct.print('chap_changed, index: ', index, ', title: ', title)
    }
}
struct.bar2(book)
struct.print('\n\n')

let chapter = struct.bar(2)
struct.print('chap_page_count: ', chapter.page_count, ', chap_title: ', chapter.title)