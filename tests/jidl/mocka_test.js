let mocka_test1 = require('mockatest');
let mocka_test2 = require('mockatest'); // Using multiple mocka instances
let sip = require('Simple'); // feature to be test

/*
 *   example: demonstration for using mocka interface
*/
mocka_test1.test("suite1", "add", () => {
    mocka_test1.expect_true(2 == 1 + 1, "2 == 1 + 1");
    mocka_test1.expect_true(3 == 1 + 1, "3 == 1 + 1");

});

mocka_test1.test("suite2", "sub", () => {
    mocka_test1.expect_true(0 == 1 - 1, "0 == 1 - 1");
    mocka_test1.expect_true(-1 == 1 - 1, "-1 == 1 - 1");
});

/*
 * example: test for feature Simple_1_0
 *   int bar2(int[] values)
 *   string[] bar3()
*/
mocka_test1.test("Simple", "bar2", () => {
    mocka_test1.expect_true(sip.bar2([1, 2, 3]) == 4, "sip.bar2(1, 2, 3) == 4");
    mocka_test1.expect_true(sip.bar2([0, 0]) == 0, "sip.bar2(0, 0) == 0");
    mocka_test1.expect_true(sip.bar3()[0] == "hello", "sip.bar3()[0] == \"hello\"");
});

/* example: Using multiple mocka instances
*/
mocka_test2.test("suite3", "test1", () => {
    mocka_test2.expect_true(true);
});

mocka_test2.test("suite4", "test2", () => {
    mocka_test2.expect_true(39);
});

// execute runAllOnce in the end
mocka_test1.runAllOnce();
mocka_test2.runAllOnce();
