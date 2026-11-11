#!/bin/sh
set -eu
unset PYTHONHOME PYTHONPATH
export PYTHONNOUSERSITE=1 PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1
export SSL_CERT_FILE="$APPDIR/share/reriff/ca-certificates.crt"
exec "$APPDIR/bin/python3" -I "$APPDIR/lib/reriff/entry.py" "$@"
