let test = require('null_test');

test.print('\n')
test.setChapter(0, {
	page_count : 35,
	title: 'monkey born from a stone'
})
test.setChapter(1)
test.print('\n')

test.setChapChangedCb(function (index, title) {
  test.print('chapter changed, index: ', index, ', title: ', title)
})
test.setChapChangedCb()
test.print('\n')

let chapter2 = {
    page_count : 200,
    title: 'chapter two'
}

let book_1 = {
    book_info: {
        author: 'ChengenWu',
        hot: 10000,
    },
    page_count : 600,
    title: 'my book',
    first_chap: chapter2,
    chap_changed: function (index, title) {
        test.print('chap_changed, index: ', index, ', title: ', title)
    }
}
test.setBook(book_1)
test.print('\n')

let book_2 = {
    page_count : 500,
}
test.setBook(book_2)
test.print('\n')

