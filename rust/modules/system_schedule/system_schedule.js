const test = require('system.schedule');

console.log('system.schedule test begin ========================================');

let job = {
    type: 1,
    timeout: 60000, // 60s
    interval: 10000, // 10s
    triggerMethod: "start",
    params: { key: "key", value: "value" }
}

test.scheduleJob(job).then(info => {
    console.log(`scheduled job, job_id: ${info.id}`);
    setTimeout(() => {
      console.log(`cancel job, job_id: ${info.id}`);
      const ret = test.cancel(info.id);
      console.log(`canceled Job, ret: ${ret}`);
    }, 3000);
}).catch(err => {
    console.log(`schedule job error: ${err}`);
}).finally(() => {
    console.log(`schedule job completed`);
});


console.log('system.schedule test end =========================================\n');