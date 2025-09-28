#!/bin/bash
set -euxo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
cd $script_dir

RUST_HOME=/tools/rust
sudo chmod 777 -R $RUST_HOME
mkdir -p $HOME/.cargo

echo "Replace crates.io with aliyun.."
echo "[source.crates-io]
replace-with = 'aliyun'

[source.aliyun]
registry = \"sparse+https://mirrors.aliyun.com/crates.io-index/\"" | sudo tee $HOME/.cargo/config.toml

echo "Setting up vela-nightly Rust toolchain..."

RUST_TOOLCHAIN_PATH=$WORKSPACE/prebuilts/rust/linux/nightly/rustc
if [ -d "${RUST_TOOLCHAIN_PATH}" ]; then
    # Create vela-nightly toolchain link if it doesn't exist
    if ! rustup toolchain list | grep -q "vela-nightly"; then
        echo "Setting up vela-nightly Rust toolchain..."
        rustup toolchain link vela-nightly ${RUST_TOOLCHAIN_PATH}
    fi
    # Set vela-nightly as toolchain via RUSTUP_TOOLCHAIN environment variable
    export RUSTUP_TOOLCHAIN=vela-nightly
    echo "Rust toolchain set to vela-nightly via RUSTUP_TOOLCHAIN"
else
    echo "Warning: Rust prebuilt toolchain not found at ${RUST_TOOLCHAIN_PATH}"
fi

echo "Begin rustfmt check..."
cargo fmt --check

echo "Begin install deps..."
sudo apt install -y libuv1-dev clang

echo "Begin clippy check..."
BUILD_ON_LINUX=1 FEATURE_STATIC_BINDING=1 cargo clippy --workspace -- -A dead_code -A clippy::missing_safety_doc