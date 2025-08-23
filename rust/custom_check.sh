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

echo "Begin rustfmt check..."
cargo fmt --check

echo "Begin install deps..."
sudo apt install -y libuv1-dev clang

echo "Begin clippy check..."
BUILD_ON_LINUX=1 FEATURE_STATIC_BINDING=1 cargo clippy --workspace -- -A dead_code -A clippy::missing_safety_doc