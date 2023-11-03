let device = require('device');

feat_test("device info property", "info object", () => {
    var device_info = device.getInfo();
    feat_expect_true(typeof device_info === 'object', "device_info is object");
    feat_expect_true(typeof device_info.brand === 'string', "brand is object");
    feat_expect_true(typeof device_info.IMEI === 'string', "IMEI is object");
    feat_expect_true(typeof device_info.manufacturer === 'string', "manufacturer is object");
    feat_expect_true(typeof device_info.product === 'string', "product is object");
    feat_expect_true(typeof device_info.osType === 'string', "osType is object");
    feat_expect_true(typeof device_info.osVersionName === 'string', "osVersionName is object");
    feat_expect_true(typeof device_info.osVersionCode === 'number', "osVersionCode is number");
    feat_expect_true(typeof device_info.platformVersionName === 'string', "platformVersionName is object");
    feat_expect_true(typeof device_info.platformVersionCode === 'number', "platformVersionCode is number");
    feat_expect_true(typeof device_info.APILevel === 'number', "APILevel is number");
    feat_expect_true(typeof device_info.language === 'string', "language is object");
    feat_expect_true(typeof device_info.region === 'string', "region is object");
    feat_expect_true(typeof device_info.screenWidth === 'number', "screenWidth is number");
    feat_expect_true(typeof device_info.screenHeight === 'number', "screenHeight is number");
    feat_expect_true(typeof device_info.deviceType === 'string', "deviceType is object");
    feat_expect_true(typeof device_info.screenShape === 'string', "screenShape is object");
});

feat_test("device info api", "IDs", () => {
    var id = device.getDeviceid();
    feat_expect_true(typeof id === 'string', "dId is string");
    id = device.getid();
    feat_expect_true(typeof id === 'string', "id is string");
    id = device.getserial();
    feat_expect_true(typeof id === 'string', "serial is string");
});

feat_test("device info api", "storage", () => {
    var ssize = device.gettotalstorage();
    print(ssize);
    feat_expect_true(typeof ssize === 'string', "total storage is string");
    ssize = Number(ssize);
    feat_expect_true(typeof ssize === 'number', "total storage is transform to number");
    feat_expect_true(ssize >= 1000000, "total storage exceeds 1M");

    ssize = device.getavailablestorage() 
    print(ssize);
    feat_expect_true(typeof ssize === 'string', "avaliable storage is string");
    ssize = Number(ssize);
    feat_expect_true(typeof ssize === 'number', "avaliable storage is transform to number");
    feat_expect_true(ssize >= 1000000, "avaliable storage exceeds 1M");
});
feat_test_all();