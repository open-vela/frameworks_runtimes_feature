const network = require("system.network");

// 使用 async/await 顺序执行测试
(async () => {
    console.log(
        "system.network test begin ========================================",
    );

    try {
        // 测试 1: 调用 getType() 获取网络类型
        console.log("\n--- Test 1: getType() ---");
        const typeResult = await network.getType();
        console.log(`Network type result: ${JSON.stringify(typeResult)}`);
        // 根据日志，返回结构是 {"data":{"type":"wifi"}}
        if (typeResult && typeResult.data && typeResult.data.type) {
            console.log(`Network type: ${typeResult.data.type}`);
        } else if (typeResult && typeResult.type) {
            // 兼容直接返回 {type:"wifi"} 的情况
            console.log(`Network type: ${typeResult.type}`);
        } else {
            console.error("Invalid typeResult structure:", typeResult);
        }

        // 测试 2: 多次调用 getType() 以验证每次都重新获取
        console.log("\n--- Test 2: Multiple getType() calls ---");
        for (let i = 0; i < 3; i++) {
            try {
                const result = await network.getType();
                if (result && result.data && result.data.type) {
                    console.log(
                        `Call ${i + 1}: Network type = ${result.data.type}`,
                    );
                } else if (result && result.type) {
                    // 兼容直接返回 {type:"wifi"} 的情况
                    console.log(`Call ${i + 1}: Network type = ${result.type}`);
                } else {
                    console.error(
                        `Call ${i + 1}: Invalid result structure:`,
                        result,
                    );
                }
            } catch (err) {
                console.error(`Call ${i + 1} error:`, err);
            }
            // 等待一小段时间
            await new Promise((resolve) => setTimeout(resolve, 500));
        }

        // 测试 3: 订阅网络变化事件
        console.log("\n--- Test 3: subscribe() to network changes ---");
        try {
            await network.subscribe({
                callback: (result) => {
                    // callback 现在返回对象 {type: "wifi"} 或 {data: {type: "wifi"}}
                    const type = result?.data?.type || result?.type || result;
                    console.log(
                        `*** Network type changed callback: ${type} (result: ${JSON.stringify(result)}) ***`,
                    );
                },
            });
            console.log(`Subscribe successful`);

            // 保持订阅一段时间，等待网络变化事件
            console.log(
                "Waiting for network interface changes (20 seconds)...",
            );
            await new Promise((resolve) => setTimeout(resolve, 20000));
        } catch (err) {
            console.error(`Subscribe error:`, err);
            throw err;
        }

        // 测试 4: 取消订阅
        console.log("\n--- Test 4: unsubscribe() ---");
        try {
            network.unsubscribe();
            console.log("Unsubscribed successfully");

            // 等待一小段时间，确保监控任务已退出
            await new Promise((resolve) => setTimeout(resolve, 10000));
        } catch (err) {
            console.error(`Unsubscribe error:`, err);
        }

        // 测试 5: 取消订阅后再次订阅
        console.log("\n--- Test 5: Subscribe again after unsubscribe ---");
        try {
            await network.subscribe({
                callback: (result) => {
                    // callback 现在返回对象 {type: "wifi"} 或 {data: {type: "wifi"}}
                    const type = result?.data?.type || result?.type || result;
                    console.log(
                        `*** Second subscription callback: ${type} (result: ${JSON.stringify(result)}) ***`,
                    );
                },
            });
            console.log(`Second subscribe successful`);

            // 等待一小段时间
            await new Promise((resolve) => setTimeout(resolve, 2000));

            // 再次取消订阅
            network.unsubscribe();
            console.log("Second unsubscribe successful");
        } catch (err) {
            console.error(`Second subscribe error:`, err);
        }
    } catch (error) {
        console.error(`Operation failed:`, error);
        if (error.code !== undefined) {
            console.error(`Error code: ${error.code}`);
        }
        if (error.data !== undefined) {
            console.error(`Error data: ${error.data}`);
        }
    }

    console.log(
        "\nsystem.network test end =========================================\n",
    );
})();
