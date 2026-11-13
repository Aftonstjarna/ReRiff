#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
zig_tool=${1:-/tmp/reriff-zig/zig-x86_64-linux-0.14.1/zig}
export ZIG_GLOBAL_CACHE_DIR=/tmp/reriff-menu-zig-global
export ZIG_LOCAL_CACHE_DIR=/tmp/reriff-menu-zig-local
menu_version=$(cat VERSION)
mkdir -p build/experimental-engine-menu
"$zig_tool" cc -target x86-windows-gnu -shared -O2 -Wall -Wextra -Werror \
  "-DRERIFF_VERSION=\"$menu_version\"" ExperimentalMenu/engine_menu.c -o build/experimental-engine-menu/ReRiff.Notify.dll \
  -luser32 -lgdi32 -ladvapi32 -static
"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  "-DRERIFF_VERSION=\"$menu_version\"" ExperimentalMenu/test_engine.c -o build/experimental-engine-menu/test-engine.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static
"$zig_tool" cc -target x86-windows-gnu -municode -Wl,--subsystem,windows -O2 -Wall -Wextra -Werror \
  NativeLauncher/test_notification.c -o build/experimental-engine-menu/test-notification.exe -luser32 -static
"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_display_modes.c -o build/experimental-engine-menu/test-display.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_ui_scale.c -o build/experimental-engine-menu/test-ui-scale.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_save_prompt.c -o build/experimental-engine-menu/test-save-prompt.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_antialiasing.c -o build/experimental-engine-menu/test-antialiasing.exe \
  -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -municode -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_flac_sample_rate.c -o build/experimental-engine-menu/test-flac.exe \
  -Wl,--image-base,0x10000000 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_unicode_trace.c -o build/experimental-engine-menu/test-unicode-trace.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_native_archive.c -o build/experimental-engine-menu/test-native-archive.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_cjk_direct.c -o build/experimental-engine-menu/test-cjk-direct.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_cjk_hd_metrics.c -o build/experimental-engine-menu/test-cjk-hd-metrics.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_latin_fonts.c -o build/experimental-engine-menu/test-latin-fonts.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_main_scale.c -o build/experimental-engine-menu/test-main-scale.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static

"$zig_tool" cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_font_components.c -o build/experimental-engine-menu/test-font-components.exe \
  -Wl,--image-base,0x10000000 -luser32 -lgdi32 -ladvapi32 -static
