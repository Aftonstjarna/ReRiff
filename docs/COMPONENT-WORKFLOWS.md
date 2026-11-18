# Parallel component development

Launcher, client mod and server changes can be developed on separate branches
and in separate Git worktrees of this repository. Give each chat its own
worktree; switching branches inside one shared checkout does not isolate edits,
build files or the Git index. A worktree per component also keeps its `build/`
outputs separate. Commit or transfer the intended changes before creating a
worktree: uncommitted files are not copied into a new one.

| Component | Main sources | Independent workflow |
| --- | --- | --- |
| Launcher and installation | `NativeLauncher/`, `LinuxInstaller/`, `LinuxRuntime/`, `packaging/` | `launcher-checks.yml` |
| Client mod | `ClientConfig/`, `ExperimentalMenu/`, `NewsHeaders/` | `client-checks.yml` |
| Server | `docker/`, `vendor/riffapi-local-extended/`, `ServerSync/`, `Progression/` | `container.yml` |

Launcher and client checks build independently, then run their selected native
regressions on Windows. Server container integration checks use their existing
workflow. Path filters avoid rebuilding the server for a UI-only edit. Shared
inputs intentionally trigger more than one workflow. Manual dispatch is
available for each workflow; these checks do not publish client releases.
Release packaging also calls both component checks as required parallel gates.

For example, from a clean integration checkout, create separate worktrees:

```sh
git worktree add ../ReRiff-launcher -b codex/launcher-work HEAD
git worktree add ../ReRiff-client -b codex/client-work HEAD
git worktree add ../ReRiff-server -b codex/server-work HEAD
```

Use an existing component branch instead when one already holds the work.
Keep changes within the component's sources where possible. Coordinate shared
configuration, protocol changes, patcher interfaces and packaging changes before
merging. `Shared/` affects both native/managed integrations; `Progression/`
contains server balance policy, while the client implementation lives under
`ClientConfig/` and `ExperimentalMenu/`.

Independent native builds accept separate output directories:

```sh
python scripts/build-windows-launcher.py --zig /path/to/zig --component launcher --output-dir build/launcher
python scripts/build-windows-launcher.py --zig /path/to/zig --component client --output-dir build/client
```

The default `--component all` preserves the release build interface. The release
workflow remains the integration gate: installers combine tested launcher,
client helpers and optional server packages from one commit. Component CI is
not a substitute for installer, compatibility and live gameplay validation.
Only the integration/release owner should update `VERSION`, release notes and
publish a release. Independent work does not require separate public versions.

Use a separate test game installation per worktree when testing in parallel.
All worktrees otherwise point at the same real game and save files; do not
install two candidate DLL sets into that game at once. Prestige testing must
retain its existing explicit confirmation and backed-up isolated save.
