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

let exchange =require('service.exchange');

feat_async_test("exchange","set_application",(done)=>{
  exchange.set({
    key: 'A1',
    value: 'V1',
    scope:'application',
    package:'com.xiaomi.application',
    sign:'7a12ec1d66233f20a20141035b1f7937',
    success: function(data) {
      feat_expect_true(true, "exchange set_application success");
      done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange set_application fail");
      done();
    }
  })
})

feat_async_test("exchange","get_application",(done)=>{
  exchange.get({
    key: 'A1',
    scope:'application',
    package:'com.xiaomi.application',
    sign:'7a12ec1d66233f20a20141035b1f7937',
    success: function(data) {
       var res = false;
       if(data == 'V1') { 
        res = true;
       }
       feat_expect_true(res, "exchange get_application success");
       done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange get_application fail");
      done();
    }
  })
})

feat_async_test("exchange","remove",(done)=>{
  exchange.remove({
    key: 'A1',
    package:'com.xiaomi.application',
    success: function(data) {
      feat_expect_true(true, "exchange remove success");
      done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange remove fail");
      done();
    }
  })
})

feat_async_test("exchange","set_global",(done)=>{
  exchange.set({
    key: 'A2',
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
    key: 'A2',
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

feat_test("exchange","setParamError1",(done)=>{
  exchange.set({
    key: 'A1',
    scope:'application',
    sign:'7a12ec1d66233f20a20141035b1f7937',
    success: function(data) {
       feat_expect_true(false, "exchange setParamError1 expect false,fail");
       done();
    },
    fail: function(data, code) {
      feat_expect_true(true, "exchange setParamError1 expect false,success");
      done();
    }
  })
})

feat_test("exchange","setParamError2",(done)=>{
  exchange.set({
    key: 'A1',
    scope:'global',
    package:'com.xiaomi.application',
    sign:'7a12ec1d66233f20a20141035b1f7937',
    success: function(data) {
       feat_expect_true(false, "exchange setParamError2 expect false,fail");
       done();
    },
    fail: function(data, code) {
      feat_expect_true(true, "exchange setParamError2 expect false,success");
      done();
    }
  })
})

feat_test("exchange","setParamError3",(done)=>{
  exchange.get({
    key: 'A1',
    scope:'global',
    package:'com.xiaomi.application',
    sign:'7a12ec1d66233f20a20141035b1f7937',
    success: function(data) {
       feat_expect_true(false, "exchange setParamError3 expect false,fail");
       done();
    },
    fail: function(data, code) {
      feat_expect_true(true, "exchange setParamError3 expect false,success");
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

feat_async_test("exchange","grantPermission",(done)=>{
  exchange.grantPermission({
    package:'com.xiaomi.application',
    key:'V3',
    sign:"7a12ec1d66233f20a20141035b1f7937",
    success: function(data) {
      var res =false; 
      if(data == '4') {
        res=true;
      }
      feat_expect_true(res, "exchange grantPermission success");
      done();
    },
    fail: function(data, code) {
      feat_expect_true(err, "exchange grantPermission fail");
      done();
    }
  })  
})

feat_async_test("exchange","revokePermission",(done)=>{
  exchange.revokePermission({
    package:'com.xiaomi.application',
    key:'V3',
    success: function(data) {
      feat_expect_true(true, "exchange revokePermission success");
      done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "exchange revokePermission fail");
      done();
    }
  })
})