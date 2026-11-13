#!/usr/bin/env bash
# Submit evidence without writing into the running decomp checkout.
set -euo pipefail
reriff_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
coordinator_root=${RIFF_COORDINATOR_ROOT:-"$reriff_root/../agent-coordinator"}
coordinator="$coordinator_root/build/coordinator"
config="$coordinator_root/build/source-recovery-config.json"
if [[ ! -x "$coordinator" || ! -f "$config" ]]; then
  echo 'Configured recovery coordinator not found; set RIFF_COORDINATOR_ROOT.' >&2
  exit 1
fi
if [[ $# == 1 && $1 == --list ]]; then
  exec "$coordinator" research-list "$config"
elif [[ $# == 2 && $1 == --show ]]; then
  exec "$coordinator" research-show "$config" "$2"
elif [[ $# == 1 && $1 != --* ]]; then
  exec "$coordinator" research-submit "$config" "$reriff_root" "$1"
else
  echo 'Usage: submit-decomp-research.sh RELATIVE_NOTE | --list | --show ID' >&2
  exit 2
fi
