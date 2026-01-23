# introduction to ts2wasm.sh
- Download the compiler code through the script and compile it, and then use the compiler to compile the ts test case code to generate the corresponding wasm bytecode.
- The test case of jidl about wamr as blow. <br>
| simple | promise_test | interface_test | struct_test
- And the script will automatically compile the above ts files to generate corresponding wasm files and copy them to the "feature/tests/jidl" directory.

## Usage
- `./ts2wasm.sh <repository_url>` - Clone the compiler from the specified repository and compile it
- `./ts2wasm.sh <repository_url> <hooks_url>` - Clone the compiler from the specified repository, download the commit hook from hooks_url, and compile it
