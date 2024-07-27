var struct = require("Struct");
console.log("struct: ", struct);

struct.foo(1, {});
var Chapter = struct.bar(1024);
console.log("Chapter returned by bar: ", Chapter);