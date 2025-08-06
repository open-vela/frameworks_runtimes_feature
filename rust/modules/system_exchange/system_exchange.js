const test = require('system.exchange');

// 将回调风格的方法包装成返回 Promise 的函数
function promisify(fn) {
  return (args) => new Promise((resolve, reject) => {
    fn({
      ...args,
      success: resolve,
      fail: (data, code) => reject({ code, data })
    });
  });
}

const setAsync = promisify(test.set);
const getAsync = promisify(test.get);
const removeAsync = promisify(test.remove);
const clearAsync = promisify(test.clear);

// 使用 async/await 顺序执行
(async () => {
  const key = 'xiaomi';
  const value = 'vela';

  console.log('system.exchange test begin ====================');

  try {
    // Set
    await setAsync({ key, value });
    console.log(`Set property: ${key} -> value: ${value}`);

    // Get
    const ret = await getAsync({ key });
    console.log(`Get property: ${key} -> value: ${ret.value}`);

    // Remove
    await removeAsync({ key });
    console.log(`Remove property: ${key}`);

    // // Clear
    await clearAsync({});
    console.log(`Clear all properties`);

  } catch (error) {
    console.error(`Operation failed, code = ${error.code}, msg = ${error.data}`);
  }

  console.log('system.exchange test end =====================\n');
})();