# quickapp Feature framework JIDL glue code generation tools

# 使用方式
jidlast.py // convert JIDL file to a JSON format AST tree
command:
python3 ./jidlast.py <path_to_jidl_file>
example:
````shell
python3 ./jidlast.py ./samples/Simple.jidl
````
./jsongensource.py // generate glue code against the Feature framework from a JSON AST tree
command:
python3 ./jsongensource.py <path_to_json_file> [-out-dir <out_dir>] <-header header_file_name -source source_file_name>
examples:
````shell
python3 ./jsongensource.py ./samples/Simple.json -out-dir ./samples/ -header simple_1_0.h -source simple_1_0.cpp
python3 ./jsongensource.py ./samples/Simple.json -header simple_1_0.h -source simple_1_0.cpp
````
run the test
````shell
./run_all_test.sh
````

generator c++ file
```shell
python3 ./jsongensource.py ./samples/Simple.json -lang c++ -out-dir ./samples/ -header simple_1_0.h -source simple_1_0.cpp

# JIDL规则
## 基本规则
1, JIDL支持的数据类型如下：
A, 基本类型：int, float, double, string, boolean, long, uint, void, object (object表示any类型);
B, 复杂类型：array, promise, typed_array_type (array表示其他基本类型的数组, 比如int[], string[]等, promise类型对应于js中的promise, 例如promise<int, string>, typed_array_type类型对应于js中的TypedArray, 比如typed_array<uint8>, typed_array<int16>等)
2, JIDL支持的基本语法单位如下：
module, funciton, use, callback, event, property, interface, struct
(其中, funciton可以接受callback类型的参数，可以返回promise类型的值，interface可包含function和property类型的成员，struct可包基本含数据类型、数组类型、callback类型以及嵌套的struct引用类型的成员)

## 隐含规则
1, 自定义数组类型：例如先定义一个struct A, 然后可以定义一个A的数组 A[], 也可以把A[]又定义为另一个struct的成员；
2, 缺省值：JIDL函数传参数的缺省值literal_value目前只支持四种常量：number, integer, string, boolean (JIDL语法隐含支持array_literal，但feature框架还不支持，比如[0, "hello", 3.5])；
3, 一般数据类型，或struct的成员类型如果没有定义缺省值，在作为函数参数传递时，不能省略掉实参 (后面会添加除四种常量以外的缺省值常量支持，比如object支持缺省值);
4, interface支持callback成员 (正在实现中);

