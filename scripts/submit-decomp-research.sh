#!/usr/bin/env bash
# Compatibility wrapper. Research notes are published to the ReRiff wiki and
# the decomp coordinator receives the wiki snapshot for triage.
set -euo pipefail
reriff_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
exec "$reriff_root/scripts/publish-research.sh" "$@"
