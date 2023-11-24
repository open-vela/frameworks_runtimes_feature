# The test case of jidl about wamr as blow.
- simple
- promise_test
- interface_test
- struct_test

## Wasmnizer-ts download and compile.
```
cd vela/frameworks/base
git clone git@github.com:LevelCA/Wasmnizer-ts.git
cd Wasmnizer-ts 
npm i && npm run build
```
## Dependent project download and branch switching
- wamr
```
cd vela/apps/interpreters/wamr/wamr
git checkout gc_refactor
git pull
```
- quickjs
```
cd vela/apps/interpreters/quickjs/quickjs
git checkout wamr_gc
git pull
```
## Compile jidl_main binary.
```
cd vela/frameworks/base/feature/tests/jidl/
mkdir build && cd build
cmake ..
make
```
## Compile ts case file and test case.
```
cd vela/frameworks/base/feature/tests/jidl/build
# struct_test
node ../../../../Wasmnizer-ts/build/cli/ts2wasm.js ../struct_test.ts -o struct_test.wasm
mv struct_test.wasm ../
./feature_jidl_test ../struct_test.wasm ../manifest.json
```
```
# interface_test
node ../../../../Wasmnizer-ts/build/cli/ts2wasm.js ../interface_test.ts -o interface_test.wasm
mv struct_test.wasm ../
./feature_jidl_test ../interface_test.wasm ../manifest.json
```
```
# promise_test
node ../../../../Wasmnizer-ts/build/cli/ts2wasm.js ../promise_test.ts -o promise_test.wasm
mv promise_test.wasm ../
./feature_jidl_test ../promise_test.wasm ../manifest.json
```
```
# simple_1_0
node ../../../../Wasmnizer-ts/build/cli/ts2wasm.js ../simple_1_0.ts -o simple_1_0.wasm
mv simple_1_0.wasm ../
./feature_jidl_test ../simple_1_0.wasm ../manifest.json
```