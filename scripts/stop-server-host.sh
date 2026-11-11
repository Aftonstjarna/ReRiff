#!/bin/sh
set -eu
server_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$server_root/../logs"
exec /bin/sh "$server_root/linux-runtime.sh" stop-servers --root "$server_root" "$@" >> "$server_root/../logs/ReRiff-localserver.log" 2>&1
