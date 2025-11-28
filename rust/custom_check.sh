#!/bin/bash
set -euxo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
cd $script_dir

T=$WORKSPACE
# Generate .cargo/config.toml at project root
mkdir -p "$T/.cargo"
cat > "$T/.cargo/config.toml" <<EOF
[path-bases]
runtime = "$T/apps/frameworks/runtimes/rust"
external = "$T/apps/external/rust"
android = "$T/apps/external/android"

[source.crates-io]
replace-with = 'aliyun'

[source.aliyun]
registry = "sparse+https://mirrors.aliyun.com/crates.io-index/"

EOF

echo "Setting up vela-nightly Rust toolchain..."
SYSTEM=$(uname | tr '[:upper:]' '[:lower:]')
if [ -d $T/prebuilts/rust/${SYSTEM}/nightly/rustc/bin ]; then
    RUST_TOOLCHAIN_PATH=$T/prebuilts/rust/${SYSTEM}/nightly/rustc/bin
else
    echo "no rust toolchain found, exit..."
    exit 0
fi
export PATH=$RUST_TOOLCHAIN_PATH:$PATH
export RUST_SRC_PATH=$T/prebuilts/rust/${SYSTEM}/nightly/rustc/lib/rustlib/src/rust/library
export CARGO_HOME=$T/.cargo

echo "Begin rustfmt check..."
cargo fmt --check

echo "Begin install deps..."
sudo apt install -y libuv1-dev clang

echo "Begin clippy check..."
FEATURE_STATIC_BINDING=1 cargo clippy --workspace -- -A dead_code -A clippy::missing_safety_doc