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

# Changelog review before publication

- Stable releases require review of the complete proposed changelog and explicit
  approval of the final wording before publication. Material wording changes
  require renewed approval.
- Beta prereleases do not require changelog approval, as explicitly requested by
  the user on November 18, 2026. Publish only after release checks pass.
- Keep `RELEASE-NOTES.md`, packaged notes and the GitHub release body consistent.

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
- Keep `main` on the latest stable release. Publish stable builds from `main`:
  fast-forward `main` to the tested stable branch, then let the `main` push or
  dispatch run the release workflow. Do not publish a stable from a side branch
  without updating `main`. `main`'s `VERSION` and history must contain the latest
  stable tag; the `Keep main on the latest stable` workflow verifies this.
# Server channel promotion

- Server experiments are developed on `reriff/experimental-server`. A push there
  builds and publishes the `experimental` container image and does not move
  `latest`.
- When a stable client update depends on experimental server changes, promote
  both together: merge or fast-forward the tested stable branch into `main` and
  push. The `main` push publishes the `latest` server image, and the release
  workflow publishes the stable client. Do not promote server experiments to
  `main` ahead of a stable client release without explicit approval.
- After promotion, merge `main` back into `reriff/experimental-server` so the next
  experiment starts from the released server image. Keep
  `reriff/experimental-client` based on `main` for the next client beta.
- `latest` and `main` describe the same released server. `experimental` is the
  optional, movable preview channel. Tags are produced by
  `.github/workflows/container.yml`; do not publish these channels by hand.

# Upstream server restoration after stable releases

- After publishing a stable release, identify changes in that release that restore
  original server functionality. Isolate those changes on the existing
  `Server-Restoration` branch of `Aftonstjarna/ReRiff` for contribution to
  `rumbadebu/riffapi-local-extended`.
- Base the contribution on current upstream code and retain the existing focused
  restoration work. Do not merge the full ReRiff main history into this branch.
  Exclude launcher, installer, client/game patches and unrelated ReRiff features;
  include server dependencies, tests and documentation needed by the restoration.
- Run focused restoration regressions and upstream compatibility checks before
  submitting. Explain protocol/client prerequisites and any new dependencies.
- Split changes that restore different functions or fix unrelated problems into
  separate draft upstream PRs. Group only tightly related changes, their required
  dependencies, documentation and tests. Each PR should explain one concrete
  restoration and its validation.
- Keep `Server-Restoration` as the restoration integration branch. Each separate
  PR needs its own focused branch, such as `Server-Restoration-<topic>`, based on
  upstream so its diff excludes unrelated restoration work. Do not rename or
  delete an existing PR's source branch to create these separate contributions.
- Update an open PR only for changes to that same restoration, retaining its
  existing review state. For a different restoration, or after its previous PR
  has closed or merged, create a separate draft PR. Do not mark a new draft ready
  or merge it without user instruction.
- This standing instruction authorizes pushing the isolated branch and creating
  or updating its upstream PR as part of a stable release; do not ask again.
  Beta-only work waits until it reaches stable. If a release contains no original
  server restoration changes, skip the upstream contribution.

# ReRiff research submissions

- Keep new reverse-engineering notes in ReRiff and submit them with
  `bash scripts/submit-decomp-research.sh reports/NOTE.md`. See `reports/AGENTS.md`.
- Do not write research notes directly into the live decomp checkout. The
  coordinator tracks snapshots, provenance and review status outside that tree;
  direct untracked files stop recovery. Submission grants no matching-byte credit.

# Repair missing ReRiff-managed files

- During installation, update or component enablement, treat missing ReRiff-owned
  files as an incomplete installation, not a fatal ownership mismatch. Restore
  them from a verified bundled payload or redownload the verified component.
- Receipts alone do not prove a component is installed. Check that its required
  files exist, including legacy numbered archives and disabled component folders.
- Do not require missing legacy files to migrate to a freshly downloaded pack.
  Retire surviving legacy files only after verifying their ownership hashes.
- Preserve existing modified or unrelated files, user data and settings. Missing
  original game files require a verified original backup or game-file repair;
  do not substitute unverified downloads. Keep repair transactional and test both
  missing-file recovery and protection of modified surviving files.
