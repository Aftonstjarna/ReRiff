#!/bin/sh
set -eu
server_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
unset PYTHONHOME PYTHONPATH
exec /bin/sh "$server_root/linux-runtime.sh" --python "$server_root/server-control.py" stop "$@" >> "$server_root/../logs/ReRiff-localserver.log" 2>&1
