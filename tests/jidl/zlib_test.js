let zlib = require('system.zlib');
// feature_test_cli data/zlib_test.js
const base64String =
        "eJyd0TEOgzAMQNG7eI4s25iE5CpVB1QyRCoCQdoFcXfaqalKB9jz/GVngXEaxjjlFGcIlwVS6iCAQ0YGA8/2/ogQyEBOfZxz248Q2FXeEnnfkFUDt6F7PRGi1Xy4lFz0x2tFNWtDbtcLNlidzwu6r/xhzmWdD3JFLfl+3dZO+A8XlPO7v+vlz+HxAXLi9td1A30AoEA="
      // 1. Base64解码
      function inflate(data) {
        const byteArray = base64ToUint8Array(data)
        const decompressedData = zlib.decompressSync(byteArray) // 解压缩
        // 3. 转换为字符串
        const originalString = uint8ArrayToString(decompressedData)
        console.log("inflate originalString==", originalString)
        return JSON.parse(originalString)
      }
      function base64ToUint8Array(base64) {
        console.log(base64)
        const base64Chars =
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
        const padding = "=".repeat((4 - (base64.length % 4)) % 4) // 处理 Base64 的填充字符
        const base64Url = (base64 + padding)
          .replace(/-/g, "+")
          .replace(/_/g, "/") // 处理 URL 安全的 Base64
        console.log(base64Url)
        let byteLength = (base64Url.length * 3) / 4 // 计算解码后的字节长度
        if (base64Url.endsWith("==")) byteLength -= 2
        else if (base64Url.endsWith("=")) byteLength -= 1
        const outputArray = new Uint8Array(byteLength)
        let position = 0
        console.log(base64Url.length)
        for (let i = 0; i < base64Url.length; i += 4) {
          const char1 = base64Chars.indexOf(base64Url[i])
          const char2 = base64Chars.indexOf(base64Url[i + 1])
          const char3 = base64Chars.indexOf(base64Url[i + 2])
          const char4 = base64Chars.indexOf(base64Url[i + 3])
          const byte1 = (char1 << 2) | (char2 >> 4)
          const byte2 = ((char2 & 15) << 4) | (char3 >> 2)
          const byte3 = ((char3 & 3) << 6) | char4
          console.log("char=========>", char1, char2, char3, char4)
          outputArray[position++] = byte1
          if (char3 !== -1) outputArray[position++] = byte2 // 64 是填充字符 '='
          if (char4 !== -1) outputArray[position++] = byte3
        }
        console.log(outputArray)
        return outputArray
      }
      function uint8ArrayToString(uint8Array) {
        let result = ""
        for (let i = 0; i < uint8Array.length; i++) {
          result += String.fromCharCode(uint8Array[i]) // 将每个字节转换为字符
        }
        return result
      }
      console.log(inflate(base64String))
