# Release metadata and packaging

Scope: `packaging/`, `VERSION`, `RELEASE-NOTES.md`, release workflows and the
installer payload layout.

## Original-file reference

- Use
  `/var/home/afton/Applications/SteamDepotDownloader/depots/351991/1687655/` as
  the unmodified Windows game reference for file hashes and byte comparisons.
  `packaging/vanilla-reference.json` records its executable/DLL SHA-256 hashes,
  sizes and Steam depot/build/manifest identifiers. Compare working copies and
  backups against this baseline before treating them as original game files.
- Do not substitute an unverified download for a missing original file. Require
  a verified backup or game-file repair.

## Changelog review before publication

- Stable releases require review of the complete proposed changelog and explicit
  approval of the final wording before publication. Material wording changes
  require renewed approval.
- Beta prereleases do not require changelog approval, as explicitly requested by
  the user on November 18, 2026. Publish only after release checks pass.
- Keep `RELEASE-NOTES.md`, packaged notes and the GitHub release body consistent.

## In-game release summaries

- For each new release, add a `## In-game summary` section to its release notes,
  followed by `## Full release notes` for the detailed changelog.
- Summarize the most important player-facing changes in at most three complete
  bullet sentences, each at most 72 characters (excluding the bullet prefix).
  Write concise summaries rather than cutting detailed sentences mid-thought.
- Keep the full changelog on GitHub. Native News displays the short summary and
  uses its existing Visit URL action to open that exact release's full notes.

## Release naming and channels

- New prereleases use `X.X.X-beta.N` (starting at beta.1), followed by stable
  `X.X.X`. Remind the user and suggest the beta spelling if they request the old
  `X.X.X-N` style. Keep legacy versions readable for updater compatibility.
- Stable remains GitHub Latest and the default download/update channel. Publish
  betas as prereleases with `--latest=false`; experimental updates require
  opt-in.
- Do not delete or draft releases automatically. Superseded betas may be removed
  when requested; preserve Git tags for the tested commits.
- Keep `reriff/main` on the latest stable release. Publish stable builds from
  `reriff/main`: fast-forward `reriff/main` to the tested stable branch, then let
  the `reriff/main` push or dispatch run the release workflow. Do not publish a
  stable from a side branch without updating `reriff/main`. `reriff/main`'s
  `VERSION` and history must contain the latest stable tag; the
  `Keep main on the latest stable` workflow verifies this.

## Build and test

The release workflow is the integration gate. Local preflight:

```sh
python -m unittest discover -s packaging -p test_release_metadata.py -v
python scripts/test_client_update.py
python scripts/test_ui_assets.py
```

Run the launcher and client checks that cover the packaged components. Do not
publish a release from an untested commit or a side branch.

## Where documentation goes

- Build steps, repository history and release process: wiki
  [Building and releasing](https://github.com/Aftonstjarna/ReRiff/wiki/Building-and-releasing).
- Installation and update behavior: wiki
  [Installation](https://github.com/Aftonstjarna/ReRiff/wiki/Installation) and
  [Updates](https://github.com/Aftonstjarna/ReRiff/wiki/Updates).
- Original-file layout and restoration: wiki
  [Restoring original files](https://github.com/Aftonstjarna/ReRiff/wiki/Restoring-original-files).

Do not add a release or packaging Markdown file. Update the matching wiki page.
Shared or cross-component changes follow `docs/agents/README.md`.
