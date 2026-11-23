#!/usr/bin/env bash
# Run inside Distrobox development. Reuse the existing native-analysis database.
set -euo pipefail
if (( $# == 0 )); then
  echo 'Usage: bash Decomp/ghidra-export.sh ADDRESS [ADDRESS ...]' >&2
  exit 2
fi
repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
: "${GHIDRA_HOME:?set GHIDRA_HOME to the Ghidra install}"
: "${GHIDRA_JAVA_HOME:=$GHIDRA_HOME/jdk-25.0.5.0.2}"
export JAVA_HOME="$GHIDRA_JAVA_HOME"
export PATH="$JAVA_HOME/bin:$PATH"
cd "$repo_root"
exec "$GHIDRA_HOME/support/analyzeHeadless" \
  "${GHIDRA_PROJECT:-/tmp/reriff-ghidra}" ReRiffNative -process Game.exe -noanalysis \
  -scriptPath "$repo_root/Decomp" \
  -postScript ExportRecovery.java \
  "$repo_root/analysis/ghidra/recovery" "$@"
