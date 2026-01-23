#!/bin/bash

#
# Copyright (C) 2023 Xiaomi Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

REPO_URL=$1
HOOKS_URL=$2

if [ ! -d "ts2wasm" ]; then
    if [ -n "$HOOKS_URL" ]; then
        git clone "$REPO_URL" -b main && (cd "ts2wasm" && mkdir -p `git rev-parse --git-dir`/hooks/ && curl -Lo `git rev-parse --git-dir`/hooks/commit-msg "$HOOKS_URL" && chmod +x `git rev-parse --git-dir`/hooks/commit-msg)
    else
        git clone "$REPO_URL" -b main && (cd "ts2wasm" && mkdir -p `git rev-parse --git-dir`/hooks/)
    fi
fi

cd ts2wasm
npm i && npm run build
cd ..

BUILD_DIR="ts2wasm/build/cli"
COMPILER="ts2wasm.js"
OUTPUT_DIR="../jidl"

if [ -e "$BUILD_DIR/$COMPILER" ]; then
    node $BUILD_DIR/$COMPILER $OUTPUT_DIR/simple_1_0.ts -o $OUTPUT_DIR/simple_1_0.wasm
    node $BUILD_DIR/$COMPILER $OUTPUT_DIR/promise_test.ts -o $OUTPUT_DIR/promise_test.wasm
    node $BUILD_DIR/$COMPILER $OUTPUT_DIR/interface_test.ts -o $OUTPUT_DIR/interface_test.wasm
    node $BUILD_DIR/$COMPILER $OUTPUT_DIR/struct_test.ts -o $OUTPUT_DIR/struct_test.wasm
fi