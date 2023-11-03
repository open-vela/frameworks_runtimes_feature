import { promise_test } from "./promise_test.d";

export function main() {
    // create feature module object
    let test = new promise_test();

    test.print("before call foo()");
    test.foo(10,  'hello').then((a: number) => {
        // console.log("foo resolve a: ", a);
        test.print("foo resolve a: ", a);
    }, (b: number) => {
        test.print("foo reject b: ", b);
    });
    test.print("after call foo()");

    test.print("before call use_foo()");
    test.use_foo(12).then((a: number) => {
        test.print("use_foo resolve a: ", a);
    }, (b: number) => {
        test.print("use_foo reject b: ", b);
    })
    test.print("after call use_foo()");

    test.print("before call foo1()");
    test.foo1(15).then((a: number) => {
        test.print("foo1 resolve a: ", a);
    }, (b: string) => {
        test.print("foo1 reject b: ", b);
    })
    test.print("after call foo1()");

    test.print("before call foo2()");
    test.foo2().then((a: number) => {
        test.print("foo2 resolve a: ", a);
    }, (b: string) => {
        test.print("foo2 reject b: ", b);
    })
    test.print("after call foo2()");

    test.print("before call bar()");
    test.bar().then((a: number[]) => {
        // console.log("bar resolve a: ", a);
        test.print("bar resolve a: ", a);
    }, (b: string[]) => {
        console.log("bar reject b: ", b);
        // test.print("bar reject b: ", b);
    })
    test.print("after call bar()");

    test.print("before call bar1()");
    test.bar1().then((a: number[]) => {
        console.log("bar1 resolve a: ", a);
        // test.print("bar1 resolve a: ", a);
    }, (b: string[]) => {
        console.log("bar1 reject b: ", b);
        // test.print("bar1 reject b: ", b);
    })
    test.print("after call bar1()");

    test.print("before call bar2()");
    test.bar2().then((a: number[]) => {
        test.print("bar2 resolve a: ", a);
    }, (b: string) => {
        test.print("bar2 reject b: ", b);
    })
    test.print("after call bar2()");
}
