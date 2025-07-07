let test = require('unit_const_test');
init_feat_filter("constTest.*");

feat_test("constTest", "constValTest", () => {
    feat_expect_true(test.num_val === 50, "constValTest case 1 error!");
    feat_expect_true(test.str_val === "hello world", "constValTest case 2 error!");
    feat_expect_true(test.bool_val === true, "constValTest case 3 error!");
    feat_expect_true(almostEqualFloat(test.float_val, 3.14, 1e-5), "constValTest case 4 error!");
})

feat_test("constTest", "constEnumTest1", () => {
    feat_expect_true(test.DATA_TYPES.GPS === 0, "constEnumTest1 case 1 error!");
    feat_expect_true(test.DATA_TYPES.COMPASS === 3, "constEnumTest1 case 2 error!");
    feat_expect_true(test.DATA_TYPES.PROXIMITY === 4, "constEnumTest1 case 3 error!");
    feat_expect_true(test.DATA_TYPES.LIGHT === 5, "constEnumTest1 case 4 error!");
    feat_expect_true(test.DATA_TYPES.GRAVITY === 8, "constEnumTest1 case 5 error!");
})

feat_test("constTest", "constEnumTest2", () => {
    feat_expect_true(test.Color.E_RED === "red", "constEnumTest2 case 1 error!");
    feat_expect_true(test.Color.E_BLUE === "blue", "constEnumTest2 case 2 error!");
    feat_expect_true(test.Color.E_GREEN === "green", "constEnumTest2 case 3 error!");
})

feat_test("constTest", "constMixObjectTest", () => {
    feat_expect_true(test.mix_const.num_val === 10, "constMixObjectTest case 1 error!");
    feat_expect_true(test.mix_const.str_val === "hello", "constMixObjectTest case 2 error!");
    feat_expect_true(test.mix_const.bool_val === false, "constMixObjectTest case 3 error!");
    feat_expect_true(almostEqualFloat(test.float_val, 3.14, 1e-5), "constMixObjectTest case 4 error!");
})

feat_test("constTest", "constNestObjectTest", () => {
    feat_expect_true(test.data.a === 0, "constNestObjectTest case 1 error!");
    feat_expect_true(test.data.b === 1, "constNestObjectTest case 2 error!");
    feat_expect_true(test.data.c === 2, "constNestObjectTest case 3 error!");
    feat_expect_true(test.data.d.e === "hello", "constNestObjectTest case 4 error!");
    feat_expect_true(test.data.d.f === true, "constNestObjectTest case 5 error!");
    feat_expect_true(test.data.d.g === 3, "constNestObjectTest case 6 error!");
    feat_expect_true(test.data.d.h.i === "world", "constNestObjectTest case 7 error!");
    feat_expect_true(test.data.d.h.j === 10, "constNestObjectTest case 8 error!");
    feat_expect_true(test.data.d.h.k === false, "constNestObjectTest case 9 error!");
})

