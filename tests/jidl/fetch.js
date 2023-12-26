let fetch = require('system.fetch');

feat_async_test("fetch", "fetch", (done) => {
  return new Promise(function (resolve, reject) {
    fetch.fetch({
      'url': 'http://httpbin.org/get',
      'data': {},
      'header':{},
      'method': "",
      'responseType': '',
      'timeout': 10000,
      'success': function (response) {
        print("hik succ headers");
        print(response.statusCode);
        //
        print(JSON.stringify(response.data));
        print("hik succ data");
        print(JSON.stringify(response.headers));
        resolve(true);
      },
      'fail': function (data, code) {
        print("hik err");
        resolve(true);
      },
      'complete': function () {
        print('hik complete');
      }
    });
  }).then((res) => {
    print('hik then');
    feat_expect_true(res, "fetch  success");
    done();
  }, (err) => {
    feat_expect_true(err, "fetch  fail");
    done();
  });

})


feat_async_test("fetch", "fetch", (done) => {
  return new Promise(function (resolve, reject) {
    fetch.fetch({
      'url': 'https://wis.qq.com/weather/common?source=pc&province=%E5%8C%97%E4%BA%AC&city=%E5%8C%97%E4%BA%AC&country=%E5%8C%97%E4%BA%AC&weather_type=forecast_1h',
      'responseType': 'json',
      // 'data': b,
      'header':{"Content-Type":"application/x-www-form-urlencoded"},
      'method': "",
      'responseType': '',
      'timeout': 10000,
      'success': function (response) {
        // const obj = response.data;
        // self.result2 = JSON.stringify(obj.data.forecast_1h[0]);
        print('the status code of the response: ${response.code}');
        print(response.code);
        print('the data of the response: ${response.data}');
        print(response.data)
        print('the headers of the response: ${JSON.stringify(response.headers)}');
        print(JSON.stringify(response.headers));
        resolve(true);
      },
      'fail': function (data, code) {
        print('handling fail, errMsg = ${data}');
        print(data);
        print('handling fail, errCode = ${data}');
        print(true);
        resolve(false);
      },
      'complete': function () {
        print('hik complete');
      }
    });
  }).then((res) => {
    print('hik then');
    feat_expect_true(res, "fetch  success");
    done();
  }, (err) => {
    feat_expect_true(err, "fetch  fail");
    done();
  });

})
