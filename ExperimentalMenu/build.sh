#!/usr/bin/env bash
set -euo pipefail
cd /var/home/afton/.dev/ReRiff
zig_tool=${1:-/tmp/reriff-zig/zig-x86_64-linux-0.14.1/zig}
export ZIG_GLOBAL_CACHE_DIR=/tmp/reriff-menu-zig-global
export ZIG_LOCAL_CACHE_DIR=/tmp/reriff-menu-zig-local
mkdir -p build/experimental-menu
"$zig_tool" cc -target x86-windows-gnu -shared -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/menu.c -o build/experimental-menu/ReRiff.Notify.dll \
  -luser32 -lgdi32 -lwininet -static
"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_menu.c -o build/experimental-menu/test-menu.exe \
  -luser32 -lgdi32 -lwininet -static
"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_load.c -o build/experimental-menu/test-load.exe \
  -luser32 -static
