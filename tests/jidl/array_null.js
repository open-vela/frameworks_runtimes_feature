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

console.log('\n\n')

