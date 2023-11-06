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

let storage = require('storage')

feat_async_test("sensor","clear",(done)=>{
    return new Promise(function (resolve, reject) {
        storage.clear({
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
        })
    }).then((res)=>{
        feat_expect_true(res, "storage clear success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "storage clear fail");
        done();
      });
})

feat_test("storage","length",()=>{
    feat_expect_true(storage.length == -1,"length expect -1")
})

feat_async_test("storage","setA1",(done)=>{
    return new Promise(function (resolve, reject) {
        storage.set({
            key: 'A1',
            value: 'V1',
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
        })
    }).then((res)=>{
        feat_expect_true(res, "storage setA1 success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "storage setA1 fail");
        done();
      });
})

feat_async_test("storage","setA2",(done)=>{
    return new Promise(function (resolve, reject) {
        storage.set({
            key: 'A2',
            value: 'V2',
            success: function(data) {
               resolve(true);
            },
            fail: function(data, code) {
                reject(false);
            }
        })
    }).then((res)=>{
        feat_expect_true(res, "storage setA2 success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "storage setA2 fail");
        done();
      });
})

feat_async_test("storage","get",(done)=>{
    return new Promise(function (resolve, reject) {
        storage.get({
            key: 'A1',
            success: function(data) {
               if(data == 'V1') {resolve(true);}
               else reject(false);
            },
            fail: function(data, code) {
                reject(false);
            }
        })
    }).then((res)=>{
        feat_expect_true(res, "storage get success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "storage get fail");
        done();
      });
})

feat_test("storage","length1",()=>{
    feat_expect_true(storage.length == 1,"length expect 1")
})

feat_async_test("storage","key",(done)=>{
    return new Promise(function (resolve, reject) {
        storage.key({
            index: 1,
            success: function(data) {
               if(data == 'A2') {resolve(true);}
               else reject(false);
            },
            fail: function(data, code) {
                reject(false);
            }
        })
    }).then((res)=>{
        feat_expect_true(res, "storage key success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "storage key fail");
        done();
      });
})

feat_async_test("storage","delete",(done)=>{
    return new Promise(function (resolve, reject) {
        storage.delete({
            key:'A1',
            success: function(data) {
               if(data == 'A1') {resolve(true);}
               else reject(false);
            },
            fail: function(data, code) {
                reject(false);
            }
        })
    }).then((res)=>{
        feat_expect_true(res, "storage delete success");
        done();
      },
      (err)=>{
        feat_expect_true(err, "storage delete fail");
        done();
      });
})

feat_test("storage","length2",()=>{
    feat_expect_true(storage.length == 0,"length expect 0")
})

feat_test_all();