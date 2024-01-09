/*
 * Copyright (C) 2024 Xiaomi Corporation
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

let power = require('system.internal.power')

feat_test("power","shutDown",()=>{
    power.shutDown({
        success: function(data) {
            feat_expect_true(true, "power shutDown success");
        },
        fail: function(data, code) {
            feat_expect_true(false, "power shutDown fail");
        }
    })
})

feat_test("power","reboot",()=>{
    power.reboot({
        success: function(data) {
            feat_expect_true(true, "power reboot success");
        },
        fail: function(data, code) {
            feat_expect_true(false, "power reboot fail");
        }
    })
})