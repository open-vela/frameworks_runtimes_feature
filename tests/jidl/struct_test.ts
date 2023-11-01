import { struct_test, Chapter, Book } from "./struct_test.d";

export function main() {
    // create feature module object
    let test = new struct_test();

    let chap: Chapter = { page_count: 365, title: "Pride and Prejudice", is_end: false};
    test.foo(1024, chap);

    const res_bar:Chapter = test.bar(42);
    test.print('test.bar ret, page_count: ', res_bar.page_count);
    test.print('test.bar ret, title: ', res_bar.title);
    test.print('test.bar ret, is_end: ', res_bar.is_end);

    let chapter1: Chapter = {
        page_count : 100,
        title: "chapter one",
        is_end:false,
    }

    const a:any = 1001;
    let book1: Book = {
        any_param: a,
        page_count: 1000,
        title: "my book",
        chap_titles: ["section1", "section2", "section3"],
        first_chap: chapter1,
        chap_changed: (index: number, title: string) => {
            console.log("chapter changed, index: ", index, "title: ", title);
        }
    };
    test.bar2(book1);
    test.print('\n\n')

    let chapter2: Chapter = {
        page_count: 200,
        title: 'chapter two',
        is_end: true
    }
    test.foo(5, chapter2);
 
    const b:any = 2001;
    let book2: Book = {
        any_param: b,
        page_count : 600,
        title: 'your book',
        chap_titles: ['section 4', 'section 5', 'section 6'],
        first_chap: chapter2,
        chap_changed: function (index, title) {
            test.print('chap_changed, index: ', index, ', title: ', title)
        }
    }
    test.bar2(book2);
    test.print('\n\n');
}
