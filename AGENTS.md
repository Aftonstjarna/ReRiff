# Extending the original game

- Prefer the game's existing native solution. Inspect its original functions,
  controls and behavior before adding a replacement implementation.
- Copy, reuse or extend a suitable original game function instead of creating
  a separate system for the same job. Preserve its native input handling,
  navigation, repeat behavior, artwork and lifecycle where applicable.
- Add a new implementation only when no suitable original solution exists;
  document that gap and keep the integration consistent with the game.
- For every new menu or menu addition, retain native spacing and control
  styling. Check the available space at supported resolutions and UI scales;
  reuse the native scrolling solution when entries overflow. Do not compress
  entries or allow controls to crowd, overlap, or become unreachable.
- Consult the RiffRacerDecomp recovery project for reverse-engineering work and
  contribute verified new findings there. Preserve original hashes, exact bytes,
  metadata tokens and provenance; keep intentional ReRiff patches separate from
  byte-for-byte original reconstruction and do not claim matching without comparison.

# ReRiff research submissions

- Keep new reverse-engineering notes in ReRiff and submit them with
  `bash scripts/submit-decomp-research.sh reports/NOTE.md`. See `reports/AGENTS.md`.
- Do not write research notes directly into the live decomp checkout. The
  coordinator tracks snapshots, provenance and review status outside that tree;
  direct untracked files stop recovery. Submission grants no matching-byte credit.
