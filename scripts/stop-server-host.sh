#!/bin/sh
set -eu
server_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
unset PYTHONHOME PYTHONPATH
exec python3 "$server_root/server-control.py" stop "$@" >> "$server_root/server-console.log" 2>&1
