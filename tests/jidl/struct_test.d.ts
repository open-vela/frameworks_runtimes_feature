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
  chap_changed: (index: number, title: string) => void;
}


export class struct_test {
  private instance: number;
  constructor(){
    this.init_native(struct_test.clazz_name);
  }
  declare foo(a: number, b: Chapter): void;
  declare bar(a: number): Chapter;
  declare bar2(a: Book): void;
  declare print(...rest: any[]): void;

// private:
  static readonly clazz_name = "struct_test";
  declare init_native(name: string): void;
}