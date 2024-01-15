// Copyright 2023 Xiaomi, Inc. All rights reserved.


export class Simple {
  private instance: number;
  constructor(){
    this.init_native(Simple.clazz_name);
  }
  declare printStr(c: string): void;
  declare print(...rest: any[]): void;
  declare foo(a: number, c: string, b: number): number;
  declare bar(): void;
  declare bar5(a: number, ...rest: any[]): void;
  declare bar6(a: number, b: number, c: boolean): string;
  ubar6 (a: number): string {
    return this.bar6 (1, a, false);
  }
  declare goo(a: number, b: number, cb: (x: number, y: string, z: number) => void): void;
  declare goo2(cb: (a: number, b: string, ...rest: any[]) => void, cb3: () => void, cb4: (...rest: any[]) => void): void;
  goo3 (cb: (x: number, y: string, z: number) => void): void {
    this.goo (100, 200, cb);
  }
  declare foo2(x: number, y: number, cb: (x: number, y: string, z: number) => void, cb2: (a: number, b: string, ...rest: any[]) => void): void;
  declare foo3(x: number, y: number, cb: (x: number, y: string, z: number) => void): void;
  declare justTestNeverCall1(): void;
  declare justTestNeverCall2(): void;
  declare bar2(values: number[]): number;
  declare bar3(): string[];
  static const x: number = 1;
  static const y: string = "hello world";
  static const z: number = 9.8;
  get name(): string {
    return this.get_name_0();
  }
  declare get_name_0(): string;
  set name(v: string) {
    this.set_name_0(v);
  }
  declare set_name_0(v: string): void;
  get version(): string {
    return this.get_version_0();
  }
  declare get_version_0(): string;
  get args(): string[] {
    return this.get_args_0();
  }
  declare get_args_0(): string[];

// private:
  static readonly clazz_name = "Simple";
  declare init_native(name: string): void;
}