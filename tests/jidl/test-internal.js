let unittest = require('feat_test');

function feat_test(name, desc, cb) {
    unittest.testsuite(name, desc, cb);
}

function feat_async_test(suitname, desc, test_cb) {
    var async_id;
    async_id = unittest.testsuite(suitname, desc, () => {
        test_cb(() => unittest.done(async_id));
    }, true);
}

function __feat_test_all() { // hide to outside
    unittest.run_all_tests();
}

function feat_expect_true(r, d) {
    return unittest.expect_true(r, d);
}

function print(a) {
    unittest.print(a);
}
