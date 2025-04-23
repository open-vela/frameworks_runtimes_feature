let test = require('callback_test');

function show_args(pre, args) {
  console.log(pre, 'args: ')
  for(var i = 0; i < args.length; i++) {
    console.log(args[i])
  }
};

console.log('test begin')
test.goo(3, 12, function(x, y, z){
    console.log('goo: cb1, x=', x, ', y=', y, ',z=', z, '\n');
});

test.goo2(
  function() {show_args('cb2', arguments);},
  function() {console.log('cb3, no args\n')},
  function() {show_args('cb4', arguments);}
);

test.goo3(function(x,y,z) {
  console.log('goo3: cb1, x=', x, ', y=', y, ',z=', z, '\n')
});

test.foo2(200, 3.33333, function(x,y,z) {
  console.log('foo2: cb1, x=',x, ',y=',y,',z=',z, '\n');
}, function() {
  show_args('foo2: cb2, arguments:', arguments);
});

test.foo3(355, 59.39923, function(x,y,z) {
  console.log('foo3: cb1, x=',x, ',y=',y, ',z=',z, '\n');
});

console.log('test ended\n')
