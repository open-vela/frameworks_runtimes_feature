let test = require('simple');

console.log('simple test begin ====================');
let foo_ret = test.foo();
console.log('foo_ret:', foo_ret);

try {
    console.log("testing passing null as number...")
    test.bar(null);
} catch (err) {
    console.log(`error happened as expected: ${err}`);
}

let bar_ret = test.bar(1);
console.log('bar_ret:', bar_ret);
test.bar(2, 1.234);

let goo_ret = test.goo(3.1415926);
console.log('goo_ret:', goo_ret);
test.goo();

test.doo();

console.log("testing passing null as string...")
test.hoo(null);

test.hoo("hello world");

let chap_array = [
    { page_count: 5, title: "chap 1", is_end: false },
    { page_count: 10, title: "chap 2", is_end: false },
    { page_count: 20, title: "chap 3", is_end: false },
    { page_count: 50, title: "chap 4", is_end: true }
]
test.set_chapter_array(chap_array);
let chap_array_ret = test.get_chapter_array();
console.log(`got chapter array: ${JSON.stringify(chap_array_ret)}`);

test.set_book({
    book_name: "monkey king",
    chap_1: {
        page_count: 10,
        title: "chap one",
        is_end: false
    },
    chap_changed: function (index, title) {
        console.log('chap_changed, index=', index, ', title=', title, '\n');
    },
    book_info: {
        author: "wuchengen",
        dynasty: "Qing dynasty",
        class: "literature"
    },
    chaps_info: [
        {
            title: "chap one",
            page_count: 10,
        },
        {
            title: "chap two",
            page_count: 50,
        },
    ]
});
let book_ret = test.get_book();
console.log(`got book 1: ${JSON.stringify(book_ret)}`);
let book_ret_2 = test.get_book();
console.log(`got book 2: ${JSON.stringify(book_ret_2)}`);

test.set_chapter({
    page_count: 10,
    title: "chap 1",
    is_end: false
});
let chap_ret = test.get_chapter();
console.log(`got chapter 1: ${JSON.stringify(chap_ret)}`);

test.set_chapter({
    page_count: 50,
    title: "chap 100",
    is_end: true
});
let chap_ret_2 = test.get_chapter();
console.log(`got chapter 100: ${JSON.stringify(chap_ret_2)}`);

test.moo(5, function (x, y, z) {
    console.log('moo:x=', x, ', y=', y, ',z=', z, '\n');
});

console.log("will call noo() as promise");
test.noo(true).then(a => {
    console.log("noo promise resolve, a:", a);
}).catch((data) => {
    console.log("noo promise reject, code:", data.code, ", msg:", data.data);
}).finally(() => {
    console.log("noo promise finally");
})
test.noo(false).then(a => {
    console.log("noo promise resolve, a:", a);
}).catch((data) => {
    console.log("noo promise reject, code:", data.code, ", msg:", data.data);
}).finally(() => {
    console.log("noo promise finally");
})
console.log("did call noo() as promise\n");

console.log("will call noo() as callbacks");
test.noo(true, {
    success: (a) => { console.log("noo callback success, a:", a); },
    fail: (msg, code) => { console.log("noo callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("noo callback complete"); }
})
test.noo(false, {
    success: (a) => { console.log("noo callback success, a:", a); },
    fail: (msg, code) => { console.log("noo callback fail, code:", code, ", msg:", msg); },
    complete: () => { console.log("noo callback complete"); }
})
console.log("did call noo() as callbacks\n\n");

console.log("will call poo() as promise");
test.poo(true).then(a => {
    console.log("poo promise resolve, a:", a);
}).catch((data) => {
    console.log("poo promise reject, code:", data.code, ", msg:", data.data);
}).finally(() => {
    console.log("poo promise finally");
})

test.poo(false).then(a => {
    console.log("poo promise resolve, a:", a);
}).catch((data) => {
    console.log("poo promise reject, code:", data.code, ", msg:", data.data);
}).finally(() => {
    console.log("poo promise finally");
})
console.log("did call poo() as promise\n");
console.log('simple test end =====================\n');


console.log('test events begin ====================');
function on_data_changed_1(data) {
    console.log("on data changed 1, data:", data);
};

function on_data_changed_2(data) {
    console.log("on data changed 2, data:", data);
};

function on_data_changed_3(data) {
    console.log("on data changed 3, data:", data);
};

function on_data_changed_4(data) {
    console.log("on data changed 4, data:", data);
};

let data_changed_array_1 = [
    on_data_changed_2,
    on_data_changed_3
]

let data_changed_array_2 = [
    on_data_changed_1,
    on_data_changed_4
]

console.log("testing event data_changed = on_data_changed_1")
test.data_changed = on_data_changed_1
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed = data_changed_array_1")
test.data_changed = data_changed_array_1
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed on 'on_data_changed_4'")
test.on('data_changed', on_data_changed_4)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed off 'on_data_changed_4'")
test.off('data_changed', on_data_changed_4)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed on 'data_changed_array_2'")
test.on('data_changed', data_changed_array_2)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event data_changed off 'data_changed_array_2'")
test.off('data_changed', data_changed_array_2)
test.invoke_event('data_changed')
console.log('\n\n')

console.log("testing event array data_changed = data_changed_array_2")
test.data_changed = data_changed_array_2
test.invoke_event('data_changed')
console.log('\n\n')

function on_state_changed_1(state) {
    console.log("on state changed 1, state:", state);
};

function on_state_changed_2(state) {
    console.log("on state changed 2, state:", state);
};

function on_state_changed_3(state) {
    console.log("on state changed 3, state:", state);
};

function on_state_changed_4(state) {
    console.log("on state changed 4, state:", state);
};

let state_changed_array_1 = [
    on_state_changed_2,
    on_state_changed_3
]

let state_changed_array_2 = [
    on_state_changed_1,
    on_state_changed_4
]

console.log("testing event state_changed = on_state_changed_1")
test.state_changed = on_state_changed_1
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed = state_changed_array_1")
test.state_changed = state_changed_array_1
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed on 'on_state_changed_4'")
test.on('state_changed', on_state_changed_4)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed off 'on_state_changed_4'")
test.off('state_changed', on_state_changed_4)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed on 'state_changed_array_2'")
test.on('state_changed', state_changed_array_2)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed off 'state_changed_array_2'")
test.off('state_changed', state_changed_array_2)
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed = state_changed_array_2")
test.state_changed = state_changed_array_2
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event state_changed = null")
test.state_changed = null
test.invoke_event('state_changed')
console.log('\n\n')

console.log("testing event getter")
test.state_changed = state_changed_array_2
let state_changed_handlers = test.state_changed
console.log('state_changed_handlers:\n', state_changed_handlers)
console.log('\n\n')

console.log("testing invalid events")
test.invoke_event('invalid_event')
console.log('\n\n')
console.log('test events end ====================');


console.log('test arraybuffer begin ====================')
let buffer_1 = new Uint8Array([10, 9, 8, 7, 6]);
console.log('test.set_buffer(buffer_1) first time');
test.set_buffer(buffer_1.buffer);
console.log('test.set_buffer(buffer_1) second time');
test.set_buffer(buffer_1.buffer);
console.log('test.set_buffer(buffer_1) third time');
test.set_buffer(buffer_1.buffer);

let test_2 = require('simple');

let buff_copy = test.get_buffer_copy();
const buff_copy_view = new Int8Array(buff_copy);
console.log("test.get_buffer_copy(), view: ", buff_copy_view.toString());
console.log('test_2.set_buffer(buff_copy)');
test_2.set_buffer(buff_copy);

let buff_no_copy = test.get_buffer_no_copy();
const buff_no_copy_view = new Int32Array(buff_no_copy);
console.log("test.get_buffer_no_copy(), view: ", buff_no_copy_view.toString());
console.log('test_2.set_buffer(buff_no_copy)');
test_2.set_buffer(buff_no_copy);


let buff_0 = new Uint8Array([1, 2, 3, 4, 5]);
let buff_1 = new Uint8Array([5, 4, 3, 2, 1]);
let buff_2 = new Uint8Array([2, 1, 5, 4, 3]);
let buff_array = [buff_0.buffer, buff_1.buffer, buff_2.buffer]
console.log('test.set_buffer_array(buff_array)');
test.set_buffer_array(buff_array);

console.log("test.get_buffer_array_copy()");
let buff_array_copy = test.get_buffer_array_copy();
console.log('test_2.set_buffer_array(buff_array_copy)');
test_2.set_buffer_array(buff_array_copy);

console.log("test.get_buffer_array_no_copy()");
let buff_array_no_copy = test.get_buffer_array_no_copy();
console.log('test_2.set_buffer_array(buff_array_no_copy)');
test_2.set_buffer_array(buff_array_no_copy);
console.log('test arraybuffer end ====================')

// Interface has bugs, disable for now
/*
console.log('test interfaces begin ====================');
let dog = test.createDog(0);
console.log("dog: ", dog);
console.log("dog.name: ", dog.name, ", dog.legCount: ", dog.legCount)
dog.name = "dog2"
console.log("dog.name: ", dog.name, ", dog.legCount: ", dog.legCount)
dog.run(100, "xiaomi")
dog.eatFoods([
    "meet",
    "bone",
    "fish"
])

let airplane = test.createAirplane();
console.log("airplane.breed:", airplane.breed)
airplane.breed = "airplane2"
console.log("after set, airplane.breed:", airplane.breed)
let airplane_fly_ret = airplane.fly();
console.log(`airplane_fly_ret: ${JSON.stringify(airplane_fly_ret)}`);

let pigeon = test.createPigeon();
console.log("pigeon.name:", pigeon.name, ", legCount:", pigeon.legCount, ", breed:", pigeon.breed, ", weight:", pigeon.weight);
pigeon.name = "pigeon2";
pigeon.breed = "pigeon2";
pigeon.weight = pigeon.weight * 2;
console.log("after set, pigeon.name:", pigeon.name, ", legCount:", pigeon.legCount, ", breed:", pigeon.breed, ", weight:", pigeon.weight);
pigeon.eatFoods([
    "bug",
    "earthworm",
    "rice"
])
let pigeon_fly_ret = pigeon.fly();
console.log(`pigeon_fly_ret: ${JSON.stringify(pigeon_fly_ret)}`);
let cat = test.createCat();
console.log("cat.name:", cat.name)
cat.name = "cat2"
console.log("after set, cat.name:", cat.name)
cat.run(200, "home town")
test.setAnimal(cat)
console.log('test interfaces end ====================');
*/
