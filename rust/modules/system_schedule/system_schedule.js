const schedule = require('system.schedule');

(async () => {
    console.log('system.schedule test begin ========================================');

    let job = {
        type: 1,
        timeout: 1000,
        interval: 500,
        triggerMethod: "start",
        params: { key: "xiaomi", value: "vela" }
    };

    try {
        const info = await schedule.scheduleJob(job);
        console.log(`scheduled job successlly, job_id: ${info.id}`);

        await new Promise((resolve, reject) => {
            setTimeout(async () => {
                try {
                    console.log(`cancel job_id: ${info.id} 3s later`);
                    await schedule.cancel(info.id);
                    console.log(`Job canceled successfully`);
                    resolve();
                } catch (error) {
                    console.error(`Failed to cancel job: ${info.id}`, error);
                    reject(error);
                }
            }, 3000);
        });

        console.log(`Job canceled successfully`);
    } catch (err) {
        console.log(`schedule job error: ${err}`);
    }

    console.log('system.schedule test end =========================================\n');
})();