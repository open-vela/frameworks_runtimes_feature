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

let sensor = require('sensor');

feat_async_test("sensor","Accelerometer",(done)=>{
  sensor.subscribeAccelerometer({
    callback: function(ret) {
      feat_expect_true(true, "sensor subscribeAccelerometer success");
      sensor.unsubscribeAccelerometer();
      done();
    }
  })
})

feat_async_test("sensor","AccelerometerAddInterval",(done)=>{
  sensor.subscribeAccelerometer({
    interval:'game',
    callback: function(ret) {
      feat_expect_true(true, "sensor AccelerometerAddInterval success");
      sensor.unsubscribeAccelerometer();
      done();
    }
  })
})


feat_async_test("sensor","Compass",(done)=>{
  sensor.subscribeCompass({
    callback: function(ret) {
      feat_expect_true(true, "sensor Compass success");
      sensor.unsubscribeCompass();
      done();
    }
  })
})

feat_async_test("sensor","Proximity",(done)=>{
  sensor.subscribeProximity({
    callback: function(ret) {
      feat_expect_true(true, "sensor Proximity success");
      sensor.unsubscribeProximity();
      done();
    }
  })
})

feat_async_test("sensor","Proximity_reserved",(done)=>{
  sensor.subscribeProximity({
    reserved:true,
    callback: function(ret) {
      feat_expect_true(true, "sensor Proximity_reserved success");
      sensor.unsubscribeProximity();
      done();
    }
  })
})

feat_async_test("sensor","Light",(done)=>{
  sensor.subscribeLight({
    callback: function(ret) {
      feat_expect_true(true, "sensor Proximity success");
      sensor.unsubscribeLight();
      done();
    }
  })
})

feat_async_test("sensor","Step",(done)=>{
  sensor.subscribeStepCounter({
    callback: function(ret) {
      feat_expect_true(true, "sensor Step success");
      sensor.unsubscribeStepCounter();
      done();
    },
    fail: function(data, code) {
      feat_expect_true(false, "sensor Step fail");
      done();
    }
  })
})