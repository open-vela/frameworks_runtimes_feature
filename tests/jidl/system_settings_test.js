/**
 *
 * feature_test_cli -m 10 /quickapp/system_settings_test.js &
 */
settings = require("system.settings");

console.log("system.settings test begin");
{
  let test_name = "set prop 1";
  settings.setProp({
    key: "car.cockpitMode",
    value: 1,
    success: function (data) {
      console.log(`${test_name} success`);
    },
    fail: function (data, code) {
      console.log(`${test_name} fail ${data} ${code}`);
    },
    complete: function () {
      console.log(`${test_name} complete`);
    },
  });
}

{
  let test_name = "set get prop in timer 5s";
  setTimeout(() => {
    settings.setProp({
      key: "car.cockpitMode",
      value: 2,
      success: function () {
        console.log(`${test_name} success`);
      },
      fail: function (data, code) {
        console.log(`${test_name} fail ${data} ${code}`);
      },
      complete: function () {
        console.log(`${test_name} complete`);
      },
    });
  }, 5000);
}

{
  let test_name = "get prop single";
  settings.getProp({
    key: "car.cockpitMode",
    success: function (data) {
      console.log(`${test_name} success: `, JSON.stringify(data), typeof data);
    },
    fail: function (data, code) {
      console.log(`${test_name} fail ${data} ${code}`);
    },
    complete: function () {
      console.log(`${test_name} complete`);
    },
  });
}

{
  let test_name = "get all prop";
  settings.getProp({
    key: "car",
    success: function (data) {
      console.log(`${test_name} success: `, JSON.stringify(data), typeof data);
    },
    fail: function (data, code) {
      console.log(`${test_name} fail ${data} ${code}`);
    },
    complete: function () {
      console.log(`${test_name} complete`);
    },
  });
}

{
  let test_name = "subscribe car.cockpitMode";
  settings.subscribeProp({
    key: "car.cockpitMode",
    callback: function (data) {
      console.log(`${test_name} change`);
      console.log("car.cockpitMode change", JSON.stringify(data));
    },
    fail: function (data, code) {
      console.log(`${test_name} fail ${data} ${code}`);
    },
  });
}

// setTimeout(() => {
//   settings.unsubscribeProp("car.cockpitMode");
//   settings.setProp({
//     key: "car.cockpitMode",
//     value: 1,
//     success: function (data) {
//       console.log("set car.cockpitMode 1 success in timer", data);
//     },
//     fail: function (data, code) {
//       console.log(`set prop car.cockpitMode 1  fail in timer ${data} ${code}`);
//     },
//     complete: function () {
//       console.log("complete set car.cockpitMode 1 in timer");
//     },
//   });
// }, 8000);
