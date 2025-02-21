let test = require('permissions_test');

test.foo(1, "hello");
test.foo(2, "hello", 2.0);

test.foo2(3, "byebye!", 50);

test.bar(2).then(a => {
    console.log("bar resolve int: ", a);
}).catch(data => {
    console.log("bar reject code: ", data.code, ", msg: ", data.msg);
}).finally(() => {
    console.log("bar promise finally");
});

test.bar2().then(a => {
    console.log("bar2 resolve string: ", a);
}).catch(data => {
    console.log("bar2 reject code: ", data.code, ", msg: ", data.msg);
}).finally(() => {
    console.log("bar2 promise finally");
});

test.bar2(2.6, "nice to meet you").then(a => {
    console.log("bar2 resolve string: ", a);
}).catch(data => {
    console.log("bar2 reject code: ", data.code, ", msg: ", data.msg);
}).finally(() => {
    console.log("bar2 promise finally");
});

function show_args(pre, args) {
  console.log(pre, 'args: ')
  for(var i = 0; i < args.length; i++) {
    console.log(args[i])
  }
  console.log('\n')
};

test.goo(
  function(x,y) {
    console.log("goo cb1, x: ", x, ", y: ", y);
  },
  function() {
    show_args('cb2', arguments);
  }
);

test.goo2({
  fail: function(msg, code) {
    console.log("goo2 fail, code: ", code, ", msg: ", msg);
  },
  complete: function() {
    console.log("goo2 complete");
  }
});

test.goo2({
  page_count: 1000,
  complete: function() {
    console.log("goo2 complete");
  }
});



