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

# Original-file reference

- Use `/var/home/afton/Applications/SteamDepotDownloader/depots/351991/1687655/`
  as the unmodified Windows game reference for file hashes and byte comparisons.
  `packaging/vanilla-reference.json` records its executable/DLL SHA-256 hashes,
  sizes and Steam depot/build/manifest identifiers. Compare working copies and
  backups against this baseline before treating them as original game files.

# In-game release summaries

- For each new release, add a `## In-game summary` section to its release notes,
  followed by `## Full release notes` for the detailed changelog.
- Summarize the most important player-facing changes in at most three complete
  bullet sentences, each at most 72 characters (excluding the bullet prefix).
  Write concise summaries rather than cutting detailed sentences mid-thought.
- Keep the full changelog on GitHub. Native News displays the short summary and
  uses its existing Visit URL action to open that exact release's full notes.

# Release naming and channels

- New prereleases use `X.X.X-beta.N` (starting at beta.1), followed by stable
  `X.X.X`. Remind the user and suggest the beta spelling if they request the
  old `X.X.X-N` style. Keep legacy versions readable for updater compatibility.
- Stable remains GitHub Latest and the default download/update channel. Publish
  betas as prereleases with `--latest=false`; experimental updates require opt-in.
- Do not delete or draft releases automatically. Superseded betas may be removed
  when requested; preserve Git tags for the tested commits.
