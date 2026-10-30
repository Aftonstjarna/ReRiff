#!/bin/sh
# Called by Wine start /unix. Keep backend diagnostics on the Linux side.
set -eu
server_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec /bin/sh "$server_root/start-server.sh" >> "$server_root/server-console.log" 2>&1
