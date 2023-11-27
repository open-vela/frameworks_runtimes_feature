feat_async_test("no timeout1", "add", (done) => {
    new Promise(function (resolve, reject) {
        resolve(1 + 1);
    }).then((res) => {
        feat_expect_true(res == 3, "async add 1 + 1 == 3");
        done();
    });
});

feat_async_test("timeout", "set timeout", (done)=> {
    new Promise(function (resolve, reject) {
        setTimeout(() => {
            resolve(1);
        }, 4000);
    }).then((res) => {
        feat_expect_true(res == 3, "async add 1 == 3");
        done();
    });
});

feat_async_test("no timeout2", "sub", (done) => {
    new Promise(function (resolve, reject) {
        resolve(5 - 1);
    }).then((res) => {
        feat_expect_true(res == 4, "async sub 5 - 1 == 4");
        done();
    });
});
