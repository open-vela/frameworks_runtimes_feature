let configuration = require('configuration');
const regex = new RegExp(/[~`!@#$%^&*()\-_=+[\]{}|;:'",<>/?]/);
feat_test("configuration property", "configuration object", () => {
    var config = configuration.getLocale();
    var regex_check;
    feat_expect_true(typeof config === 'object', "config is object");

    feat_expect_true(typeof config.language === 'string', "config.language is object");
    regex_check = regex.test(config.language);
    feat_expect_true(regex_check < 1, "config.language does not contain special character");

    feat_expect_true(typeof config.countryOrRegion === 'string', "config.countryOrRegion is object");
    regex_check = regex.test(config.countryOrRegion);
    feat_expect_true(regex_check < 1, "config.countryOrRegion does not contain special character");

    print(config.language);
    print(config.countryOrRegion);
});