let crypto = require('system.crypto');

feat_test("cryptoTest", "hashDigest", () => {
    let ret1 = crypto.hashDigest({
        data: 'hello world',
    });
    feat_expect_true(ret1 === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "digest case 1 error!");

    let ret2 = crypto.hashDigest({
        uri: '',
        algo: 'SHA512'
    });
    feat_expect_true(typeof ret2 === 'string', "digest case 2 error!");

    let ret3 = crypto.hashDigest({
        algo: 'MD5'
    });
    feat_expect_true(typeof ret3 === 'string', "digest case 3 error!");

    let ret4 = crypto.hashDigest({});
    feat_expect_true(typeof ret4 === 'string', "digest case 4 error!");

    let str = 'Uint8Array text';
    const u8Arr1 = new Uint8Array(str.length);
    for (let i = 0, strLen = str.length; i < strLen; i++) {
        u8Arr1[i] = str.charCodeAt(i);
    }
    let ret5 = crypto.hashDigest({
        data: u8Arr1,
        algo: 'MD5'
    })
    feat_expect_true(ret5 === "18ea9d80ba6a693a6919f27567935098", "digest case 5 error!");

    let u8Arr2 = new Uint8Array([1, 3, 5, 7, 9]);
    let ret6 = crypto.hashDigest({
        data : u8Arr2,
        algo: 'MD5'
    });
    feat_expect_true(ret6 === '75e966753520b561f007b366d58743ee', "digest case 6 error!");

    let ret7 = crypto.hashDigest({
        data : 'text to be digested',
        algo: 'SHA256'
    });
    feat_expect_true(ret7 === "7f7b5c3a78e022b8974575b7e045b4fd3678f8ae7d5fa56b48300fcfa9719e82", "digest case 7 error!");

    let ret8 = crypto.hashDigest({
        uri: 'internal://files/stack.png',
        algo: 'SHA1'
    });
    feat_expect_true(typeof ret8 === 'string', "digest case 8 error!");

    let ret9 = crypto.hashDigest({
        uri: 'internal://files/test.txt',
        algo: 'MD5'
    });
    feat_expect_true(typeof ret9 === 'string', "digest case 9 error!");
});

feat_test("cryptoTest", "hmacDigest", () => {
    let ret1 = crypto.hmacDigest({
        data: 'hello',
        algo: 'SHA512',
        key: 'b8950cfe5af681ba40d3989a030affdd9b89af3c3b8f3ba80b7080889643c99be12da3fe0e27527b1ca98b5fde2ec8daf2991fe53580358eba470a42417f5416',
        success: (res) => {
          console.log(`hmacDigest success: ${res.data}`);
          this.hmacData = res.data;
        },
        fail: (data, code) => {
          console.log(`### HmacDigest fail ### ${code}: ${data}`)
        }
      });
    feat_expect_true(ret1 === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "digest case 1 error!");
});
