let struct = require('struct_test');
let errorFlag = true;
init_feat_filter("structTest.*");

function failCb(msg, code) {
    console.log(msg);
    console.log(code);
}

// 用于接受异常上报，errorFlag设置为false，如果调用完需要重置errorFlag
function errorCheckCb(msg, code) {
    errorFlag = false;
}

feat_test("structTest", "testSimpleDefault", () => {
    try {
        feat_expect_true(struct.testSimpleDefault({fail: failCb}), "Simple默认值测试 case1 参数全不填 error!");
        feat_expect_true(struct.testSimpleDefault({fail: failCb, testxxsx: 1}), "Simple默认值测试 case2 参数多填 error!");
    } catch (e) {
        feat_expect_true(false, "Simple默认值测试 case error!");
    }
});

feat_test("structTest", "testSimpleDefault_异常", () => {
    errorFlag = true;
    struct.testSimpleDefault({fail: errorCheckCb, int_test: "test"});
    feat_expect_true(errorFlag == false, "Simple默认值异常测试 case1 int_test参数类型错误 error!")
    errorFlag = true;

    struct.testSimpleDefault({fail: errorCheckCb, title: 1});
    feat_expect_true(errorFlag == false, "Simple默认值异常测试 case2 title参数类型错误 error!")
    errorFlag = true;

});

feat_test("structTest", "testSimpleDefault1", () => {
    let data = struct.testSimpleDefault1();
    feat_expect_true(data.int_test == 2, "Simple作为返回值测试 case 1 error!");
    feat_expect_true(data.boolean_test == false, "Simple作为返回值测试 case 2 error!");
    feat_expect_true(data.long_test == 20000000000, "Simple作为返回值测试 case 3 error!");
    feat_expect_true(data.uint_test == 2, "Simple作为返回值测试 case 4 error!");
    feat_expect_true(almostEqualFloat(data.float_test, 3.4, 1e-5), "Simple作为返回值测试 case 5 error!");
    feat_expect_true(almostEqualFloat(data.double_test, 1.234, 1e-5), "Simple作为返回值测试 case 6 error!");
    feat_expect_true(data.title == "hello world", "Simple作为返回值测试 case 7 error!");
});

feat_test("structTest", "testSimpleDefault1_异常", () => {
    errorFlag = true;
    feat_expect_true(true, "case 1 error!");
});

feat_test("structTest", "testComplex", () => {
    let params = {
        fail: failCb,
        object_test1: 1,
        object_test2: 'hello world',
        simple: {fail: null},
        string_array: ["hello world", "hello world"],
        double_array: [5.2364, 5.2364],
        test_cb: function (data) {
            feat_expect_true(data, "Complex测试回调 case1 error!")
        }
    };
    feat_expect_true(struct.testComplex(params), "Complex测试 case 1 error!");
});

feat_test("structTest", "testComplex_异常", () => {
    errorFlag = true;
    let params = {
        fail: errorCheckCb,
        object_test1: 1,
        object_test1: 'hello world',
        simple: {fail: null},
        string_array: 'hello world',
        double_array: [5.2364, 5.2364],
        test_cb: function (data) {
            feat_expect_true(data, "Complex测试回调 case1 error!")
        }
    };
    struct.testComplex(params);
    feat_expect_true(errorFlag == false, "Complex异常测试 case 1 string_array数组类型错误 error!")
    errorFlag = true;

    params = {
        fail: errorCheckCb,
        object_test1: 1,
        object_test1: 'hello world',
        simple: [],
        string_array: ['hello world'],
        double_array: [5.2364, 5.2364],
        test_cb: function (data) {
            feat_expect_true(data, "Complex测试回调 2 error!")
        }
    };

    struct.testComplex(params);
    feat_expect_true(errorFlag == false, "Complex异常测试 case 2 simple数组类型错误 error!")
    errorFlag = true;

});

feat_test("structTest", "testComplex1", () => {
    let data = struct.testComplex1();
    feat_expect_true(arraysEqual(data.string_array, ['hello', 'world']), "Complex作为返回值测试 case 1 error!");
    feat_expect_true(data.object_test1 == 1, "Complex作为返回值测试 case 2 error!");
    feat_expect_true(data.object_test2 == "hello world", "Complex作为返回值测试 case 3 error!");
    let simple = data.simple;
    feat_expect_true(simple.uint_test == 2, "Complex作为返回值测试 case 4 error!");
    feat_expect_true(almostEqualFloat(simple.float_test, 3.4, 1e-5), "Complex作为返回值测试 case 5 error!");
    feat_expect_true(almostEqualFloat(simple.double_test, 1.234, 1e-5), "Complex作为返回值测试 case 6 error!");
    feat_expect_true(simple.title == "hello world", "Complex作为返回值测试 case 7 error!");
    feat_expect_true(simple.int_test == 2, "Complex作为返回值测试 case 8 error!");
    feat_expect_true(simple.boolean_test == false, "Complex作为返回值测试 case 9 error!");
    feat_expect_true(simple.long_test == 20000000000, "Complex作为返回值测试 case 10 error!");
    feat_expect_true(almostEqualFloat(data.double_array[0], 1.1234, 1e-5), "Complex作为返回值测试 case 11 error!");
    feat_expect_true(almostEqualFloat(data.double_array[1], 2.2727, 1e-5), "Complex作为返回值测试 case 12 error!");
});

feat_test("structTest", "testNested", () => {
    feat_expect_true(struct.testNested({fail: failCb, subs: null}), "Nested测试 case 1 error!");
});

feat_test("structTest", "testNested_异常", () => {
    errorFlag = true;
    feat_expect_true(struct.testNested({fail: failCb, subs: null}), "Nested异常测试 case 1 error!");
});

feat_test("structTest", "testNested1", () => {
    let data = struct.testNested1();
    feat_expect_true(arraysEqual(data.subs, []), "Nested作为返回值测试 case 1 error!");
    feat_expect_true(data.a == 2, "Nested作为返回值测试 case 2 error!");
});
