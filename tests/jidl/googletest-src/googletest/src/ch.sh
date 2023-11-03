#!/bin/bash
filenames=$(cat files)

for f in ${filenames}
do
    echo $f
    mv $f.cc $f.cpp
done

