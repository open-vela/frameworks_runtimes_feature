feat_test("interlacing", "add", () => {
    feat_expect_true(2 == 1 + 1, "2 == 1 + 1"); // true
    feat_expect_true(3 == 1 + 1, "3 == 1 + 1"); // false
    feat_expect_true(5 == 2 + 3, "5 == 2 + 3"); // true
    feat_expect_true(1 == -1 + 1, "1 == -1 + 1"); // false
});

feat_async_test("async", "add", (done) => {
    new Promise(function (resolve, reject) {
        resolve(1 + 1);
    }).then((res) => {
        feat_expect_true(res == 3, "async add 1 + 1 == 3");
        done();
    });
});

console.log("hello:", [1,2,3], {"x":1, "y": "i am y"});
console.error("hello:", [1,2,3], {"x":1, "y": "i am y"});
