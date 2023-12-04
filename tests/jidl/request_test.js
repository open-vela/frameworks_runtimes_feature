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

var token;

feat_async_test("requestTest", "download", (done) => {
    return new Promise(function(resolve, reject) {
        request.download({
            url : 'https://statres.quickapp.cn/quickapp/quickapp/201806/file/quickapp_sample_v1000.rpk',
            header : {
                test : 'abc',
                test2 : 'ddd'
            },
            // share: false,
            filename : 'sample.rpk', // 指定文件名
            onDownLoadNotify : function(data) {
                request.print('### onDownLoadNotify ### ')
                request.print('result = ', data.result, 'percent = ', data.percent)
            },
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

feat_async_test("requestTest", "onDownloadComplete", (done) => {
    return new Promise(function(resolve, reject) {
        request.onDownloadComplete({
            token : token,
            success : function(data) {
                request.print("### request onDownloadComplete success ###");
                request.print('data = ', data)
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

feat_async_test("requestTest", "download", (done) => {
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

feat_async_test("requestTest", "download", (done) => {
    return new Promise(function(resolve, reject) {
        request.download({
            url : 'https://www.quickapp.cn/assets/images/home/logo_quickApp.png',
            // filename : 'quickAppLogo.png', // 指定文件名
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
