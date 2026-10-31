#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if pkg-config --exists sdl3 2>/dev/null || [[ -f "$root/build/native-deps/SDL3-3.2.0/include/SDL3/SDL.h" ]]; then exit 0; fi
mkdir -p "$root/build/native-deps"
archive="$root/build/native-deps/SDL3-3.2.0.tar.gz"
curl --fail --location --show-error 'https://github.com/libsdl-org/SDL/releases/download/release-3.2.0/SDL3-3.2.0.tar.gz' -o "$archive"
printf '%s  %s\n' bf308f92c5688b1479faf5cfe24af72f3cd4ce08d0c0670d6ce55bc2ec1e9a5e "$archive" | sha256sum --check
tar -xzf "$archive" -C "$root/build/native-deps" --no-same-owner --no-same-permissions 'SDL3-3.2.0/include' 'SDL3-3.2.0/LICENSE.txt'
