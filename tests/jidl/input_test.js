const input = require('system.test.input')

// 更好的日志格式化函数
function log(type, message) {
    const prefix = {
        'START'   : '### TEST START ###',
        'SUCCESS' : '### SUCCESS    ###',
        'FAIL'    : '### FAIL       ###',
        'ERROR'   : '### ERROR      ###',
        'COMPLETE': '### COMPLETE   ###',
        'INFO'    : '### INFO       ###',
    }[type] || '>'

    console.log(`[INPUT_TEST] ${prefix} | ${message}`)
}

function testTap() {
    log('START', '===== Testing tap function =====')

    return new Promise((resolve) => {
        input.tap({
            x: 100,
            y: 200,
            success: () => {
                log('SUCCESS', `tap callback success: x=100, y=200`)

                // Use Promise to test second tap
                input.tap({
                    x: 150,
                    y: 250
                }).then(res => {
                    log('SUCCESS', `tap Promise success: x=150, y=250`)
                    resolve() // Resolve Promise after completion
                }).catch((code, error) => {
                    log('FAIL', `tap Promise failed: x=150, y=250, error=${error}, code=${code}`)
                    resolve() // Resolve Promise even on failure to continue testing
                })
            },
            fail: (code, error) => {
                log('FAIL', `tap callback failed: x=100, y=200, error=${error}, code=${code}`)
                resolve() // Continue even on failure
            },
            complete: () => {
                log('COMPLETE', 'tap callback completed')
            }
        })
    })
}

function testDrag() {
    log('START', '===== Testing drag function =====')

    return new Promise((resolve) => {
        input.drag({
            startX: 100,
            startY: 200,
            endX: 300,
            endY: 400,
            duration: 500,
            success: (res) => {
                log('SUCCESS', `drag callback success: from(100,200)to(300,400), duration=500ms`)

                // Use Promise to test second drag
                input.drag({
                    startX: 150,
                    startY: 250,
                    endX: 350,
                    endY: 450,
                    duration: 600
                }).then(res => {
                    log('SUCCESS', `drag Promise success: from(150,250)to(350,450), duration=600ms`)
                    resolve() // Resolve Promise after completion
                }).catch((code, error) => {
                    log('FAIL', `drag Promise failed: from(150,250)to(350,450), error=${error}, code=${code}`)
                    resolve() // Resolve Promise even on failure to continue testing
                })
            },
            fail: (code, error) => {
                log('FAIL', `drag callback failed: from(100,200)to(300,400), error=${error}, code=${code}`)
                resolve() // Continue even on failure
            },
            complete: () => {
                log('COMPLETE', 'drag callback completed')
            }
        })
    })
}

function testSwipe() {
    log('START', '===== Testing swipe function =====')

    return new Promise((resolve) => {
        input.swipe({
            startX: 100,
            startY: 200,
            endX: 300,
            endY: 400,
            duration: 500,
            success: (res) => {
                log('SUCCESS', `swipe callback success: from(100,200)to(300,400), duration=500ms`)

                // Use Promise to test second swipe
                input.swipe({
                    startX: 150,
                    startY: 250,
                    endX: 350,
                    endY: 450,
                    duration: 600
                }).then(res => {
                    log('SUCCESS', `swipe Promise success: from(150,250)to(350,450), duration=600ms`)
                    resolve() // Resolve Promise after completion
                }).catch((code, error) => {
                    log('FAIL', `swipe Promise failed: from(150,250)to(350,450), code=${code}, error=${error}`)
                    resolve() // Resolve Promise even on failure to continue testing
                })
            },
            fail: (code, error) => {
                log('FAIL', `swipe callback failed: from(100,200)to(300,400), code=${code}, error=${error}`)
                resolve() // Continue even on failure
            },
            complete: () => {
                log('COMPLETE', 'swipe callback completed')
            }
        })
    })
}

function testPressSystemButton() {
    log('START', '===== Testing pressSystemButton function =====')

    return new Promise((resolve) => {
        input.pressSystemButton({
            button: 'HOME',
            success: (res) => {
                log('SUCCESS', `pressSystemButton HOME callback success: result=${JSON.stringify(res)}`)

                // Test DOWN button
                input.pressSystemButton({
                    button: 'down'
                }).then(res => {
                    log('SUCCESS', `pressSystemButton DOWN Promise success: result=${JSON.stringify(res)}`)

                    // Test THIRD button
                    return input.pressSystemButton({
                        button: 'THIRD'
                    })
                }).then(res => {
                    log('SUCCESS', `pressSystemButton Third Promise success: result=${JSON.stringify(res)}`)
                    resolve() // Complete all button tests
                }).catch((code, error) => {
                    log('FAIL', `pressSystemButton Promise failed: code=${code}, error=${error}`)
                    resolve() // Continue even on error
                })
            },
            fail: (code, error) => {
                log('FAIL', `pressSystemButton HOME callback failed: code=${code}, error=${error}`)
                resolve() // Continue even on failure
            },
            complete: () => {
                log('COMPLETE', 'pressSystemButton HOME callback completed')
            }
        })
    })
}

function testLongPressSystemButton() {
    log('START', '===== Testing longPressSystemButton function =====')

    return new Promise((resolve) => {
        input.longPressSystemButton({
            button: 'HOME',
            duration: 2000,
            success: (res) => {
                log('SUCCESS', `longPressSystemButton HOME callback success: duration=2000ms`)

                // Use Promise to test second long press
                input.longPressSystemButton({
                    button: 'DOWN',
                    duration: 3000
                }).then(res => {
                    log('SUCCESS', `longPressSystemButton DOWN Promise success: duration=3000ms`)
                    resolve() // Resolve Promise after completion
                }).catch((code, error) => {
                    log('FAIL', `longPressSystemButton DOWN Promise failed: code=${code}, error=${error}`)
                    resolve() // Resolve Promise even on failure to continue testing
                })
            },
            fail: (code, error) => {
                log('FAIL', `longPressSystemButton HOME callback failed: code=${code}, error=${error}`)
                resolve() // Continue even on failure
            },
            complete: () => {
                log('COMPLETE', 'longPressSystemButton HOME callback completed')
            }
        })
    })
}

function testWheel() {
    log('START', '===== Testing wheel function =====')

    return new Promise((resolve) => {
        input.wheel({
            value: 5,
            success: (res) => {
                log('SUCCESS', `wheel callback success: value=5`)

                // Use Promise to test second wheel operation
                input.wheel({
                    value: -3
                }).then(res => {
                    log('SUCCESS', `wheel Promise success: value=-3`)
                    resolve() // Resolve Promise after completion
                }).catch((code, error) => {
                    log('FAIL', `wheel Promise failed: value=-3, code=${code}, error=${error}`)
                    resolve() // Resolve Promise even on failure to continue testing
                })
            },
            fail: (code, error) => {
                log('FAIL', `wheel callback failed: value=5, code=${code}, error=${error}`)
                resolve() // Continue even on failure
            },
            complete: () => {
                log('COMPLETE', 'wheel callback completed')
            }
        })
    })
}

function testTapEdgeCases() {
    log('START', '===== Testing tap edge cases =====')

    return new Promise((resolve) => {
        // Test missing parameters
        log('INFO', 'Testing tap with missing parameters')
        input.tap({}).catch((code, error) => {
            log('ERROR', `Expected error (missing tap parameters): code=${code}, error=${error}`)
            resolve() // Resolve Promise even on failure to continue testing
        })
    })
}

function testDragEdgeCases() {
    log('INFO', 'Testing drag with invalid parameter types')

    return new Promise((resolve) => {
        return input.drag({
            startX: "string instead of number",
            startY: 200,
            endX: 300,
            endY: 400
        }).catch((code, error) => {
            log('ERROR', `Expected error (invalid parameter type): code=${code}, error=${error}`)
            resolve() // Resolve Promise even on failure to continue testing
        })
    })
}

function testPressSystemButtonEdgeCases() {
    log('INFO', 'Testing pressSystemButton with invalid parameter types')

    return input.pressSystemButton({
        button: 'NONEXISTENT_BUTTON'
    }).catch((code, error) => {
        log('ERROR', `Expected error (invalid parameter type): code=${code}, error=${error}`)
        resolve() // Resolve Promise even on failure to continue testing
    })
}

// Sequential execution of all tests
async function runAllTests() {
    log('INFO', 'Starting all input tests...')

    await testTap()
    log('INFO', 'Tap test completed, starting drag test')

    await testDrag()
    log('INFO', 'Drag test completed, starting swipe test')

    await testSwipe()
    log('INFO', 'Swipe test completed, starting pressSystemButton test')

    await testPressSystemButton()
    log('INFO', 'PressSystemButton test completed, starting longPressSystemButton test')

    await testLongPressSystemButton()
    log('INFO', 'LongPressSystemButton test completed, starting wheel test')

    await testWheel()
    log('INFO', 'Wheel test completed, starting edge cases test')

    //Only one edge case test can run at a time, as errors terminate execution
    // await testTapEdgeCases()
    // await testDragEdgeCases()
    // await testPressSystemButtonEdgeCases()
    log('INFO', '✨✨✨ All input tests completed! ✨✨✨')
}

// Execute tests
runAllTests().catch(err => {
    log('ERROR', `Error during test execution: ${err}`)
})
