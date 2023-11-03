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

let exchange =require('exchange');

feat_async_test("exchange","set_application",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.set({
            key: 'A1',
            value: 'V1',
            scope:'application',
            package:'com.xiaomi.application',
            sign:'7a12ec1d66233f20a20141035b1f7937',
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange set_application success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange set_application fail");
        done();
      });
})

feat_async_test("exchange","get_application",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.get({
            key: 'A1',
            scope:'application',
            package:'com.xiaomi.application',
            sign:'7a12ec1d66233f20a20141035b1f7937',
            success: function(data) {
               print(data);
               if(data == 'V1') {resolve(true);}
               else {reject(false);}
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange get_application success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange get_application fail");
        done();
      });
})

feat_async_test("exchange","remove",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.remove({
            key: 'A1',
            package:'com.xiaomi.application',
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange remove success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange remove fail");
        done();
      });
})

feat_async_test("exchange","set_global",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.set({
            key: 'A2',
            scope:'global',
            value:'V2',
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange set_global success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange set_global fail");
        done();
      });
})

feat_async_test("exchange","get_global",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.get({
            key: 'A2',
            scope:'global',
            success: function(data) {
               if(data == 'V2') {resolve(true);}
               else {reject(false);}
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange set_global success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange set_global fail");
        done();
      });
})

feat_async_test("exchange","clear",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.clear({
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange clear success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange clear fail");
        done();
      });
})

feat_async_test("exchange","grantPermission",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.grantPermission({
            package:'com.xiaomi.application',
            key:'V3',
            sign:"7a12ec1d66233f20a20141035b1f7937",
            success: function(data) {
               if(data == '4') {resolve(true)}
               reject(false);
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange grantPermission success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange grantPermission fail");
        done();
      });
})

feat_async_test("exchange","revokePermission",(done)=>{
    return new Promise(function (resolve, reject) {
        exchange.revokePermission({
            package:'com.xiaomi.application',
            key:'V3',
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
          })
    }).then((res)=>{
        feat_expect_true(res, "exchange revokePermission success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "exchange revokePermission fail");
        done();
      });
})

feat_test_all();