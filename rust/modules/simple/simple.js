let test = require('simple');

console.log('rust_simple test begin ====================');
let foo_ret = test.foo();
console.log('foo_ret:', foo_ret);

let bar_ret = test.bar(1);
console.log('bar_ret:', bar_ret);
test.bar(2, 1.234);

let goo_ret = test.goo(3.1415926);
console.log('goo_ret:', goo_ret);
test.goo();

test.doo();

test.hoo("hello world");

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
        title: "chap 1",
        is_end: false
    }
});
let book_ret = test.get_book();
console.log(`got book 1: ${JSON.stringify(book_ret)}`);
let book_ret_2 = test.get_book();
console.log(`got book 2: ${JSON.stringify(book_ret_2)}`);

test.moo(5, function(x, y, z) {
    console.log('moo:x=', x, ', y=', y, ',z=', z, '\n');
});

console.log("will call noo() as promise");
test.noo(true).then(a => {
    console.log("noo promise resolve, a:", a);
}).catch((data) => {
    console.log("noo promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    console.log("noo promise finally");
})
test.noo(false).then(a => {
    console.log("noo promise resolve, a:", a);
}).catch((data) => {
    console.log("noo promise reject, code:", data.code, ", msg:", data.msg);
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
    console.log("poo promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    console.log("poo promise finally");
})

test.poo(false).then(a => {
    console.log("poo promise resolve, a:", a);
}).catch((data) => {
    console.log("poo promise reject, code:", data.code, ", msg:", data.msg);
}).finally(() => {
    console.log("poo promise finally");
})
console.log("did call poo() as promise\n");
console.log('rust_simple test end =====================\n');

// Interface has bugs, disable for now
/*
console.log('test interfaces begin ====================');
let dog = test.createDog(0);
console.log("dog: ", dog);
console.log("dog.name: ", dog.name, ", dog.legCount: ", dog.legCount)
dog.name = "dog2"
console.log("dog.name: ", dog.name, ", dog.legCount: ", dog.legCount)
dog.run(100, "xiaomi")
dog.eatFood([
    "meet",
    "bone",
    "dog food"
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
pigeon.eatFood([
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
