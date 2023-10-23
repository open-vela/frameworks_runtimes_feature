// Copyright 2023 Xiaomi, Inc. All rights reserved.

export class Chapter {
    page_count: number;
    title: string;
    is_end: boolean;
}

export class Book {
    any_param: any;
    page_count: number;
    title: string;
    chap_titles: string[];
    first_chap: Chapter;
    ChapChanged: (index: number, title: string) => void;
}

export class Struct {
    constructor() {
        this.init_native(this.clazz_name);
    }

    declare foo(a: number, b: Chapter): void;

    declare bar(a: number): Chapter;

    declare print(...rest: any[]): void;

    declare bar2(b: Book): void;

    readonly clazz_name = "Struct";
    declare init_native(name: string): void;
}