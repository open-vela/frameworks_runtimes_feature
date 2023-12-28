通用 feature 框架使用隐藏规则

1. 前端参数有些是定义为可选的，目前 jidl 定义不支持可选/必选，那么对于基础数据类型（字符串，整型数字，浮点数，bool）可以使用 jidl 默认值，对于复杂数据类型（结构体，数组，回调函数）可以设计为 any 方式透传（在 jidl 中关键字为 object）
2. Feature 框架可以支持结构体自引用，以及结构体中包含结构体/数组
3. Feature 框架的数组只支持基础数据类型与结构体数组
4. 回调函数的 cid 可以先跟据 bool FeatureCheckCallbackId(FeatureInstanceHandle handle, FtCallbackId cid)去判断 JS 上是否有这个回调函数，如果有的话再执行 FeatureInvokeCallback
5. 回调函数的 cid 最终需要通过 FeatureRemoveCallback 去释放
6. Feature 框架持有 uv_loop_t* loop，可以通过 uv_loop_t* FeatureGetUVLoop(FeatureManagerHandle handle)去拿到，但是使用前需要指针判空，可能拿到的是空指针
7. Feature 框架支持异步调用 FeaturePost，可以在自己的线程中使用
