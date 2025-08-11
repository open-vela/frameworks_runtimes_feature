let test = require('array_null');

console.log('test.set_pages: pass null')
test.set_pages()
console.log('test.set_pages: pass int array')
test.set_pages([10, 20, 50, 100, 150, 200, 250])

console.log('test.set_chapters: pass null')
test.set_chapters()
console.log('test.set_chapters: pass string array')
test.set_chapters(['I', 'am', 'your', 'friend'])

console.log('test.set_book: pass null')
test.set_book()
let book_1 = {
    'page_count' : 50,
    'title': 'chapter one',
    'chapter_pages': [1, 2, 3, 4, 5],
    'chapters': ['chap 1', 'chap 2', 'chap 3', 'chap 4', 'chap 5']
}
console.log('test.set_book: pass book_1')
test.set_book(book_1)

let book_2 = {
    'page_count' : 100,
    'title': 'chapter two',
}
console.log('test.set_book: pass book_2')
test.set_book(book_2)

console.log('test.set_books: pass null')
test.set_books()
let book_array = [
    book_1,
    book_2
]
console.log('test.set_books: pass book_array')
test.set_books(book_array)

console.log('test.get_books')
let books = test.get_books()
console.log('got books:', JSON.stringify(books, null, 2))

console.log('test.set_nested')
let nested = {
    a: 1,
    sub: { a: 21, sub: null, subs: [ { a: 211, sub: null, subs: null }, { a: 212, sub: null, subs: null } ]},
    subs: [
        {
            a: 11,
            sub: { a: 121, sub: null, subs: null },
            subs: [
                { a: 111, sub: null, subs: [ { a: 1111, sub: null, subs: null }, { a: 1112, sub: null, subs: null } ] },
                { a: 112, sub: null, subs: [ { a: 1121, sub: null, subs: null }, { a: 1122, sub: null, subs: null } ] }
            ]
        },
        {
            a: 12,
            sub: null,
            subs: [
                { a: 121, sub: null, subs: [ { a: 1211, sub: null, subs: null }, { a: 1212, sub: null, subs: null } ] },
                {
                    a: 122,
                    sub: null,
                    subs: [
                        { a: 1221, sub: null, subs: [ { a: 12211, sub: null, subs: null }, { a: 12212, sub: null, subs: null } ] },
                        { a: 1222, sub: null, subs: null }
                    ]
                }
            ]
        }
    ]
}
test.set_nested(nested)
let nested_ret = test.get_nested()
console.log('nested_ret: ', JSON.stringify(nested_ret));

console.log('test.set_nested2')
let nested2 = {
    a: 1,
    sub: {
        a: 11,
        sub: {
            a: 111,
            sub: {
                a: 1111,
                sub: null
            }
        }
    }
}
test.set_nested2(nested2)
let nested2_ret = test.get_nested2()
console.log('nested2_ret: ', JSON.stringify(nested2_ret));

console.log('\n\n')

