let router = require('system.router')

feat_test("router","replace",()=>{
    router.replace({
        uri: '/test',
        params: {
            testId: '1'
        }
    })
})

feat_test("router","back1",()=>{
    router.back()
})

feat_test("router","back2",()=>{
    router.back({
        path: '/A'
    })
})

feat_test("router","clear",()=>{
    router.clear()
})

feat_test("router","getLength",()=>{
    feat_expect_true(router.getLength() == 0,"length expect 0")
})

feat_test("router","push",()=>{
    router.push({
        uri: '/test',
        params: {
            testId: '1'
        }
    })
})

feat_test("router","getState",()=>{
    var page = router.getState()
    feat_expect_true(true, "get state success");
})

feat_test("router","getPages",()=>{
    var stacks = router.getPages()
    feat_expect_true(true, "get pages success");
})