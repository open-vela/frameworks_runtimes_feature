let uploadtask = require('uploadtask');
let upload = null;
// 在外面定义,下面new 出来的对象在app 被销毁的时候回收?

function cb1(res) {
  print("Progress 1:")
  print(res.progress)
}

function cb2(res) {
  print("Progress 2:")
  print(res.progress)
}


feat_async_test("Uploadtask", "uploadFile", (done) => {
  return new Promise(function (resolve, reject) {
    upload = uploadtask.uploadFile({
      'url': process.env.UPLOAD_URL || "SET_UPLOAD_URL_ENV_VAR", // Please set UPLOAD_URL environment variable to your upload server
      'filePath': "hiktest.md",
      'name': "file",
      'header': { 'Content-Type': 'multipart/form-data' },
      'formData': {},
      'timeout': 10000,
      'success': function (res) {
        let s = JSON.stringify(res.headers);
        print(s);
        resolve(true);
      },
      'fail': function (data, code) {
        print('hik handling fail, errMsg');
        resolve(true);
      },
      'complete': function () {
        print('hik complete');
      }
    });

    upload.offProgressUpdate(cb1)

  }).then((res) => {
    print('hik then');
    feat_expect_true(res, "uploadtask  success");
    done();
  }, (err) => {
    feat_expect_true(err, "uploadtask  fail");
    done();
  });
})
