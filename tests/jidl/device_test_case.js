let device = require('device');
const regex = new RegExp(/[~`!@#$%^&*()\-_=+[\]{}|;:'",<>/?]/);
feat_test("device info property", "info object", () => {
    var device_info = device.getInfo();
    var regex_check, tmp;
    feat_expect_true(typeof device_info === 'object', "device_info is object");

    feat_expect_true(typeof device_info.brand === 'string', "brand is object");
    regex_check = regex.test(device_info.brand);
    feat_expect_true(regex_check < 1, "brand does not contain special character");

    feat_expect_true(typeof device_info.IMEI === 'string', "IMEI is object");
    regex_check = regex.test(device_info.IMEI);
    feat_expect_true(regex_check < 1, "IMEI does not contain special character");

    feat_expect_true(typeof device_info.manufacturer === 'string', "manufacturer is object");
    regex_check = regex.test(device_info.manufacturer);
    feat_expect_true(regex_check < 1, "manufacturer does not contain special character");

    feat_expect_true(typeof device_info.product === 'string', "product is object");
    regex_check = regex.test(device_info.product);
    feat_expect_true(regex_check < 1, "product does not contain special character");

    feat_expect_true(typeof device_info.osType === 'string', "osType is object");
    regex_check = regex.test(device_info.osType);
    feat_expect_true(regex_check < 1, "osType does not contain special character");

    feat_expect_true(typeof device_info.osVersionName === 'string', "osVersionName is object");
    regex_check = regex.test(device_info.osVersionName);
    feat_expect_true(regex_check < 1, "osVersionName does not contain special character");

    feat_expect_true(typeof device_info.osVersionCode === 'number', "osVersionCode is number");
    feat_expect_true(device_info.osVersionCode >= 0, "osVersionCode >= 0");

    feat_expect_true(typeof device_info.platformVersionName === 'string', "platformVersionName is object");
    regex_check = regex.test(device_info.platformVersionName);
    feat_expect_true(regex_check < 1, "platformVersionName does not contain special character");

    feat_expect_true(typeof device_info.platformVersionCode === 'number', "platformVersionCode is number");
    feat_expect_true(device_info.platformVersionCode >= 0, "platformVersionCode >= 0");

    feat_expect_true(typeof device_info.APILevel === 'number', "APILevel is number");
    feat_expect_true(device_info.APILevel >= 0, "APILevel >= 0");

    feat_expect_true(typeof device_info.language === 'string', "language is object");
    regex_check = regex.test(device_info.language);
    feat_expect_true(regex_check < 1, "language does not contain special character");

    feat_expect_true(typeof device_info.region === 'string', "region is object");
    regex_check = regex.test(device_info.region);
    feat_expect_true(regex_check < 1, "region does not contain special character");

    feat_expect_true(typeof device_info.screenWidth === 'number', "screenWidth is number");
    feat_expect_true(device_info.screenWidth > 0, "screenWidth > 0");

    feat_expect_true(typeof device_info.screenHeight === 'number', "screenHeight is number");
    feat_expect_true(device_info.screenHeight >= 0, "screenHeight >= 0");

    feat_expect_true(typeof device_info.deviceType === 'string', "deviceType is object");
    regex_check = regex.test(device_info.deviceType);
    feat_expect_true(regex_check < 1, "deviceType does not contain special character");
    if(device_info.deviceType == "watch" || device_info.deviceType == "band"
        || device_info.deviceType == "smartspeaker" || device_info.deviceType == "unknown")
    {
        tmp = true;
    }
    else
    {
        tmp = false;
    }
    feat_expect_true(tmp, "deviceType is supported");

    feat_expect_true(typeof device_info.screenShape === 'string', "screenShape is object");
    regex_check = regex.test(device_info.screenShape);
    feat_expect_true(regex_check < 1, "screenShape does not contain special character");
    if(device_info.screenShape == "rect" || device_info.screenShape == "circle"
        || device_info.screenShape == "unknown")
    {
        tmp = true;
    }
    else
    {
        tmp = false;
    }
    feat_expect_true(tmp, "screenShape is supported");
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