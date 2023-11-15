let crypto = require('system.crypto');

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

let hmacData;
let signData;
let fileSignData;
let imgSignData;
let arrBuffSignDataText = "";
let arrBuffSignData = [];
let verifyResult;
let fileVerifyResult;
let imgVerifyResult;
let arrBuffSignResult = "";

let arrEncryptDataText = "";
let arrEnptAESDataText = "";
let arrEncryptData = [];
let arrEnptAESData = [];

let strPublicEncryptData = "";
let strPublicDecryptData = "";
let strPrivateEncryptData = "";
let strPrivateDecryptData = "";
let arrDecryptData = "";
let strEnptAESData = "";

feat_test("cryptoTest", "hashDigest", () => {
    let ret1 = crypto.hashDigest({
        data: 'hello world',
    });
    feat_expect_true(ret1 === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "hashDigest case 1 error!");
 
    let ret2 = crypto.hashDigest({
        uri: '',
        algo: 'SHA512'
    });
    feat_expect_true(typeof ret2 === 'string', "hashDigest case 2 error!");

    let ret3 = crypto.hashDigest({
        algo: 'MD5'
    });
    feat_expect_true(typeof ret3 === 'string', "hashDigest case 3 error!");

    let ret4 = crypto.hashDigest({});
    feat_expect_true(typeof ret4 === 'string', "hashDigest case 4 error!");

    let str = 'Uint8Array text';
    const u8Arr1 = new Uint8Array(str.length);
    for (let i = 0, strLen = str.length; i < strLen; i++) {
        u8Arr1[i] = str.charCodeAt(i);
    }
    let ret5 = crypto.hashDigest({
        data: u8Arr1,
        algo: 'MD5'
    })
    feat_expect_true(ret5 === "18ea9d80ba6a693a6919f27567935098", "hashDigest case 5 error!");

    let u8Arr2 = new Uint8Array([1, 3, 5, 7, 9]);
    let ret6 = crypto.hashDigest({
        data : u8Arr2,
        algo: 'MD5'
    });
    feat_expect_true(ret6 === '75e966753520b561f007b366d58743ee', "hashDigest case 6 error!");

    let ret7 = crypto.hashDigest({
        data : 'text to be digested',
        algo: 'SHA256'
    });
    feat_expect_true(ret7 === "7f7b5c3a78e022b8974575b7e045b4fd3678f8ae7d5fa56b48300fcfa9719e82", "hashDigest case 7 error!");

    let ret8 = crypto.hashDigest({
        uri: 'internal://files/stack.png',
        algo: 'SHA1'
    });
    feat_expect_true(typeof ret8 === 'string', "hashDigest case 8 error!");

    let ret9 = crypto.hashDigest({
        uri: 'internal://files/test.txt',
        algo: 'MD5'
    });
    feat_expect_true(typeof ret9 === 'string', "hashDigest case 9 error!");
});

feat_test("cryptoTest", "hmacDigest", () => {
    crypto.hmacDigest({
        data: 'hello',
        algo: 'SHA512',
        key: 'b8950cfe5af681ba40d3989a030affdd9b89af3c3b8f3ba80b7080889643c99be12da3fe0e27527b1ca98b5fde2ec8daf2991fe53580358eba470a42417f5416',
        success: (res) => {
          print(`hmacDigest success: ${res.data}`)
          hmacData = res.data;
        },
        fail: (data, code) => {
          print(`### HmacDigest fail ### ${code}: ${data}`)
        }
    });
    print("wjf: ", hmacData);
    feat_expect_true(hmacData === "696b069cea69ea52571eb1139fbf394f800aa801972e4bfd6c9f800afa63a1c46b5f66874065e3aec97655a34d031d98e0f0a9d0bf9e84d06fa29c8e74b93656", "hmacDigest case 1 error!");
});

feat_test("cryptoTest", "sign", () => {
    crypto.sign({
        data: 'hello',
        algo: 'RSA-MD5',
        privateKey: PRIVATEKEY,
        success: (res) => {
          print(`sign success: ${res.data}`)
          signData = res.data;
        },
        fail: (data, code) => {
          print(`### sign fail ### ${code}: ${data}`)
        },
        complete: () => {
          console.log('complete excute');
        }
    });
    feat_expect_true(signData === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "sign case 1 error!");

    crypto.sign({
        uri: 'internal://files/test2.txt',
        privateKey: PRIVATEKEY,
        success: (res) => {
          print(`sign success: ${res.data}`)
          fileSignData = res.data;
        },
        fail: (data, code) => {
          print(`### sign fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(fileSignData === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "sign case 2 error!");

    crypto.sign({
        uri: 'internal://files/1.jpg',
        privateKey: PRIVATEKEY,
        success: (res) => {
          print(`sign success: ${res.data}`)
          imgSignData = res.data;
        },
        fail: (data, code) => {
          print(`### sign fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(imgSignData === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "sign case 3 error!");

    let str = 'hello world';
    const u8Arr = new Uint8Array(str.length);
    for (let i = 0, strLen = str.length; i < strLen; i++) {
        u8Arr[i] = str.charCodeAt(i);
    }
    print("u8Arr============")
    console.log(u8Arr)
    crypto.sign({
        data: u8Arr,
        privateKey: PRIVATEKEY,
        success: (res) => {
          print(`sign success: ${res.data}`)
          arrBuffSignData = res.data;
          arrBuffSignDataText = arrBuffSignData.join(',')
        },
        fail: (data, code) => {
          print(`### sign fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(arrBuffSignDataText === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "sign case 4 error!");
});

feat_test("cryptoTest", "verify", () => {
    crypto.verify({
        data: 'hello',
        algo: 'RSA-MD5',
        publicKey: PUBLICKEY,
        signature: signData,
        success: (data) => {
          print(`verify success: ${data}`)
          verifyResult = data;
        },
        fail: (data, code) => {
          print(`### verify fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(verifyResult === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "verify case 1 error!");

    crypto.verify({
        uri: 'internal://files/test2.txt',
        publicKey: PUBLICKEY,
        signature: fileSignData,
        success: (data) => {
          print(`verify success: ${data}`)
          fileVerifyResult = data;
        },
        fail: (data, code) => {
          print(`### verify fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(fileSignData === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "verify case 2 error!");

    crypto.verify({
        uri: 'internal://files/1.jpg',
        publicKey: PUBLICKEY,
        signature: imgSignData,
        success: (data) => {
          print(`verify success: ${data}`)
          imgVerifyResult = data;
        },
        fail: (data, code) => {
          print(`### verify fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(imgVerifyResult === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "verify case 3 error!");

    let str = 'hello world';
    const u8Arr = new Uint8Array(str.length);
    for (let i = 0, strLen = str.length; i < strLen; i++) {
        u8Arr[i] = str.charCodeAt(i);
    }
    print("arrBuffSignData==========")
    print(arrBuffSignData)
    crypto.verify({
        data: u8Arr,
        publicKey: PUBLICKEY,
        signature: arrBuffSignData,
        success: (data) => {
          print(`verify success: ${data}`)
          arrBuffSignResult = data;
        },
        fail: (data, code) => {
          print(`### verify fail ### ${code}: ${data}`)
        },
        complete: () => {
          print('complete excute');
        }
    });
    feat_expect_true(arrBuffSignResult === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "verify case 4 error!");
});

feat_test("cryptoTest", "encrypt", () => {
    let str = 'hello';
    crypto.encrypt({
        //待加密的文本内容
        data: str,
        //base64编码后的加密公钥
        key: PUBLICKEY,
        success: (res) => {
          print(`encrypt success: ${res.data}`);
          strPublicEncryptData = res.data;
        },
        fail: (data, code) => {
          print(`### encrypt fail ### ${code}: ${data}`)
        }
    });
    feat_expect_true(strPublicEncryptData === "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", "encrypt case 1 error!");

    let u8Arr = new Uint8Array(str.length);
    for (let i = 0, strLen = str.length; i < strLen; i++) {
        u8Arr[i] = str.charCodeAt(i);
    }
    crypto.encrypt({
        //待加密的文本--uint8Array类型
        data: u8Arr,
        //base64编码后的加密公钥
        key: PUBLICKEY,
        success: (res) => {
          print(`encrypt success: ${res.data}`);
          arrEncryptData = res.data;
          arrEncryptDataText = arrEncryptData.join(',')
        },
        fail: (data, code) => {
          print(`### encrypt fail ### ${code}: ${data}`)
        }
    });
    feat_expect_true(arrEncryptDataText === 'string', "encrypt case 2 error!");
});
