let test = require('promise_callback');

console.log("will call foo_cb() as promise");
test.foo_cb(true).then(a => {
    console.log("foo_cb promise resolve, a:", a);
}).catch((data) => {
    console.log("foo_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("foo_cb promise finally");
})
test.foo_cb(false).then(a => {
    console.log("foo_cb promise resolve, a:", a);
}).catch((data) => {
    console.log("foo_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("foo_cb promise finally");
})
console.log("did call foo_cb() as promise\n");

console.log("will call foo_cb() as callbacks");
test.foo_cb(true, {
    success: (a) => { console.log("foo_cb callback success, a:", a); },
    fail: (msg, code) => { console.log("foo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("foo_cb callback complete"); }
  })
test.foo_cb(false, {
    success: (a) => { console.log("foo_cb callback success, a:", a); },
    fail: (msg, code) => { console.log("foo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("foo_cb callback complete"); }
  })
console.log("did call foo_cb() as callbacks\n\n");

console.log("will call bar_cb() as promise");
test.bar_cb(true).then(a => {
    console.log("bar_cb promise resolve, a:", a);
}).catch((data) => {
    console.log("bar_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("bar_cb promise finally");
})
test.bar_cb(false).then(a => {
    console.log("bar_cb promise resolve, a:", a);
}).catch((data) => {
    console.log("bar_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("bar_cb promise finally");
})
console.log("did call foo_cb() as promise\n");

console.log("will call bar_cb() as callbacks");
test.bar_cb(true, {
    success: (a) => { console.log("bar_cb callback success, a:", a); },
    fail: (msg, code) => { console.log("bar_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("bar_cb callback complete"); }
  })
test.bar_cb(false, {
    success: (a) => { console.log("bar_cb callback success, a:", a); },
    fail: (msg, code) => { console.log("bar_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("bar_cb callback complete"); }
  })
console.log("did call bar_cb() as callbacks\n");

console.log("will call void_cb() as promise");
test.void_cb(true).then(() => {
    console.log("void_cb promise resolve");
}).catch((data) => {
    console.log("void_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("void_cb promise finally");
})
test.void_cb(false).then(a => {
    console.log("void_cb promise resolve");
}).catch((data) => {
    console.log("void_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("void_cb promise finally");
})
console.log("did call void_cb() as promise\n");

console.log("will call void_cb() as callbacks");
test.void_cb(true, {
    success: () => { console.log("void_cb callback success"); },
    fail: (msg, code) => { console.log("void_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("void_cb callback complete"); }
  })
test.void_cb(false, {
    success: () => { console.log("void_cb callback success"); },
    fail: (msg, code) => { console.log("void_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("void_cb callback complete"); }
  })
console.log("did call void_cb() as callbacks\n");

console.log("will call goo_cb() as promise");
test.goo_cb(true).then(a => {
    console.log("goo_cb promise resolve a: ", JSON.stringify(a));
}).catch((data) => {
    console.log("goo_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("goo_cb promise finally");
})
test.goo_cb(false).then(a => {
    console.log("goo_cb promise resolve a: ", JSON.stringify(a));
}).catch((data) => {
    console.log("goo_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("goo_cb promise finally");
})
console.log("did call goo_cb() as promise\n");

console.log("will call goo_cb() as callbacks");
test.goo_cb(true, {
    success: (a) => { console.log("goo_cb callback success, a:", JSON.stringify(a)); },
    fail: (msg, code) => { console.log("goo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("goo_cb callback complete"); }
  })
test.goo_cb(false, {
    success: (a) => { console.log("goo_cb callback success, a:", JSON.stringify(a)); },
    fail: (msg, code) => { console.log("goo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("goo_cb callback complete"); }
  })
console.log("did call goo_cb() as callbacks\n");

console.log("will call moo_cb() as promise");
test.moo_cb(true).then(a => {
    console.log("moo_cb promise resolve a: ", JSON.stringify(a));
}).catch((data) => {
    console.log("moo_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("moo_cb promise finally");
})
test.moo_cb(false).then(a => {
    console.log("moo_cb promise resolve a: ", JSON.stringify(a));
}).catch((data) => {
    console.log("moo_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("moo_cb promise finally");
})
console.log("did call moo_cb() as promise\n");

console.log("will call moo_cb() as callbacks");
test.moo_cb(true, {
    success: (a) => { console.log("moo_cb callback success, a:", JSON.stringify(a)); },
    fail: (msg, code) => { console.log("moo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("moo_cb callback complete"); }
  })
test.moo_cb(false, {
    success: (a) => { console.log("moo_cb callback success, a:", JSON.stringify(a)); },
    fail: (msg, code) => { console.log("moo_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("moo_cb callback complete"); }
  })
console.log("did call moo_cb() as callbacks\n");

console.log("will call obj_cb() as promise");
test.obj_cb(true, {
	page_count: 30,
	title: "nice to",
	is_end: false,
  }).then(a => {
    console.log("obj_cb promise resolve, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end);
}).catch((data) => {
    console.log("obj_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("obj_cb promise finally");
})
test.obj_cb(false, {
	page_count: 30,
	title: "meet you",
	is_end: true,
  }).then(a => {
    console.log("obj_cb promise resolve, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end);
}).catch((data) => {
    console.log("obj_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("obj_cb promise finally");
})
console.log("did call obj_cb() as promise\n");

console.log("will call obj_cb() as callbacks");
test.obj_cb(true, {
    page_count: 20,
    title: "hello",
    is_end: false,
    success: (a) => { console.log("obj_cb callback success, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end); },
    fail: (msg, code) => { console.log("obj_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("obj_cb callback complete"); }
  })
test.obj_cb(false, {
    page_count: 20,
    title: "world",
    is_end: true,
    success: (a) => { console.log("obj_cb callback success, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end); },
    fail: (msg, code) => { console.log("obj_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("obj_cb callback complete"); }
  })
console.log("did call obj_cb() as callbacks\n");

console.log("will call struct_cb() as promise");
test.struct_cb(true).then(a => {
    console.log("struct_cb promise resolve, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end);
}).catch((data) => {
    console.log("struct_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("struct_cb promise finally");
})
test.struct_cb(false).then(a => {
    console.log("struct_cb promise resolve, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end);
}).catch((data) => {
    console.log("struct_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("struct_cb promise finally");
})
console.log("did call struct_cb() as promise\n");

console.log("will call struct_cb() as callbacks");
test.struct_cb(true, {
    success: (a) => { console.log("struct_cb callback success, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end); },
    fail: (msg, code) => { console.log("struct_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("struct_cb callback complete"); }
  })
test.struct_cb(false, {
    success: (a) => { console.log("struct_cb callback success, page_count:", a.page_count, ", title:", a.title, ", is_end:", a.is_end); },
    fail: (msg, code) => { console.log("struct_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("struct_cb callback complete"); }
  })
console.log("did call struct_cb() as callbacks\n");

console.log("will call struct_array_cb() as promise");
test.struct_array_cb(true).then(a => {
    console.log("struct_array_cb promise resolve, chap_array:", JSON.stringify(a));
}).catch((data) => {
    console.log("struct_array_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("struct_array_cb promise finally");
})
test.struct_array_cb(false).then(a => {
    console.log("struct_array_cb promise resolve, chap_array:", JSON.stringify(a));
}).catch((data) => {
    console.log("struct_array_cb promise reject, code:", data.code, ", data:", data.data);
}).finally(() => {
    console.log("struct_array_cb promise finally");
})
console.log("did call struct_cb() as promise\n");

console.log("will call struct_array_cb() as callbacks");
test.struct_array_cb(true, {
    success: (a) => { console.log("struct_array_cb callback success, chap_array:", JSON.stringify(a)); },
    fail: (msg, code) => { console.log("struct_array_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("struct_array_cb callback complete"); }
  })
test.struct_array_cb(false, {
    success: (a) => { console.log("struct_array_cb callback success, chap_array:", JSON.stringify(a)); },
    fail: (msg, code) => { console.log("struct_array_cb callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("struct_array_cb callback complete"); }
  })
console.log("did call struct_array_cb() as callbacks\n");
