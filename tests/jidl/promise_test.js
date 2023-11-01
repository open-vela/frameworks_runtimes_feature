let console = require('console');
let promise = require('promise_test');

promise.print("before call foo()");
promise.foo(10,  'hello').then(a => {
    promise.print("foo resolve a: ", a);
}, b => {
    promise.print("foo reject b: ", b);
})
promise.print("after call foo()");

promise.print("before call use_foo()");
promise.use_foo(12).then(a => {
    promise.print("use_foo resolve a: ", a);
}, b => {
    promise.print("use_foo reject b: ", b);
})
promise.print("after call use_foo()");

promise.print("before call foo1()");
promise.foo1(15).then(a => {
    promise.print("foo1 resolve a: ", a);
}, b => {
    promise.print("foo1 reject b: ", b);
})
promise.print("after call foo1()");

promise.print("before call foo2()");
promise.foo2().then(a => {
    promise.print("foo2 resolve a: ", a);
}, b => {
    promise.print("foo2 reject b: ", b);
})
promise.print("after call foo2()");

promise.print("before call bar()");
promise.bar().then(a => {
    promise.print("bar resolve a: ", a);
}, b => {
    promise.print("bar reject b: ", b);
})
promise.print("after call bar()");

promise.print("before call bar1()");
promise.bar1().then(a => {
    promise.print("bar1 resolve a: ", a);
}, b => {
    promise.print("bar1 reject b: ", b);
})
promise.print("after call bar1()");

promise.print("before call bar2()");
promise.bar2().then(a => {
    promise.print("bar2 resolve a: ", a);
}, b => {
    promise.print("bar2 reject b: ", b);
})
promise.print("after call bar2()");
