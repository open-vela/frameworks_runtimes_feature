let test = require('any_test');

test.setAny(1, "sring any");
test.setAny(2, 6);

let any_obj = {
    'any_1' : 10,
    'any_2': 'hello world',
    'any_3': true
}
test.setAny(3, any_obj);

let numbers = new Uint8Array([1, 3, 5, 7, 9]);
//test.print("uint8Array: ", numbers.toString());
test.setAny(4, numbers);

let any_ret = test.getAny();
test.print("any_ret: ", any_ret);

test.print('\n')
test.print('null param test begin ====================');
test.setAny(3, null);
test.print('null param test end =====================\n');



