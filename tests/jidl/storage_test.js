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

let storage = require('system.storage')

feat_test("storage","clear",()=>{
    storage.clear({
        success: function(data) {
            feat_expect_true(true, "storage clear success");
        },
        fail: function(data, code) {
            feat_expect_true(false, "storage clear fail");
        }
    })
})

feat_test("storage","length",()=>{
    feat_expect_true(storage.length == -1,"length expect -1")
})


feat_async_test("storage","setA1",(done)=>{
    storage.set({
        key: 'A1',
        value: 'V1',
        success: function(data) {
            feat_expect_true(true, "storage setA1 success");
            done();
        },
        fail: function(data, code) {
            feat_expect_true(false, "storage setA1 fail");
            done();
        }
    });
})

feat_async_test("storage","setA2",(done)=>{
    storage.set({
        key: 'A2',
        value: 'V2',
        success: function(data) {
            feat_expect_true(true, "storage setA2 success");
            done();
        },
        fail: function(data, code) {
            feat_expect_true(false, "storage setA2 fail");
            done();
        }
    })
})

feat_async_test("storage","get",(done)=>{
    storage.get({
        key: 'A1',
        success: function(data) {
            var res = false;
           if(data == 'V1') {
               res = true;
           }
           feat_expect_true(res, "storage getA1 success");
           done();
        },
        fail: function(data, code) {
            feat_expect_true(false, "storage getA1 fail");
            done();
        }
    })
})

feat_test("storage","length1",()=>{
    feat_expect_true(storage.length == 1,"length expect 1")
})

feat_async_test("storage","key",(done)=>{
    storage.key({
        index: 1,
        success: function(data) {
            var res = false;
            if(data == 'A2') {
                res = true;
            }
            feat_expect_true(res, "storage key success");
            done();
        },
        fail: function(data, code) {
            feat_expect_true(false, "storage key fail");
            done();
        }
    })
})

feat_async_test("storage","delete",(done)=>{
    storage.delete({
        key:'A1',
        success: function(data) {
           var res = false;
           if(data == 'A1') {
             res = true;
           }
           feat_expect_true(res, "storage key success");
           done();
        },
        fail: function(data, code) {
            feat_expect_true(false, "storage key fail");
            done();
        }
    })
})

feat_test("storage","length2",()=>{
    feat_expect_true(storage.length == 0,"length expect 0")
})