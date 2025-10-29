let test_1 = require('arraybuffer_test');
let test_2 = require('arraybuffer_test');

console.log('arraybuffer test begin ====================')
let u8_array_1 = new Uint8Array([1, 2, 3, 4, 5]);
console.log('test_1.set_buffer(u8_array_1) 1');
test_1.set_buffer(u8_array_1.buffer);
console.log('test_1.set_buffer(u8_array_1) 2');
test_1.set_buffer(u8_array_1.buffer);
console.log('test_1.set_buffer(u8_array_1) 3');
test_1.set_buffer(u8_array_1.buffer);

let buff_copy = test_1.get_buffer_copy();
const buff_copy_view = new Uint8Array(buff_copy);
console.log("test_1.get_buffer_copy(), view: ", buff_copy_view.toString());

console.log('test_2.set_buffer(buff_copy)');
test_2.set_buffer(buff_copy);

let buff_no_copy = test_1.get_buffer_no_copy();
const buff_no_copy_view = new Uint8Array(buff_no_copy);
console.log("test_1.get_buffer_no_copy(), view: ", buff_no_copy_view.toString());

console.log('test_2.set_buffer(buff_no_copy)');
test_2.set_buffer(buff_no_copy);

let buff_0 = new Uint8Array([1, 2, 3, 4, 5]);
let buff_1 = new Uint8Array([5, 4, 3, 2, 1]);
let buff_2 = new Uint8Array([2, 1, 5, 4, 3]);
let buff_array = [ buff_0.buffer, buff_1.buffer, buff_2.buffer ]
console.log('test_1.set_buffer_array(buff_array)');
test_1.set_buffer_array(buff_array);

console.log("test_1.get_buffer_array_copy()");
let buff_array_copy = test_1.get_buffer_array_copy();
console.log('test_2.set_buffer_array(buff_array_copy)');
test_2.set_buffer_array(buff_array_copy);

console.log("test_1.get_buffer_array_no_copy()");
let buff_array_no_copy = test_1.get_buffer_array_no_copy();
console.log('test_2.set_buffer_array(buff_array_no_copy)');
test_2.set_buffer_array(buff_array_no_copy);

console.log('arraybuffer test end ====================')
