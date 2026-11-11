#!/bin/sh
set -eu
client_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
unset LD_PRELOAD LD_LIBRARY_PATH PYTHONHOME PYTHONPATH
exec "$client_root/ReRiff-ClientHost" --client-host "$@"
