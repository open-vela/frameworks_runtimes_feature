let test = require('jsonobject_test');

let data_1 = {
    'page_count' : 50,
    'title': 'chapter one',
    'chapter_pages': [1, 2, 3, 4, 5],
    'chapters': {
        'chap_1' : 'chap 1',
        'chap_2' : 'chap 2',
        'chap_3' : 'chap 3'
    }
}

let data_2 = {
    'page_count' : 80,
    'title': 'chapter two',
    'chapter_pages': [6, 7, 8, 9],
    'chapters': {
        'chap_4' : 'chap 4',
        'chap_5' : 'chap 5',
        'chap_6' : 'chap 6'
    }
}
console.log('test.set_data: pass null')
test.set_data()
console.log('test.set_data: pass an object')
test.set_data(data_1)

let json_data = test.get_data()
console.log('test.get_data: ', JSON.stringify(json_data, null, 2))

console.log('test.set_data_array: pass null')
test.set_data_array()
console.log('test.set_data_array: pass an object array')
test.set_data_array([data_1, data_2])

let json_array = test.get_data_array()
console.log('test.get_data_array: ', JSON.stringify(json_array, null, 2))

let book = {
    'meta' : {
        'author': 'nobody',
        'class': 'classic',
        'age': '1980'
    },
    'title': 'chapter two',
    'chapters': [
        {'chap_1' : {'title': 'chap 1', 'pages': 20}},
        {'chap_2' : {'title': 'chap 2', 'pages': 30}},
        {'chap_3' : {'title': 'chap 3', 'pages': 15}},
    ]
}
console.log('test.set_book: pass null')
test.set_book()
console.log('test.set_book: pass an book object')
test.set_book(book)

console.log("will call json_promise() as promise");
test.json_promise(1).then(a => {
    console.log("json_promise promise resolve, json:", JSON.stringify(a, null, 2));
}).catch((data) => {
    console.log("json_promise promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    console.log("json_promise promise finally");
})
test.json_promise(2).then(a => {
    console.log("json_promise promise resolve, json:", JSON.stringify(a, null, 2));
}).catch((data) => {
    console.log("json_promise promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    console.log("json_promise promise finally");
})
console.log("did call json_promise() as promise");

console.log("will call json_promise() as callbacks");
test.json_promise(1, {
    success: (a) => { console.log("json_promise callback success, json:", JSON.stringify(a, null, 2)); },
    fail: (code, msg) => { console.log("json_promise callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("json_promise callback complete"); }
  })

test.json_promise(2, {
    success: (a) => { console.log("json_promise callback success, json:", JSON.stringify(a, null, 2)); },
    fail: (code, msg) => { console.log("json_promise callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("json_promise callback complete"); }
  })
console.log("did call json_promise() as callbacks");

console.log("will call json_array_promise() as promise");
test.json_array_promise(1).then(a => {
    console.log("json_array_promise promise resolve, json:", JSON.stringify(a, null, 2));
}).catch((data) => {
    console.log("json_array_promise promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    console.log("json_array_promise promise finally");
})
test.json_array_promise(2).then(a => {
    console.log("json_array_promise promise resolve, json:", JSON.stringify(a, null, 2));
}).catch((data) => {
    console.log("json_array_promise promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    console.log("json_array_promise promise finally");
})
console.log("did call json_array_promise() as promise");

console.log("will call json_array_promise() as callbacks");
test.json_array_promise(1, {
    success: (a) => { console.log("json_array_promise callback success, json:", JSON.stringify(a, null, 2)); },
    fail: (code, msg) => { console.log("json_array_promise callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("json_array_promise callback complete"); }
  })

test.json_array_promise(2, {
    success: (a) => { console.log("json_array_promise callback success, json:", JSON.stringify(a, null, 2)); },
    fail: (code, msg) => { console.log("json_array_promise callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("json_array_promise callback complete"); }
  })
console.log("did call json_array_promise() as callbacks");

console.log('\n\n')
