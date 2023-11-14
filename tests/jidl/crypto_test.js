let test = require('system.crypto');

// has data no uri
let ret1 = test.hashDigest({
        data : 'text to be digested',
        algo: 'SHA256'
    });

// has uri no data
let ret2 = test.hashDigest({
        uri: 'http://www.hello.com',
        algo: 'SHA1'
    });

// has Uint8Array data no uri
let uint8_buff = new Uint8Array([1, 3, 5, 7, 9]);
let ret3 = test.hashDigest({
        data : uint8_buff,
        algo: 'MD5'
    });

// has empty uri
let ret4 = test.hashDigest({
        uri: '',
        algo: 'SHA512'
    });

// missing data and uri
let ret5 = test.hashDigest({
        algo: 'MD5'
    });

// missing all option
let ret6 = test.hashDigest({});

