let test = require('arraybuffer_test');

let numbers = new Uint8Array([2, 5, 8, 1, 4]);
test.print("uint8Array: ", numbers.toString());
test.setArraybuffer(1, numbers.buffer);

let buff = test.getArraybuffer();
var uint8_buff = new Uint8Array(buff);
test.print("uint8_buff: ", uint8_buff.toString());

test.setArraybuffer(2, numbers);

let i8_buff = test.getTypedArraybuffer(0);
test.print("int8_buff: ", i8_buff.toString());
let u8_buff = test.getTypedArraybuffer(1);
test.print("uint8_buff: ", u8_buff.toString());
let i16_buff = test.getTypedArraybuffer(2);
test.print("int16_buff: ", i16_buff.toString());
let u16_buff = test.getTypedArraybuffer(3);
test.print("uint16_buff: ", u16_buff.toString());


