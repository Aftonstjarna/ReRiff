#!/usr/bin/env bash
set -euo pipefail
cd /var/home/afton/.dev/ReRiff
zig_tool=${1:-/tmp/reriff-zig/zig-x86_64-linux-0.14.1/zig}
export ZIG_GLOBAL_CACHE_DIR=/tmp/reriff-menu-zig-global
export ZIG_LOCAL_CACHE_DIR=/tmp/reriff-menu-zig-local
mkdir -p build/experimental-engine-menu
"$zig_tool" cc -target x86-windows-gnu -shared -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/engine_menu.c -o build/experimental-engine-menu/ReRiff.Notify.dll \
  -luser32 -ladvapi32 -static
"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_engine.c -o build/experimental-engine-menu/test-engine.exe \
  -Wl,--image-base,0x10000000 -luser32 -ladvapi32 -static
