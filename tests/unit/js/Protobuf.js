var protobuf = require("Protobuf");
console.log("struct: ", protobuf);

protobuf.proto(1, { main_monitor: { width: 123, height: 456, colorDepth: 8 }, name: "NAME", price: 666, sn_code: "2.3.4.5.6.7" })
protobuf.invoke_proto((val)=>{
    console.log(val)
})