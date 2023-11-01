import { struct_test, Chapter, Book } from "./struct_test.d";

export function main() {
    // create feature module object
    let test = new struct_test();

    let chap: Chapter = { page_count: 365, title: "Pride and Prejudice", is_end: false};
    test.foo(1024, chap);

    const res_bar:Chapter = test.bar(42);
    console.log("test.bar return value Chapter ", res_bar.page_count);
    console.log("test.bar return value Chapter ", res_bar.title);
    console.log("test.bar return value Chapter ", res_bar.is_end);

    let first_chapter: Chapter = {
        page_count : 200,
        title: "chapter two",
        is_end:false,
    }

    const a:any = 1001;
    let book: Book = {
        any_param: a, 
        page_count: 1000, 
        title: "my book", 
        chap_titles: ["section1", "section2"], 
        first_chap: first_chapter, 
        chap_changed: (index: number, title: string) => {
            console.log("the book chapter changed, the change index is", index, "and the change title is", title);
        }
    };

    test.bar2(book);
}
