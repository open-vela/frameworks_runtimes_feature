let test = require('Simple');
console.log("111111111", "2222", [3,4,5], {"x":6, "y":"hello world"});

function show_args(pre, args) {
  test.print(pre, 'args: ')
  for(var i = 0; i < args.length; i++) {
    test.print(args[i])
  }
  test.print('\n')
};

function show_array(arr, pre) {
  test.print(pre)
  for (var e in arr) {
    test.print('[%s]=%s', e, arr[e])
  }
  test.print('\n')
};

test.bar();
ubar6_ret = test.ubar6(2.5);
test.goo(3, 12, function(x, y, z){
    test.print('goo:x=', x, ', y=', y, ',z=', z, '\n');
});
test.goo3(function(x,y,z) {
  test.print('goo3:x=', x, ', y=', y, ',z=', z, '\n')
});

test.print('hello world!\n')
test.print('before set, test.name=', test.name, '\n');
test.name = 'joker'
test.print('after set, test.name=', test.name, '\n');
test.print('test.version=', test.version, '\n');

test.print('test.x=', test.x, '\n');
test.print('test.y=', test.y, '\n');
test.print('test.z=', test.z, '\n');

bar6_ret = test.bar6(7, 3.5, false);
test.foo3(355, 59.39923, function(x,y,z) {
  test.print('foo3 call by async in worker: x=',x, ',y=',y, ',z=',z, '\n');
});
test.foo2(200, 3.33333, function(x,y,z) {
  test.print('foo2 call by async in current: x=',x, ',y=',y,',z=',z, '\n');
}, function() {
  show_args('cb4 by async in current:', arguments);
});
test.goo2(
  function() {show_args('cb2', arguments);},
  function() {test.print('cb3, no args\n')},
  function() {show_args('cb4', arguments);}
);

test.foo(5, 'hello', 2.5);
bar2_ret = test.bar2([1,3,5,7,9,11,13])
test.print('test.bar2 return:', bar2_ret, '\n');
test.bar5(6, 100, 'world');
show_array(test.bar3, 'test.bar3:');

test.print('null param test begin ====================');
test.printStr(null)
test.foo(10, null, 2.5);
test.print('null param test end =====================\n');
