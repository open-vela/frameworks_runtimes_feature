#!/bin/bash

CUR_DIR=`pwd`
PYTHON="python3"

mkdir -p $CUR_DIR/.out

run_feature() {
  if [ -d $1 ]; then
    jidl_files=`ls $1/*.jidl`
  elif [ -f $1 ]; then
    jidl_files=$1
  fi
  for f in $jidl_files
  do
    echo "=== $f =="
    cmd="$PYTHON $CUR_DIR/jidlast.py $f"
    echo "TOJSON: $cmd"
    $cmd > /dev/null
    json_file=${f%.*}.json
    file_name=${f##*/}
    file_name=${file_name%.*}
    cmd="$PYTHON $CUR_DIR/jsongensource.py $json_file -out-dir $CUR_DIR/.out -header $file_name.h -source $file_name.cpp"
    echo "GEN: $cmd"
    $cmd
    echo "=========================================="
  done
}

run_lvgl_binding_gen() {
  f=$1
  echo "=== UI $f === "
  cmd="$PYTHON $CUR_DIR/jidlast.py $f"
  echo "TOJSON: $cmd"
  $cmd > /dev/null
  json_file=${f%.*}.json
  file_name=${f##*/}
  file_name=${file_name%.*}
  cmd="$PYTHON $CUR_DIR/ui_render.py -t $CUR_DIR/lvgl-binding/qjs_lvgl_temp.mt -c $CUR_DIR/lvgl-binding/qjs-lvgl-config.json -i $json_file -o $CUR_DIR/.out/${file_name}_ui.c"
  echo "GEN: $cmd"
  $cmd
  echo "=========================================="
}

run_features() {
  run_feature $CUR_DIR/samples
  run_feature $CUR_DIR/../../tests/jidl
}


run_all() {
  run_features
  run_lvgl_binding_gen $CUR_DIR/samples/lvgl-ui.jidl
}

if [ $# == 0 ]; then
  run_all
fi

case $1 in
  features)
    run_features
    ;;
  lvgl)
    run_lvgl_binding_gen $CUR_DIR/samples/lvgl-ui.jidl
    ;;
  lvgl9)
    run_lvgl_binding_gen $CUR_DIR/samples/lvgl-ui9.jidl
    ;;
  feature)
    shift
    run_feature $*
    ;;
  *)
    run_all
    ;;
esac
