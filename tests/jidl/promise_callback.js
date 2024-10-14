let test = require('promise_callback');

test.print("will call foo_cb() as promise");
test.foo_cb(-2,  'hello').then(a => {
    test.print("foo_cb promise resolve, a:", a);
}).catch((data) => {
    test.print("foo_cb promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    test.print("foo_cb promise finally");
})
test.foo_cb(2,  'hello').then(a => {
    test.print("foo_cb promise resolve, a:", a);
}).catch((data) => {
    test.print("foo_cb promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    test.print("foo_cb promise finally");
})
test.print("did call foo_cb() as promise\n");

test.print("will call foo_cb() as callbacks");
test.foo_cb(-3,  'hello', {
    success: (a) => { test.print("foo_cb callback success, a:", a); },
    fail: (code, msg) => { test.print("foo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { test.print("foo_cb callback complete"); }
  })
test.foo_cb(3,  'hello', {
    success: (a) => { test.print("foo_cb callback success, a:", a); },
    fail: (code, msg) => { test.print("foo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { test.print("foo_cb callback complete"); }
  })
test.print("did call foo_cb() as callbacks\n\n");

test.print("will call bar_cb() as promise");
test.bar_cb(-2).then(a => {
    test.print("bar_cb promise resolve, a:", a);
}).catch((data) => {
    test.print("bar_cb promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    test.print("bar_cb promise finally");
})
test.bar_cb(2).then(a => {
    test.print("bar_cb promise resolve, a:", a);
}).catch((data) => {
    test.print("bar_cb promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    test.print("bar_cb promise finally");
})
test.print("did call foo_cb() as promise\n");

test.print("will call bar_cb() as callbacks");
test.bar_cb(-3, {
    success: (a) => { test.print("bar_cb callback success, a:", a); },
    fail: (code, msg) => { test.print("bar_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { test.print("bar_cb callback complete"); }
  })
test.bar_cb(3, {
    success: (a) => { test.print("bar_cb callback success, a:", a); },
    fail: (code, msg) => { test.print("bar_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { test.print("bar_cb callback complete"); }
  })
test.print("did call bar_cb() as callbacks\n");

test.print("will call obj_cb() as promise");
test.obj_cb({
	page_count: 30,
	title: "nice to",
	is_end: false,
  }).then(a => {
    test.print("obj_cb promise resolve, page_count:", a.page_count, ", title:", a.title);
}).catch((data) => {
    test.print("obj_cb promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    test.print("obj_cb promise finally");
})
test.obj_cb({
	page_count: 30,
	title: "meet you",
	is_end: true,
  }).then(a => {
    test.print("obj_cb promise resolve, page_count:", a.page_count, ", title:", a.title);
}).catch((data) => {
    test.print("obj_cb promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    test.print("obj_cb promise finally");
})
test.print("did call obj_cb() as promise\n");

test.print("will call obj_cb() as callbacks");
test.obj_cb({
	page_count: 20,
	title: "hello",
	is_end: false,
    success: (a) => { test.print("obj_cb callback success, page_count:", a.page_count, ", title:", a.title); },
    fail: (code, msg) => { test.print("obj_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { test.print("obj_cb callback complete"); }
  })
test.obj_cb({
	page_count: 20,
	title: "world",
	is_end: true,
    success: (a) => { test.print("obj_cb callback success, page_count:", a.page_count, ", title:", a.title); },
    fail: (code, msg) => { test.print("obj_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { test.print("obj_cb callback complete"); }
  })
test.print("did call bar_cb() as callbacks\n");

test.print("test loadLibrary function\n");
let err = test.loadLibrary("Error")
test.print("erro mse for code 100:", err.strerror(100));
