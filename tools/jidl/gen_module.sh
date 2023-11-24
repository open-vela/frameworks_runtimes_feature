#!/bin/bash

echo "USAGE $0 [moudle_path] [module_name] -- build one module"
echo "USAGE $0 [moudle_path]  -- build all module"

SCRIPT_PATH=`dirname ${BASH_SOURCE[0]}`

echo $SCRIPT_PATH

export PYTHONPATH=$SCRIPT_PATH:$PYTHONPATH

PYTHON="python3"

if [ -d $1 ]; then
  MODULE_PATH=$1
  MODULE_NAME=''
elif [ -f $1 ]; then
  MODULE_PATH=`dirname $1`
  MODULE_NAME=$1
fi
OUT_PATH=$MODULE_PATH/../

call_cmd() {
  cmd="$*"
  echo "CALL: $cmd"
  $cmd
}

gen_module_feature() {
  if [ ! -f $1 ]; then
    echo "$1 is not a file"
    return
  fi
  file=$1
  echo "=========$file======"

  call_cmd $PYTHON $SCRIPT_PATH/jidlast.py $file
  json_file=${file%.*}.json
  file_name=${file##*/}
  file_name=${file_name%.*}
  call_cmd $PYTHON $SCRIPT_PATH/jsongensource.py $json_file -out-dir $OUT_PATH -header $file_name.h -source $file_name.cpp
  echo "========================================="
}

if [[ "x$MODULE_NAME" == "x" ]]; then
  files=`find $MODULE_PATH -name "*.jidl"`
  for f in $files
  do
    gen_module_feature $f
  done
else
  gen_module_feature $MODULE_NAME
fi
