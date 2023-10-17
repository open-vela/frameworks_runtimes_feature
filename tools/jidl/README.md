# quickapp Feature framework JIDL glue code generation tools

# usage
jidlast.py // convert JIDL file to a JSON format AST tree
````shell
command:
python3 ./jidlast.py <path_to_jidl_file>
example:
python3 ./jidlast.py ./samples/Simple.jidl
````
./jsongensource.py // generate glue code against the Feature framework from a JSON AST tree
````shell
command:
python3 ./jsongensource.py <path_to_json_file> [-out-dir <out_dir>] <-header header_file_name -source source_file_name>
examples:
python3 ./jsongensource.py ./samples/Simple.json -out-dir ./samples/ -header simple_1_0.h -source simple_1_0.cpp
python3 ./jsongensource.py ./samples/Simple.json -header simple_1_0.h -source simple_1_0.cpp
````

run the test
./run_all_test.sh
