let test = require('struct_test');

test.foo(1024, {
        'page_count' : 365,
        'title': 'Pride and Prejudice',
        'is_end': false
    })

let res_bar = test.bar(42)
test.print('test.bar ret, page_count: ', res_bar.page_count)
test.print('test.bar ret, title: ', res_bar.title)
test.print('test.bar ret, is_end: ', res_bar.is_end)

test.bar2({
        any_param: {
            any1: 100,
            any2: 'chapter one',
            any3: [500, 'chapter 2', '0.5px'],
        },

        'page_count' : 1000,
        'title': 'my book',
        chap_titles : ['section1', 'section2', 'section3'],
        'first_chap': {
            'page_count' : 100,
            'title': 'chapter one',
            'is_end': false
        },
        'chap_changed': function (index, title) {
            test.print('chapter changed, index: ', index, ', title: ', title)
        }
    })
test.print('\n\n')

let chapter2 = {
    'page_count' : 200,
    'title': 'chapter two',
    'is_end': true
}
test.foo(5, chapter2);

let book = {
    any_param: {
        any1: 340000,
        any2: 'chapter one by one',
        any3: [500, 'chapter 220', '0.5px'],
    },
    'page_count' : 600,
    'title': 'your book',
    'chap_titles': ['section 4', 'section 5', 'section 6'],
    'first_chap': chapter2,
    'chap_changed': function (index, title) {
        test.print('chap_changed, index: ', index, ', title: ', title)
    }
}
test.bar2(book)

test.print('\n')
test.print('null param test begin ====================');
test.foo(2048, {
        page_count : 100,
        title: null,
        is_end: false
    })

let book2 = {
    any_param: null,
    page_count : 600,
    title: null,
    chap_titles:  ['section 7', 'section 8', 'section 9'],
    first_chap: null,
    chap_changed: function (index, title) {
        test.print('chap_changed, index: ', index, ', title: ', title)
    }
}
test.bar2(book2)
test.print('null param test end =====================\n');

