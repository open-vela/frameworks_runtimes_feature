# quickapp Feature框架单元测试

## 使用说明

本项目依赖**[vcpkg](https://github.com/Microsoft/vcpkg.git)**提供libffi和rapidjson库，请使用如下命令运行cmake:
````shell
cmake -DCMAKE_TOOLCHAIN_FILE={path_to_vcpkg}/scripts/buildsystems/vcpkg.cmake -Bbuild
````
当前并没有添加vcpkg作为子模块，请自行clone： 
````shell
git clone https://github.com/Microsoft/vcpkg.git
````
