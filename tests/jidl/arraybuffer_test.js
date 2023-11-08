let test = require('arraybuffer_test');

let numbers = new Uint8Array([2, 5, 8, 1, 4]);
test.print("uint8Array: ", numbers.toString());
test.setArraybuffer(5, numbers.buffer);

let buff = test.getArraybuffer();
var uint8_buff = new Uint8Array(buff);
test.print("uint8_buff: ", uint8_buff.toString());


