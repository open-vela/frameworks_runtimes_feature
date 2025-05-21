const mqttmessage = require("system.mqttmessage");

// root = proto.parse(
// 'syntax = "proto3";\
// message MqttMessage {\
//   int64 ts = 1;\
//   string id = 2;\
//   string carId = 3;\
//   string packageName = 4;\
//   string action = 5;\
//   bytes data = 6;\
// }')

console.log("=== verify test ===");
let error_data1 = {
  ts: Date.now(),
  id: "123",
  carId: "123",
  packageName: "com.aiot.test",
  action: 5, // wrong
  data: new Uint8Array([1, 2, 3]),
};

if (mqttmessage.verify(error_data1) === false) {
  console.log("verify test pass1");
} else {
  console.log("verify test fail1:", error_data1);
}

function compare_mqttmessage(a, b) {
  let judge = {
    ts: () => a.ts === b.ts,
    id: () => a.id === b.id,
    carId: () => a.carId === b.carId,
    packageName: () => a.packageName === b.packageName,
    action: () => a.action === b.action,
    "data.len": () => a.data.length === b.data.length,
    "data.data": () => a.data.every((v, i) => v === b.data[i]),
  };
  for (let key in judge) {
    if (!judge[key]()) {
      console.log("compare fail:", key);
      console.log("expect:", a[key], "actual:", b[key]);
      return false;
    }
  }
  return true;
}

console.log(
  "================ encode and decode consistent test =================="
);
let origin_data = {
  ts: Date.now(),
  id: "123",
  carId: "123",
  packageName: "com.aiot.test",
  action: "test",
  data: new Uint8Array([1, 2, 3]),
};

let bindata = mqttmessage.encode(origin_data);

let obj = mqttmessage.decode(bindata);

if (compare_mqttmessage(origin_data, obj)) {
  console.log("encode and decode consistent pass");
} else {
  console.log("encode and decode consistent fail:", origin_data, obj);
}

console.log("================ decode test ==================");

const bytes = new Uint8Array([
  8, -66, -17, -51, -24, -49, 50, 18, 19, 51, 55, 51, 48, 55, 53, 54, 55, 52,
  51, 57, 55, 52, 54, 49, 55, 48, 56, 57, 34, 21, 99, 111, 109, 46, 109, 105,
  46, 99, 97, 114, 46, 100, 105, 103, 105, 116, 97, 108, 75, 101, 121, 42, 10,
  109, 101, 115, 115, 97, 103, 101, 65, 99, 107, 50, 21, 10, 19, 51, 55, 51, 48,
  55, 53, 54, 55, 52, 51, 57, 55, 52, 54, 49, 55, 48, 56, 57,
]);

let decode_msg = mqttmessage.decode(bytes);
origin_data = {
  ts: 1739412699070,
  carId: "",
  id: "3730756743974617089",
  packageName: "com.mi.car.digitalKey",
  action: "messageAck",
  data: new Uint8Array([
    10, 19, 51, 55, 51, 48, 55, 53, 54, 55, 52, 51, 57, 55, 52, 54, 49, 55, 48,
    56, 57,
  ]),
};

if (compare_mqttmessage(origin_data, decode_msg)) {
  console.log("decode pass");
} else {
  console.log("decode fail:", JSON.stringify(decode_msg));
}

console.log("============== encode data ===============");
// 解码测试3
let test_data = {
  encode: [
    {
      text: "符合mqttMessage格式",
      value: {
        ts: 1747886439565,
        id: "3730756743974617089",
        carId: "111",
        packageName: "com.mi.car.digitalKey",
        action: "ack",
        data: new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 0, 10]),
      },
      expect: new Uint8Array([
        8, 141, 249, 153, 177, 239, 50, 18, 19, 51, 55, 51, 48, 55, 53, 54, 55,
        52, 51, 57, 55, 52, 54, 49, 55, 48, 56, 57, 26, 3, 49, 49, 49, 34, 21,
        99, 111, 109, 46, 109, 105, 46, 99, 97, 114, 46, 100, 105, 103, 105,
        116, 97, 108, 75, 101, 121, 42, 3, 97, 99, 107, 50, 7, 1, 2, 3, 4, 5, 6,
        7,
      ]),
    },
    {
      text: "mqttMessage格式缺少data",
      value: {
        ts: Date.now(),
        id: "3730756743974617089",
        carId: "111",
        packageName: "com.mi.car.digitalKey",
        action: "abc",
      },
      expect: undefined,
    },
    {
      text: "mqttMessage格式ts为string",
      value: {
        ts: "abc",
        id: "3730756743974617089",
        carId: "111",
        packageName: "com.mi.car.digitalKey",
        action: "abc",
        data: new Uint8Array([]),
      },
      expect: undefined,
    },
    {
      text: "mqttMessage格式data为字符串",
      value: {
        ts: Date.now(),
        id: "3730756743974617089",
        carId: "111",
        packageName: "com.mi.car.digitalKey",
        action: "abc",
        data: "abc",
      },
      expect: undefined,
    },
    {
      text: "object格式",
      value: { a: 1, b: 2 },
      expect: undefined,
    },
    {
      text: "空对象格式",
      value: {},
      expect: undefined,
    },
    {
      text: "Uint8Array格式",
      value: new Uint8Array([1, 2, 3]),
      expect: undefined,
    },
    {
      text: "string格式",
      value: "abc",
      expect: undefined,
    },
    {
      text: "boolean格式",
      value: true,
      expect: undefined,
    },
    {
      text: "number格式",
      value: 123,
      expect: undefined,
    },

    {
      text: "null格式",
      value: null,
      expect: undefined,
    },
  ],
};

for (let data of test_data.encode) {
  let ret = mqttmessage.encode(data.value);
  console.log("origin", JSON.stringify(data.value));
  let test = (a, b) => {
    if (Object.prototype.toString.call(a).match(/\[object Uint8Array\]/)) {
      return a.every((v, i) => v === b[i]);
    } else {
      return a === b;
    }
  };
  if (test(ret, data.expect)) {
    console.log(`${data.text} pass`);
  } else {
    console.log(`${data.text} fail`);
    console.log("expect", data.expect);
    console.log("encode", ret);
  }
}

console.log("================ pressure test ==================");
let decode_data = new Uint8Array([
  8, 220, 212, 205, 181, 223, 50, 18, 19, 51, 55, 52, 52, 51, 53, 51, 49, 52,
  49, 57, 53, 55, 53, 57, 49, 55, 53, 49, 26, 17, 76, 78, 66, 81, 76, 84, 49,
  85, 66, 76, 55, 49, 87, 72, 57, 84, 49, 34, 17, 99, 111, 109, 46, 109, 105,
  46, 99, 97, 114, 46, 109, 111, 98, 105, 108, 101, 42, 18, 80, 82, 79, 80, 69,
  82, 84, 73, 69, 83, 95, 67, 72, 65, 78, 71, 69, 68, 50, 158, 2, 120, 156, 165,
  148, 77, 110, 132, 48, 12, 133, 239, 226, 53, 138, 98, 59, 63, 78, 174, 82,
  117, 49, 234, 176, 64, 234, 104, 208, 12, 237, 102, 52, 119, 47, 116, 67, 16,
  132, 40, 176, 134, 239, 249, 217, 126, 241, 11, 250, 199, 189, 111, 31, 67,
  215, 62, 33, 126, 188, 160, 235, 174, 16, 1, 89, 33, 42, 11, 13, 252, 94, 190,
  127, 90, 136, 232, 13, 219, 128, 66, 66, 24, 26, 24, 186, 91, 251, 28, 46,
  183, 126, 245, 229, 235, 126, 29, 127, 39, 173, 223, 205, 82, 204, 204, 98,
  122, 37, 224, 180, 118, 78, 130, 248, 77, 1, 163, 140, 66, 159, 152, 217, 226,
  45, 115, 16, 151, 231, 101, 230, 153, 165, 210, 1, 122, 133, 42, 113, 0, 164,
  163, 214, 176, 169, 98, 2, 25, 155, 87, 145, 210, 28, 10, 188, 61, 201, 187,
  164, 11, 148, 104, 108, 109, 23, 211, 52, 121, 22, 33, 187, 206, 67, 105, 157,
  164, 232, 100, 26, 102, 220, 56, 85, 175, 64, 105, 7, 245, 56, 167, 254, 125,
  37, 78, 74, 138, 213, 45, 155, 92, 152, 253, 210, 60, 213, 183, 63, 165, 0,
  11, 175, 105, 23, 167, 20, 39, 87, 221, 191, 95, 240, 7, 198, 87, 194, 173,
  247, 65, 178, 199, 136, 139, 233, 219, 185, 37, 255, 124, 82, 95, 111, 143,
  127, 71, 97, 156, 94, 186, 192, 204, 49, 203, 238, 127, 52, 224, 22, 6, 106,
  79, 217, 196, 211, 241, 245, 79, 233, 119, 199, 203, 219, 101, 248, 234, 113,
  58, 135, 243, 57, 220, 28, 192, 63, 223, 127, 217, 108, 254, 193,
]);
for (let i = 0; i < 10000; i++) {
  obj = mqttmessage.decode(decode_data);
  decode_data = mqttmessage.encode(obj);
  mqttmessage.verify(obj);
}
