var struct = require("Struct");
console.log("struct: ", struct);

struct.foo(1, {});
var Chapter = struct.bar(1024);
console.log("Chapter returned by bar: ", Chapter);

struct.proto(1, { main_monitor: { width: 123, height: 456, colorDepth: 8 }, name: "NAME", price: 666, sn_code: "2.3.4.5.6.7" })
struct.proto_cb((val)=>{
    console.log(val)
})
