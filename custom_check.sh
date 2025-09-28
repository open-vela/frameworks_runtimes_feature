#!/bin/bash
set -euxo pipefail

echo "begin rustfmt check..."
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
cd $script_dir/rust
bash custom_check.sh