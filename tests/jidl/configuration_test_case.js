let configuration = require('configuration');

feat_test("configuration property", "configuration object", () => {
    var config = configuration.getLocale();
    feat_expect_true(typeof config === 'object', "config is object");
    print(config.language)
    feat_expect_true(typeof config.language === 'string', "config.language is object");
    feat_expect_true(typeof config.countryOrRegion === 'string', "config.countryOrRegion is object");
    print(config.countryOrRegion)
});
feat_test_all();