let jsonobject = require('unit_jsonobject_test');

init_feat_filter("jsonobjectTest.*");

feat_test("jsonobjectTest", "testSetData", () => {
    feat_expect_true(jsonobject.set_data({test: 1}) === true, "SetData测试 case1 参数正确 error!");
    feat_expect_true(jsonobject.set_data({testxxsx: 1}) === false, "SetData测试 case2 参数错误 error!");
    feat_expect_true(jsonobject.set_data(null) === false, "SetData测试 case3 参数为null error!");
    let data = jsonobject.set_data(1);
    feat_expect_true(data instanceof Error, "SetData测试 case4 参数类型错误 error!");
});

feat_test("jsonobjectTest", "testGetData", () => {
    let data = jsonobject.get_data();
    feat_expect_true(data.test === 1, "GetData测试 case1 返回值 error!");
    data = jsonobject.get_data(1);
    feat_expect_true(data instanceof Error, "GetData测试 case2 参数多填 error!");
});

feat_test("jsonobjectTest", "testSetDataArray", () => {
    let data = jsonobject.set_data_array([{test: 1}, {test: 1}]);
    feat_expect_true(data === true, "SetDataArray测试 case1 参数正确 error!");
    // data = jsonobject.set_data_array([1, 2]);
    // feat_expect_true(data instanceof Error, "SetDataArray测试 case2 参数错误 error!");
    // data = jsonobject.set_data_array(null);
    // feat_expect_true(data === false, "SetDataArray测试  case3 参数为null error!");
});

feat_test("jsonobjectTest", "testGetDataArray", () => {
    let data = jsonobject.get_data_array();
    feat_expect_true(data[0] === 1 && data[1].test === 1, "GetDataArray测试 case1 返回值 error!");
});

feat_test("jsonobjectTest", "testSetBook", () => {
    let data = jsonobject.set_book({meta: {test: 1}, chapters: [{test: 1}, {test: 1}]});
    feat_expect_true(data === true, "SetBook测试 case1 参数正确 error!");
});

feat_test("jsonobjectTest", "testJsonPromise", () => {
    let params = {
        val: 0,
        success : function (data) {
            feat_expect_true(data.test === 1, "JsonPromise测试 case1 error!");
            done();
        },
        fail : function (msg, code) {
            feat_expect_true(code === 202, "JsonPromise测试 case2 error!");
            done();
        }
    };
    jsonobject.json_promise(params);

    jsonobject.json_promise({val: 1}).then(a => {
        feat_expect_true(a.test === 1, "JsonPromise异常测试 case2 error!");
    }).catch((data) => {
        feat_expect_true(data.code === 202, "JsonPromise异常测试 case2 error!");
        done();
    });

});

feat_async_test("jsonobjectTest", "testJsonPromise_异常", (done) => {
    let params = {
        val: 0,
        meta: 1,
        success : function (data) {
            feat_expect_true(data.test === 1, "JsonPromise异常测试 case1 error!");
            done();
        },
        fail : function (msg, code) {
            feat_expect_true(code=== 202, "JsonPromise异常测试 case2 error!");
            done();
        }
    };
    jsonobject.json_promise(params);

    // promise 调用
    jsonobject.json_promise({val: 1, meta: 1}).then(a => {
        feat_expect_true(a.test === 1, "JsonPromise异常测试 case2 error!");
    }).catch((data) => {
        feat_expect_true(data.code === 202, "JsonPromise异常测试 case2 error!");
        done();
    });
});

feat_async_test("jsonobjectTest", "testJsonArrayPromise", (done) => {
    let params = {
        val: 0,
        success : function (data) {
            feat_expect_true(data[0] === 1 && data[1].test === 1, "JsonArrayPromise测试 case1 error!");
            done();
        },
        fail : function (msg, code) {
            feat_expect_true(code=== 202, "JsonArrayPromise测试 case2 error!");
            done();
        }
    };
    jsonobject.json_array_promise(params);

    jsonobject.json_array_promise({val: 1}).then(a => {
        feat_expect_true(a[0] === 1 && a[1].test === 1, "JsonArrayPromise测试 case2 error!");
        done();
    }).catch((data) => {
        feat_expect_true(data.code === 202, "JsonArrayPromise测试 case2 error!");
        done();
    });
});
