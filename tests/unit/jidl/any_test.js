let any = require('any_test');

function arraysEqual(arr1, arr2) {
    // 如果长度不同，直接返回 false
    if (arr1.length !== arr2.length) return false;

    // 逐个比较元素
    for (let i = 0; i < arr1.length; i++) {
        if (arr1[i] !== arr2[i]) return false;
    }

    return true;
}

feat_test("anyTest", "testSimple", () => {
    feat_expect_true(any.testSimple({test: "hello world"}), "AnyTest-Simple入参测试 case 1 error!");
});

feat_test("anyTest", "testSimple1", () => {
    let data = any.testSimple1();
    feat_expect_true(data.test == "hello world", "AnyTest-Simple返回值测试 case 1 error!");
});

feat_test("anyTest", "testComplex", () => {
    let params = {
        test_option: "hello world1",
        simple: {test: "hello world2"},
        any_array: ['hello world', 'hello world'],
    };
    feat_expect_true(any.testComplex(params), "AnyTest-Complex入参测试 case 1 error!");
});

feat_test("anyTest", "testComplex1", () => {
    let data = any.testComplex1();
    feat_expect_true(data.test_option == "hello world", "AnyTest-Complex返回值测试-option case 1 error!");
});

feat_test("anyTest", "testComplex2", () => {
    let data = any.testComplex2();
    feat_expect_true(data.simple.test == "hello world", "AnyTest-Complex返回值测试-struct case 1 error!");
});

feat_test("anyTest", "testComplex3", () => {
    let data = any.testComplex3();
    feat_expect_true(arraysEqual(data.any_array, ['hello', 'world']), "AnyTest-Complex返回值测试-array case 1 error!");
});

feat_test("anyTest", "testComplex4", () => {
    let data = any.testComplex4();
    feat_expect_true(data.test_option == "hello world" &&
        data.simple.test == "hello world1" &&
        arraysEqual(data.any_array, ['hello', 'world']),
        "AnyTest-Complex返回值测试 case 1 error!");
});

feat_test("anyTest", "testCallback", () => {
    let func = function (data) {
        feat_expect_true(data.test == "hello world", "AnyTest-Callback入参测试-struct case 1 error!");
    }
    feat_expect_true(any.testCallback(func), "AnyTest-Callback入参测试-struct case 1 error!");
});

feat_test("anyTest", "testObject", () => {
    feat_expect_true(any.testObject("obj test"), "AnyTest-object入参测试 case 1 error!");
});

feat_test("anyTest", "testObject1", () => {
    let data = any.testObject1();
    feat_expect_true(data == "obj test", "AnyTest-object返回值测试 case 1 error!");
});