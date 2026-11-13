#!/bin/sh
set -eu
client_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
unset LD_PRELOAD LD_LIBRARY_PATH PYTHONHOME PYTHONPATH
if { [ "${1-}" = server-control ] || [ "${1-}" = stop-servers ]; } && "$client_root/ReRiff-ClientHost" --client-host logging-status --root "$client_root/.."; then
    mkdir -p "$client_root/../logs"
    exec >> "$client_root/../logs/ReRiff-localserver-helper.log" 2>&1
    printf 'Linux server helper: %s\n' "${2-status}"
    export APPIMAGE_EXTRACT_AND_RUN=1
fi
exec "$client_root/ReRiff-ClientHost" --client-host "$@"
