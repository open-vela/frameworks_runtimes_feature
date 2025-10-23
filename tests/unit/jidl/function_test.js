let func = require('function_test');
init_feat_filter("functionTest.*");

feat_test("functionTest", "intTest", () => {
    let data = func.intTest(77);
    feat_expect_true(data == 77, "intTest case 1 error!");
    // 参数异常, 预期返回error
    data = func.intTest("100");
    feat_expect_true(data instanceof Error, "intTest case 2 error!");
})

feat_test("functionTest", "booleanTest", () => {
    let data = func.booleanTest(true);
    feat_expect_true(data == true, "booleanTest case 1 error!");
    // 参数异常, 预期返回Error
    data = func.booleanTest("true");
    feat_expect_true(data instanceof Error, "intTest case 2 error!");
})

feat_test("functionTest", "uintTest", () => {
    let data = func.uintTest(100);
    feat_expect_true(data == 100, "uintTest case 1 error!");
    // uint参数范围错误, 不返回预期值
    data = func.uintTest(-100);
    feat_expect_true(data !== -100, "uintTest case 2 error!");
    // 参数异常, 预期返回Error
    data = func.uintTest("100");
    feat_expect_true(data instanceof Error, "uintTest case 3 error!");
})

feat_test("functionTest", "longTest", () => {
    let data = func.longTest(10000000000);
    feat_expect_true(data == 10000000000, "longTest case 1 error!");
    // 参数异常, 预期返回Error
    data = func.longTest("10000000000");
    feat_expect_true(data instanceof Error, "longTest case 2 error!");
})

feat_test("functionTest", "floatTest", () => {
    let data = func.floatTest(1.234);
    feat_expect_true(almostEqualFloat(data, 1.234, 1e-5), "floatTest case 1 error!");
    // 参数异常, 预期返回Error
    data = func.floatTest("1.234");
    feat_expect_true(data instanceof Error, "floatTest case 2 error!");
})

feat_test("functionTest", "doubleTest", () => {
    let data = func.doubleTest(1.23456);
    feat_expect_true(almostEqualFloat(data, 1.23456, 1e-5), "doubleTest case 1 error!");
    // 参数异常, 预期返回Error
    data = func.doubleTest("1.234");
    feat_expect_true(data instanceof Error, "doubleTest case 2 error!");
})

feat_test("functionTest", "strTest", () => {
    let data = func.strTest("hello world");
    feat_expect_true(data == "hello world", "strTest case 1 error!");
    // 参数异常, 预期返回Error
    data = func.strTest(100);
    feat_expect_true(data instanceof Error, "strTest case 2 error!");
    // str缺省, 预期返回""
    data = func.strTest();
    feat_expect_true(data == "", "strTest case 3 error!");
})

feat_test("functionTest", "objectTest_simple", () => {
    let data = func.objectTest(1.23456);
    feat_expect_true(almostEqualFloat(data, 1.23456, 1e-5), "objectTest_simple case 1 error!");
    data = func.objectTest("hello world");
    feat_expect_true(data == "hello world", "objectTest_simple case 2 error!");
    // 参数缺省, 预期返回null
    data = func.objectTest();
    feat_expect_true(data == null, "objectTest_simple case 3 error!");
})

// // 复杂类型obj
// feat_test("functionTest", "objectTest_complex", () => {
//     let data = func.objectTest({
//         int_test: 1,
//         float_test: 1.234,
//     });
//     feat_expect_true(data.int_test == 1 && almostEqualFloat(data.float_test, 1.23456, 1e-5), "objectTest_complex case 1 error!");
//     data = func.objectTest([1, 2, 3, "hello world"]);
//     feat_expect_true(arraysEqual(data, [1, 2, 3, "hello world"]), "objectTest_complex case 2 error!");
//     // Todo: arraybuffer...
// })

feat_test("functionTest", "cbTest", () => {
    let cb = function (data) {
        feat_expect_true(data == 200, "cbTest case 1 error!");
    }
    feat_expect_true(func.cbTest(cb), "cbTest case 2 error!");
    // 参数异常, 预期返回Error
    let data = func.cbTest(100);
    feat_expect_true(data instanceof Error, "cbTest case 2 error!");
})

feat_test("functionTest", "optionTest", () => {
    let data = func.optionTest();
    feat_expect_true(data == true, "optionTest case 1 error!");
    data = func.optionTest(1);
    feat_expect_true(data == true, "optionTest case 2 error!");
    data = func.optionTest(1, false);
    feat_expect_true(data == true, "optionTest case 3 error!");
    data = func.optionTest(1, false, 77, 10000000000, 1.123, 1.77777, "hello");
    feat_expect_true(data == true, "optionTest case 4 error!");
    // 参数异常(顺序错误), 预期返回undefined
    data = func.optionTest("hello world");
    feat_expect_true(data instanceof Error, "optionTest case 5 error!");
})

feat_test("functionTest", "optionTest1", () => {
    let data = func.optionTest1();
    feat_expect_true(data == true, "optionTest1 case 1 error!");
})

feat_test("functionTest", "optionTest2", () => {
    let data = func.optionTest2(1);
    feat_expect_true(data == true, "optionTest2 case 1 error!");
})

// 测试option位置异常
// feat_test("functionTest", "optionTest3", () => {
//     let data = func.optionTest3(1, 100, 1.1234);
//     feat_expect_true(data == true, "optionTest3 case 1 error!");
//     // 参数异常, 预期返回Error
//     data = func.optionTest3(1, 1.1234);
//     feat_expect_true(data instanceof Error, "optionTest3 case 2 error!");
// })

feat_test("functionTest", "variableTest", () => {
    let data = func.variableTest(1, 1.1234, "hello world");
    feat_expect_true(data == true, "variableTest case 1 error!");
    data = func.variableTest(1, true, [1, 1.1234, "hello world"]);
    feat_expect_true(data == true, "variableTest case 2 error!");
    data = func.variableTest();
    feat_expect_true(data, "variableTest case 3 error!");
})

feat_test("functionTest", "variableTest1", () => {
    let data = func.variableTest1(1, "hello", [1, 1.1234, "hello world"]);
    feat_expect_true(data, "variableTest1 case 1 error!");
    // 参数异常, 预期返回Error
    data = func.variableTest1(1, [1, 1.1234, "hello"]);
    feat_expect_true(data instanceof Error, "variableTest1 case 2 error!");
})

feat_test("functionTest", "throwErrorTest", () => {
    let data = func.throwErrorTest(false);
    feat_expect_true(data === "test", "throwErrorTest case 1 error!");

    try {
        data = func.throwErrorTest(true);
  	    feat_expect_true(false, "Expected an error to be thrown, but none was thrown");
    } catch (error) {
        feat_expect_true(true, "throwErrorTest case 2 error!");
    }
})

feat_test("functionTest", "throwErrorTest1", () => {
    try {
        func.throwErrorTest1(false);
        feat_expect_true(true, "Expected an error to be thrown, but none was thrown");
    } catch (error) {
        feat_expect_true(false, "throwErrorTest1 case 1 error!");
    }

    try {
        data = func.throwErrorTest1(true);
  	    feat_expect_true(false, "Expected an error to be thrown, but none was thrown");
    } catch (error) {
        feat_expect_true(, "throwErrorTest1 case 2 error!");
    }
})