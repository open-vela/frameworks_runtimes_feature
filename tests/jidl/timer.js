let test_obj = require('Timer');

test_obj.printPoint({'x': 1.0, 'y': 2.0, 'z': 3.0});
test_obj.print('before set, myPoint =', test_obj.myPoint);
test_obj.myPoint = {'x': 500, 'y': 600, 'z': 700};
test_obj.print('after set, myPoint =', test_obj.myPoint);
test_obj.print("myConstant is: ", test_obj.myConstant);
test_obj.print("myConstant1 is: ", test_obj.myConstant1);
test_obj.print("myConstant2 is: ", test_obj.myConstant2);
let result = test_obj.printString('hello world');
test_obj.print("result is: ", result);
test_obj.func_with_cb(function(str, f1, f2, f3){
    test_obj.print("str is: ", str);
    return {'x': f1, 'y': f2, 'z': f3};
});
test_obj.func_with_cb2(function(str, f1, f2, f3){
    test_obj.print("func_with_cb2 str is: ", str);
    test_obj.print("rest arguments is:", arguments);
    return {'x': f1, 'y': f2, 'z': f3};
});
test_obj.print('hello', 'world', 1, 2, 3, 'this is a test');
test_obj.withOptional('this is passed string a');
test_obj.withOptional();

test_obj.recv_point_ptr_array_ptr([{'x': 1, 'y': 2, 'z': 3}, {'x': 4, 'y': 5, 'z': 6}]);
test_obj.recv_string_array_ptr(['hello', 'world', 'help', 'me', 'test', 'code']);

test_obj.print("return_array: ", test_obj.return_array());

test_obj.print("before call return_promise()");
test_obj.return_promise(false).then(a => {
    test_obj.print("resolve a: ", a);
}, b => {
    test_obj.print("reject b: ", b);
})
test_obj.print("after call return_promise()");
