var struct = require("Struct");
console.log("struct: ", struct);

struct.foo(1, {});
var Chapter = struct.bar(1024);
console.log("Chapter returned by bar: ", Chapter);
struct.bar2({
    'page_count' : 1000,
    'title': 'my book',
    chap_titles : ['section1', 'section2', 'section3'],
    'first_chap': {
        'page_count' : 100,
        'title': 'chapter one',
        'is_end': false
    },
    'chap_changed': function (index, title) {
        console.log('~~~~chapter changed, index: ', index, ', title: ', title)
    }
})

struct.proto(1, { main_monitor: { width: 123, height: 456, colorDepth: 8 }, name: "NAME", price: 666, sn_code: "2.3.4.5.6.7" })
struct.proto_cb((val)=>{
    console.log(val)
})
