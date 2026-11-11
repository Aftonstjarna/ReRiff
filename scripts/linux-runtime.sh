#!/bin/sh
# Wine/Steam inject loader variables that must not enter a native AppImage.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
unset LD_PRELOAD LD_LIBRARY_PATH PYTHONHOME PYTHONPATH
exec "$root/ReRiff-Runtime-x86_64.AppImage" "$@"
