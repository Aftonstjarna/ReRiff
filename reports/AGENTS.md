# Decomp research handoff

- Keep new ReRiff research notes in this repository. Submit a stable snapshot
  with `bash scripts/submit-decomp-research.sh reports/NOTE.md` from the repo root.
  Use `--list` for review status or `--show ID` for the saved text and provenance.
- Do not copy notes directly into the live RiffRacer checkout: untracked files
  stop its recovery queue. Submission uses the coordinator's separate durable
  inbox and does not modify decomp or grant matching-byte credit.
- Record the original executable hash, addresses or metadata tokens, evidence,
  tested behavior and remaining uncertainty. Distinguish original behavior
  from intentional ReRiff changes. A Git revision is context; the snapshot hash
  identifies the exact submitted note, including uncommitted research.
- Changed contents create a new linked revision; identical contents reuse the
  existing entry. Notes remain pending until reviewed. A coordinator integration
  receipt requires the exact note in a reachable decomp commit. Keep corrections
  alongside the original evidence rather than silently replacing its snapshot.
