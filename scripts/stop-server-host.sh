#!/bin/sh
set -eu
server_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec /bin/sh "$server_root/linux-runtime.sh" stop-servers --root "$server_root" "$@"
