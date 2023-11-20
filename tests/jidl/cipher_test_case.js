let cipher = require('system.cipher');

const PRIVATEKEY = `-----BEGIN RSA PRIVATE KEY-----
MIIBOgIBAAJBAJcHvhAObAAy3K0AyfGXeF6ICBAKRHXPlmmd7FYWRNl2QlnTAyRA
nScIxHtwCKmWIoTrr7m+ChIo9v0a+BHb6PkCAwEAAQJANIkn7xPlM6h9pNxqYtSK
tW9iRpobuFNugezCQiva5T25EWsJv/LHzEi4bOJAgV10dKB9gIDR2t4k3zf2zsdt
UQIhAN1r+Yg7QoknX/ZYyNajFJxE38xamhQurCkinhBAAXOVAiEArp2lm+Q2x2mU
90MtcLaKcVQgH1OGh4oT7y2+4PvcRtUCIDqG7udmmpi8Uq5AG544bxs7TVir3ixV
heY9o0AyWu/dAiEAgACcqDyRU3k4dFHQe7G0pwMOUSh/k9hKaKjWJkM65MkCICPk
FG6+nnMbQ3OaQke1RAtzYyAr+VZH2BdfRt0j9Hjy
-----END RSA PRIVATE KEY-----`;

const PUBLICKEY = `-----BEGIN PUBLIC KEY-----
MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJcHvhAObAAy3K0AyfGXeF6ICBAKRHXP
lmmd7FYWRNl2QlnTAyRAnScIxHtwCKmWIoTrr7m+ChIo9v0a+BHb6PkCAwEAAQ==
-----END PUBLIC KEY-----`;

let rsaEncryptedData;
let rsaDecryptedData;

let signData;
let verifyResult;

let digestedData;
let md5Data;

let aesEncryptedData;
let aesDecryptedData;

feat_test("cipherTest", "rsa", () => {
    cipher.rsa({
        action: 'encrypt',
        text: 'hello',
        key: PRIVATEKEY,
        hashType: 'SHA1',
        success: (res) => {
          print(`rsa encrypt success: ${res.text}`)
          rsaEncryptedData = res.text;
        },
        fail: (data, code) => {
          print(`### rsa encrypt fail ### ${code}: ${data}`)
        },
        complete: () => {
          console.log('complete excute');
        }
    });
    
    cipher.rsa({
        action: 'decrypt',
        text: rsaEncryptedData,
        key: PUBLICKEY,
        hashType: 'SHA1',
        success: (res) => {
          print(`rsa decrypt success: ${res.text}`)
          rsaDecryptedData = res.text;
        },
        fail: (data, code) => {
          print(`### rsa decrypt fail ### ${code}: ${data}`)
        },
        complete: () => {
          console.log('complete excute');
        }
    });
    feat_expect_true(rsaDecryptedData === "hello", "rsa case 1 error!");
});

feat_test("cipherTest", "sign", () => {
    cipher.sign({
        action: 'decrypt',
        text: 'hello',
        key: PRIVATEKEY,
        hashType: 'SHA1',
        success: (res) => {
          print(`sign success: ${res.text}`)
          signData = res.text;
        },
        fail: (data, code) => {
          print(`### sign fail ### ${code}: ${data}`)
        },
        complete: () => {
          console.log('complete excute');
        }
    });
    feat_expect_true(signData === "KixeG2tgDBXi/aasw8WoqB5yFl+u4OvFbttRAnFOcufi6hmOtjAMB5sE3hvBgehRtN3dMSkiiYNR4e7+43Gbrg==", "sign case 1 error!");
});

feat_test("cipherTest", "verify", () => {
    cipher.verify({
        text: 'hello',
        key: PUBLICKEY,
        hashType: 'SHA1',
        signature: signData,
        success: (data) => {
          print(`verify success: ${data.valid}`)
          verifyResult = data.valid;
        },
        fail: (data, code) => {
          print(`### verify fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(verifyResult === true, "verify case 1 error!");
});

feat_test("cipherTest", "digest", () => {
    cipher.digest({
        text: 'hello',
        hashType: 'SHA512',
        success: (res) => {
          print(`digest success: ${res.text}`)
          digestedData = res.text;
        },
        fail: (data, code) => {
          print(`### digest fail ### ${code}: ${data}`)
        }
    });
    feat_expect_true(digestedData === "9b71d224bd62f3785d96d46ad3ea3d73319bfbc2890caadae2dff72519673ca72323c3d99ba5c11d7c7acc6e14b8c5da0c4663475c2e5c3adef46f73bcdec043", "digest case 1 error!");
});

feat_test("cipherTest", "md5", () => {
    cipher.md5({
        text: 'hello',
        success: (res) => {
          print(`md5 success: ${res.text}`)
          md5Data = res.text;
        },
        fail: (data, code) => {
          print(`### md5 fail ### ${code}: ${data}`)
        }
    });
    feat_expect_true(md5Data === "5d41402abc4b2a76b9719d911017c592", "md5 case 1 error!");
});

feat_test("cipherTest", "aes", () => {
    cipher.aes({
        action: 'encrypt',
        text: 'hello',
        key: PUBLICKEY,
        success: (res) => {
          print(`aes encrypt success: ${res.text}`)
          aesEncryptedData = res.text;
        },
        fail: (data, code) => {
          print(`### aes encrypt fail ### ${code}: ${data}`)
        }
    });
    feat_expect_true(aesData === "696b069cea69ea52571", "aes case 1 error!");

    cipher.aes({
        action: 'decrypt',
        text: aesEncryptedData,
        key: PRIVATEKEY,
        success: (res) => {
          print(`aes decrypt success: ${res.text}`)
          aesDecryptedData = res.text;
        },
        fail: (data, code) => {
          print(`### aes decrypt fail ### ${code}: ${data}`)
        }
    });
    feat_expect_true(aesDecryptedData === "hello", "aes case 2 error!");
});
