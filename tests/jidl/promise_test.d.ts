// Copyright 2023 Xiaomi, Inc. All rights reserved.


export class promise_test {
  private instance: number;
  constructor(){
    this.init_native(promise_test.clazz_name);
  }
  declare foo(a: number, b: string): any;
  use_foo (a: number): any {
    return this.foo (a, "hello");
  }
  declare foo1(a: number): any;
  declare foo2(): any;
  declare bar(): any;
  declare bar1(): any;
  declare bar2(): any;
  declare print(...rest: any[]): void;

// private:
  static readonly clazz_name = "promise_test";
  declare init_native(name: string): void;
}