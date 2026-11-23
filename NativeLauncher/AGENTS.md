# Launcher, installer and runtime

Scope: `NativeLauncher/`, `LinuxInstaller/`, `LinuxRuntime/`, the Linux patcher
and their shared headers. Client mod sources are in `ExperimentalMenu/` and
`ClientConfig/`.

## Boundaries

- The launcher must remain standalone. Do not add a build dependency on the
  injected client or the server.
- Keep launcher state, update checks and installer ownership separate from game
  settings and user data.
- Preserve the existing restart protocol, logging and configuration headers
  shared with the client. A change there needs the client build checked too.

## Missing ReRiff-managed files

- During installation, update or component enablement, treat missing
  ReRiff-owned files as an incomplete installation, not a fatal ownership
  mismatch. Restore them from a verified bundled payload or redownload the
  verified component.
- Receipts alone do not prove a component is installed. Check that its required
  files exist, including legacy numbered archives and disabled component
  folders.
- Do not require missing legacy files to migrate to a freshly downloaded pack.
  Retire surviving legacy files only after verifying their ownership hashes.
- Preserve existing modified or unrelated files, user data and settings. Missing
  original game files require a verified original backup or game-file repair; do
  not substitute unverified downloads. Keep repair transactional and test both
  missing-file recovery and protection of modified surviving files.

## Build and test

```sh
python scripts/build-windows-launcher.py --zig /path/to/zig --component launcher --output-dir build/launcher
```

Build and run the affected `NativeLauncher/test_*.c` regression. Use
`scripts/test_client_update.py`, `scripts/test-linux-installer.py`,
`scripts/test_native_client.py` and the uninstall tests when their paths move.
The authoritative matrix is `.github/workflows/launcher-checks.yml`.

Test against a separate game installation. Do not install two candidate DLL
sets into the same live game.

## Where documentation goes

- Installation, updates, uninstall and original-file restoration: wiki
  [Installation](https://github.com/Aftonstjarna/ReRiff/wiki/Installation),
  [Updates](https://github.com/Aftonstjarna/ReRiff/wiki/Updates) and
  [Restoring original files](https://github.com/Aftonstjarna/ReRiff/wiki/Restoring-original-files).
- Launcher and in-game settings: wiki
  [Launcher and in-game settings](https://github.com/Aftonstjarna/ReRiff/wiki/Launcher-and-in-game-settings).
- Steam launch behavior: wiki
  [Steam launch options](https://github.com/Aftonstjarna/ReRiff/wiki/Steam-launch-options).
- Troubleshooting: wiki
  [Troubleshooting](https://github.com/Aftonstjarna/ReRiff/wiki/Troubleshooting).

Do not create a feature Markdown file. Update the matching wiki page. Shared or
cross-component work follows `docs/agents/README.md`.
