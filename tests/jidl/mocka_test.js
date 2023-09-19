let gtest1 = require('mockatest');
let gtest2 = require('mockatest');
let sip = require('Simple_1_0');
// require('../gtest2.js');

gtest1.test("suite1", "add", () => {
    gtest1.expect_true(2 == 1 + 1, "2 == 1 + 1");
    gtest1.expect_true(3 == 1 + 1, "3 == 1 + 1");
});

gtest1.test("suite2", "sub", () => {
    gtest1.expect_true(0 == 1 - 1, "0 == 1 - 1");
    gtest1.expect_true(-1 == 1 - 1, "-1 == 1 - 1");
});

gtest2.test("suite3", "test1", () => {
    gtest2.expect_true(true);
});

gtest2.test("suite4", "test2", () => {
    gtest2.expect_true(39);
});

gtest1.runAllOnce();
gtest2.runAllOnce();
