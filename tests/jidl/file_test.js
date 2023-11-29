
// 无法识别
// import request from '@system.request'

/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

let request = require('system.request');
let file = require('system.file');

feat_async_test("file", "writeText", (done) => {
    return new Promise(function(resolve, reject) {
        file.writeText({
            uri : 'internal://files/demo.txt',
            text : 'write by file.writeText. ',
            success : function() {
                request.print("### writeText success ### ");
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### writeText fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
    feat_expect_true(res, "file writeText success");
    done(); }, (err) => {
    feat_expect_true(err, "file writeText fail");
    done(); });
})

feat_async_test("file", "copy", (done) => {
    return new Promise(function(resolve, reject) {
        file.copy({
            srcUri : "internal://files/demo.txt",
            dstUri : "internal://files/copy_demo.txt",
            success : function(ret) {
                request.print("### copy success ### ", ret);
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### copy fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
        feat_expect_true(res, "file copy success");
        done(); }, (err) => {
        feat_expect_true(err, "file copy fail");
        done(); });
})
feat_async_test("file", "access", (done) => {
    return new Promise(function(resolve, reject) {
        file.access({
            uri : "internal://files/demo.txt",
            success : function() {
                request.print("### access success ### ");
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### access fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
        feat_expect_true(res, "file access success");
        done(); }, (err) => {
        feat_expect_true(err, "file access fail");
        done(); });
})

feat_async_test("file", "move", (done) => {
    return new Promise(function(resolve, reject) {
        file.move({
            srcUri : "internal://files/demo.txt",
            dstUri : "internal://files/move_demo.txt",
            success : function(ret) {
                request.print("### move success ### ", ret);
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### move fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
        feat_expect_true(res, "file move success");
        done(); }, (err) => {
        feat_expect_true(err, "file move fail");
        done(); });
})

feat_async_test("file", "delete", (done) => {
    return new Promise(function(resolve, reject) {
        file.delete({
            uri : "internal://files/move_demo.txt",
            success : function() {
                request.print("### delete success ### ");
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### delete fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
        feat_expect_true(res, "file delete success");
        done(); }, (err) => {
        feat_expect_true(err, "file delete fail");
        done(); });
})

feat_async_test("file", "writeText", (done) => {
    return new Promise(function(resolve, reject) {
        file.writeText({
            uri : 'internal://files/demo.txt',
            text : 'write by file.writeText. ',
            success : function() {
                request.print("### writeText success ### ");
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### writeText fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
    feat_expect_true(res, "file writeText success");
    done(); }, (err) => {
    feat_expect_true(err, "file writeText fail");
    done(); });
})

feat_async_test("file", "writeArrayBuffer", (done) => {
    return new Promise(function(resolve, reject) {
        var str = "write by file.writeArrayBuffer."
        var len = str.length;
        const buffer = new Uint8Array(len);
        for (let i = 0; i < len; i++) {
            buffer[i] = str.charCodeAt(i);
        }
        file.writeArrayBuffer({
            uri : 'internal://files/demo.txt',
            buffer : buffer,
            append : true,
            success : function() {
                request.print("### writeArrayBuffer success ### ");
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### writeArrayBuffer fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
            feat_expect_true(res, "file writeArrayBuffer success");
            done(); }, (err) => {
            feat_expect_true(err, "file writeArrayBuffer fail");
            done(); });
})

feat_async_test("file", "readText", (done) => {
    return new Promise(function(resolve, reject) {
        file.readText({
            uri : 'internal://files/demo.txt',
            success : function(data) {
                request.print("### readText success ### text = ", data.text);
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### readText fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
            feat_expect_true(res, "file readText success");
            done(); }, (err) => {
            feat_expect_true(err, "file readText fail");
            done(); });
})

feat_async_test("file", "readArrayBuffer", (done) => {
    return new Promise(function(resolve, reject) {
        file.readArrayBuffer({
            uri : 'internal://files/demo.txt',
            position : 0,
            length : 10,
            success : function(data) {
                request.print("### readArrayBuffer success ### ");
                request.print('buffer' + JSON.stringify(data));
                request.print('buffer.length: ' + data.buffer.length);
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### readArrayBuffer fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
            feat_expect_true(res, "file readArrayBuffer success");
            done(); }, (err) => {
            feat_expect_true(err, "file readArrayBuffer fail");
            done(); });
})

feat_async_test("file", "mkdir", (done) => {
    return new Promise(function(resolve, reject) {
        file.mkdir({
            uri : 'internal://files/newdir',
            success : function() {
                request.print("### mkdir success ### ");
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### mkdir fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
    feat_expect_true(res, "file mkdir success");
    done(); }, (err) => {
    feat_expect_true(err, "file mkdir fail");
    done(); });
})

feat_async_test("file", "rmdir", (done) => {
    return new Promise(function(resolve, reject) {
        file.rmdir({
            uri : 'internal://files/newdir',
            success : function() {
                request.print("### rmdir success ### ");
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### rmdir fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
    feat_expect_true(res, "file rmdir success");
    done(); }, (err) => {
    feat_expect_true(err, "file rmdir fail");
    done(); });
})

feat_async_test("file", "list", (done) => {
    return new Promise(function(resolve, reject) {
        file.list({
            uri : 'internal://files/',
            success : function(data) {
                request.print("### list success ### ");
                request.print(JSON.stringify(data.fileList))
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### list fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
    feat_expect_true(res, "file list success");
    done(); }, (err) => {
    feat_expect_true(err, "file list fail");
    done(); });
})

feat_async_test("file", "get", (done) => {
    return new Promise(function(resolve, reject) {
        file.get({
            uri : 'internal://files/',
            recursive : true,
            success : function(data) {
                request.print("### get success ### ");
                request.print(JSON.stringify(data))
                resolve(true);
            },
            fail : function(errmsg, errcode) {
                var fileCopyData = errcode + '---' + errmsg
                request.print("### get fail ### ", fileCopyData);
                reject(false);
            }
        })
    }).then((res) => {
    feat_expect_true(res, "file get success");
    done(); }, (err) => {
    feat_expect_true(err, "file get fail");
    done(); });
})