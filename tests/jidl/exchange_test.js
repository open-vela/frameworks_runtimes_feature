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

let exchange =require('system.exchange');


feat_async_test("exchange","set_global",(done)=>{
  exchange.set({
    key: 'A1',
    scope:'global',
    value:'V2',
    success: function(data) {
      feat_expect_true(true, "exchange set_global success");
      done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange set_global fail");
      done();
    }
  })
})

feat_async_test("exchange","get_global",(done)=>{
  exchange.get({
    key: 'A1',
    scope:'global',
    success: function(data) {
       var res =false;
       if(data == 'V2') {
        res=true;
       }
       feat_expect_true(res, "exchange get_global success");
        done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange get_global fail");
      done();
    }
  })
})

feat_async_test("exchange","remove",(done)=>{
  exchange.remove({
    key: 'A2',
    scope:'global',
    success: function(data) {
       feat_expect_true(true, "exchange get_global success");
        done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange get_global fail");
      done();
    }
  })
})

feat_test("exchange","clear",()=>{
  exchange.clear({
    success: function(data) {
      feat_expect_true(true, "exchange clear success");
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange clear fail");
    }
  })
})

feat_test("exchange","set_application",()=>{
  exchange.set({
    key: 'A1',
    scope:'application',
    value:'V2',
    success: function(data) {
      feat_expect_true(false, "exchange set_application success");
    },
    fail: function(data, code) {
      feat_expect_true(true, "exchange set_application fail");
    }
  })
})