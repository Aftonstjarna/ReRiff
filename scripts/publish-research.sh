#!/usr/bin/env bash
# Publish a research note to the ReRiff wiki and submit the wiki snapshot to
# the decomp coordinator for triage.
set -euo pipefail
reriff_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
wiki_dir=${RERIFF_WIKI_DIR:-"$reriff_root/../.reriff-wiki"}
coordinator_root=${RIFF_COORDINATOR_ROOT:-"$reriff_root/../agent-coordinator"}
coordinator="$coordinator_root/build/coordinator"
config="$coordinator_root/build/source-recovery-config.json"

usage() {
  cat >&2 <<'EOF'
Usage: publish-research.sh NOTE.md [SLUG] [--category NAME] [--dry-run] [--no-submit]
       publish-research.sh --list
       publish-research.sh --show ID
EOF
  exit 2
}

if [[ $# == 1 && $1 == --list ]]; then
  [[ -x "$coordinator" && -f "$config" ]] || { echo 'Recovery coordinator not found; set RIFF_COORDINATOR_ROOT.' >&2; exit 1; }
  exec "$coordinator" research-list "$config"
fi
if [[ $# == 2 && $1 == --show ]]; then
  [[ -x "$coordinator" && -f "$config" ]] || { echo 'Recovery coordinator not found; set RIFF_COORDINATOR_ROOT.' >&2; exit 1; }
  exec "$coordinator" research-show "$config" "$2"
fi

note=
slug=
category="New notes"
dry_run=0
submit=1
while [[ $# -gt 0 ]]; do
  case "$1" in
    --category) [[ $# -ge 2 ]] || usage; category=$2; shift 2 ;;
    --dry-run) dry_run=1; shift ;;
    --no-submit) submit=0; shift ;;
    --*) usage ;;
    *)
      if [[ -z "$note" ]]; then note=$1
      elif [[ -z "$slug" ]]; then slug=$1
      else usage
      fi
      shift ;;
  esac
done
[[ -n "$note" && -f "$note" ]] || usage
slug=${slug:-$(basename "$note" .md)}
[[ "$slug" =~ ^[A-Za-z0-9._-]+$ ]] || { echo 'Slug must use letters, digits, dot, underscore or hyphen.' >&2; exit 2; }
page="Research-$slug"

if [[ ! -d "$wiki_dir/.git" ]]; then
  git clone https://github.com/Aftonstjarna/ReRiff.wiki.git "$wiki_dir"
else
  git -C "$wiki_dir" pull --ff-only
fi

title=$(sed -n 's/^#  *//p' "$note" | head -1)
[[ -n "$title" ]] || title=$slug
{
  printf '> [Research notes](Research) · %s\n>\n' "$category"
  printf '> Repository paths refer to `Aftonstjarna/ReRiff` at `reriff/main`.\n\n'
  cat "$note"
} > "$wiki_dir/$page.md"

# Index the note under a single New notes section.
python3 - "$wiki_dir/Research.md" "$title" "$page" <<'PY'
import sys
from pathlib import Path
hub, title, page = Path(sys.argv[1]), sys.argv[2], sys.argv[3]
text = hub.read_text() if hub.exists() else '# Research notes\n'
link = f'- [{title}]({page})'
if f'({page})' in text:
    raise SystemExit(0)
heading = '## New notes'
if heading not in text:
    text = text.rstrip() + f'\n\n{heading}\n'
before, after = text.split(heading, 1)
hub.write_text(before + heading + '\n\n' + link + '\n' + after.lstrip('\n'))
PY

git -C "$wiki_dir" add "$page.md" Research.md
if git -C "$wiki_dir" diff --cached --quiet; then
  echo "Research note is unchanged."
else
  git -C "$wiki_dir" commit -m "Add research note: $slug"
  if (( dry_run )); then
    echo "Dry run: committed locally in $wiki_dir; not pushed."
  else
    git -C "$wiki_dir" push origin master
    echo "Published https://github.com/Aftonstjarna/ReRiff/wiki/$page"
  fi
fi

if (( submit )) && [[ -x "$coordinator" && -f "$config" ]]; then
  "$coordinator" research-submit "$config" "$wiki_dir" "$page.md"
fi
