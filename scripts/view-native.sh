#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
make -C "$root/NativeLinux" all
exec "$root/build/native-linux/reriff" "$@"
