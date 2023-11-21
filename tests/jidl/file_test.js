
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

let request = require('request');
let file = require('file');

var token;
feat_async_test("request", "download", (done) => {
    return new Promise(function(resolve, reject) {
        request.download({
            url : 'https://www.quickapp.cn/assets/images/home/logo_quickApp.png',
            filename : 'quickAppLogo.png', // 指定文件名
            success : function(ret) {
                token = ret.token
                request.print('### request.download.success ###')
                request.print('token = ', ret.token)
                resolve(true);
            },
            fail : function(data, code) {
                request.print('### handling fail ###')
                request.print('code = ', code, 'data = ', data)
                reject(false);
            }
        })
    }).then((res) => {
        feat_expect_true(res, "request download success");
        done(); }, (err) => {
        feat_expect_true(err, "request download fail");
        done(); });
})
var uri
feat_async_test("request", "onDownloadComplete", (done) => {
    return new Promise(function(resolve, reject) {
        request.onDownloadComplete({
            token : token,
            success : function(data) {
                request.print("### request onDownloadComplete success ###");
                request.print('data = ', data)
                uri = data;
                resolve(true);
            },
            fail : function(data, code) {
                request.print("### request onDownloadComplete fail ###");
                request.print('code = ', code, 'data = ', data)
                reject(false);
            }
        })
    }).then((res) => {
        feat_expect_true(res, "request onDownloadComplete success");
        done(); }, (err) => {
        feat_expect_true(err, "request onDownloadComplete fail");
        done(); });
})

feat_async_test("file", "copy", (done) => {
    return new Promise(function(resolve, reject) {
        file.copy({
            srcUri: "internal://files/quickAppLogo.png",
            dstUri: "internal://files/copy_quickAppLogo.png",
            success: function (ret) {
                request.print("### copy success ### ", ret);
                resolve(true);
            },
            fail: function (errmsg, errcode) {
              var fileCopyData = errcode + '---' + errmsg
              request.print("### copy fail ### ", fileCopyData);
              reject(false);
            }
          })
    }).then((res) => {
        feat_expect_true(res, "request download success");
        done(); }, (err) => {
        feat_expect_true(err, "request download fail");
        done(); });
})
feat_async_test("file", "access", (done) => {
    return new Promise(function(resolve, reject) {
        file.access({
            uri: "internal://files/quickAppLogo.png",
            success: function () {
                request.print("### access success ### ");
                resolve(true);
            },
            fail: function (errmsg, errcode) {
              var fileCopyData = errcode + '---' + errmsg
              request.print("### access fail ### ", fileCopyData);
              reject(false);
            }
          })
    }).then((res) => {
        feat_expect_true(res, "request download success");
        done(); }, (err) => {
        feat_expect_true(err, "request download fail");
        done(); });
})

feat_async_test("file", "move", (done) => {
    return new Promise(function(resolve, reject) {
        file.move({
            srcUri: "internal://files/quickAppLogo.png",
            dstUri: "internal://files/move_quickAppLogo.png",
            success: function (ret) {
                request.print("### move success ### ", ret);
                resolve(true);
            },
            fail: function (errmsg, errcode) {
              var fileCopyData = errcode + '---' + errmsg
              request.print("### move fail ### ", fileCopyData);
              reject(false);
            }
          })
    }).then((res) => {
        feat_expect_true(res, "request download success");
        done(); }, (err) => {
        feat_expect_true(err, "request download fail");
        done(); });
})

feat_async_test("file", "delete", (done) => {
    return new Promise(function(resolve, reject) {
        file.delete({
            uri: "internal://files/move_quickAppLogo.png",
            success: function () {
                request.print("### delete success ### ");
                resolve(true);
            },
            fail: function (errmsg, errcode) {
              var fileCopyData = errcode + '---' + errmsg
              request.print("### delete fail ### ", fileCopyData);
              reject(false);
            }
          })
    }).then((res) => {
        feat_expect_true(res, "request download success");
        done(); }, (err) => {
        feat_expect_true(err, "request download fail");
        done(); });
})

