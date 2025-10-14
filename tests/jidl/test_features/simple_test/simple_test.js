let test = require('simple_test');

function show_args(pre, args) {
  console.log(pre, 'args: ')
  for(var i = 0; i < args.length; i++) {
    console.log(args[i])
  }
};

function show_array(arr, pre) {
  console.log(pre)
  for (var e in arr) {
    console.log('[', e, '] = ', arr[e])
  }
};

test.bar();
ubar6_ret = test.ubar6(2.5);
test.goo(3, 12, function(x, y, z){
    console.log('goo:x=', x, ', y=', y, ',z=', z, '\n');
});
test.goo3(function(x,y,z) {
  console.log('goo3:x=', x, ', y=', y, ',z=', z, '\n')
});

console.log('hello world!\n')
console.log('before set, test.name=', test.name, '\n');
test.name = 'joker'
console.log('after set, test.name=', test.name, '\n');
console.log('test.version=', test.version, '\n');

console.log('test.x=', test.x, '\n');
console.log('test.y=', test.y, '\n');
console.log('test.z=', test.z, '\n');

bar6_ret = test.bar6(7, 3.5, false);
test.foo3(355, 59.39923, function(x,y,z) {
  console.log('foo3 call by async in worker: x=',x, ',y=',y, ',z=',z, '\n');
});
test.foo2(200, 3.33333, function(x,y,z) {
  console.log('foo2 call by async in current: x=',x, ',y=',y,',z=',z, '\n');
}, function() {
  show_args('cb4 by async in current:', arguments);
});
test.goo2(
  function() {show_args('cb2', arguments);},
  function() {console.log('cb3, no args\n')},
  function() {show_args('cb4', arguments);}
);

test.foo(5, 'hello', 2.5);
bar2_ret = test.bar2([1,3,5,7,9,11,13])
console.log('test.bar2 return:', bar2_ret, '\n');
test.bar5(6, 100, 'world');
show_array(test.bar3(), 'test.bar3:');

console.log('null param test');
test.foo(10, null, 2.5);
