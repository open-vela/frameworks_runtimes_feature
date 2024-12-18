let t0 = Date.now();
console.log("Interconnect test start...", t0)
const interconnect = require("system.interconnect")
let conn = interconnect.instance({
  package: "com.icoolme.android.weather",
  fingerprint: "xxxxxxxxxxxxxxxxx"
})

let t1 = Date.now();

console.log("Interconnect test connecting", t1)

conn.onmessage = (data) => {
  console.log('onmessage', data)
}

conn.getReadyState({
    success: data => {
      if (data.status === 1) {
        console.log('连接成功')
      } else if (data.status === 2) {
        console.log('连接失败')
      }
    },
    fail: (data, code) => {
      console.log(`handling fail, code = ${code}`)
    }
  })

conn.diagnosis({
  success: (data) => {
    console.log('diagnosis success', data.status)
  },
  fail: (data, code) => {
    console.log('diagnosis fail', data, code)
  }
})

conn.onopen = (is_first) => {
  let topen = Date.now();
  console.log('Interconnect onopen: ', topen, is_first);
}

conn.send({"hello" : "world"});

conn.onclose = () => {
  let tclose = Date.now();
  console.log('Interconnect onclose: ', tclose);
}

conn.onerror = (data) => {
  let terror = Date.now();
  console.log('Interconnect onerror: ', terror);
}