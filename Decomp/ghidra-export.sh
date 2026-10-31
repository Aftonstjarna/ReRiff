#!/usr/bin/env bash
# Run inside Distrobox development. Reuse the existing native-analysis database.
set -euo pipefail
if (( $# == 0 )); then
  echo 'Usage: bash Decomp/ghidra-export.sh ADDRESS [ADDRESS ...]' >&2
  exit 2
fi
export JAVA_HOME=/var/home/afton/.dev/ghidra_12.2_PUBLIC/jdk-25.0.5.0.2
export PATH="$JAVA_HOME/bin:$PATH"
cd /var/home/afton/.dev/ReRiff
exec /var/home/afton/.dev/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/reriff-ghidra ReRiffNative -process Game.exe -noanalysis \
  -scriptPath /var/home/afton/.dev/ReRiff/Decomp \
  -postScript ExportRecovery.java \
  /var/home/afton/.dev/ReRiff/analysis/ghidra/recovery "$@"
