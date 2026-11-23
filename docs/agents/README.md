# Agent guide index

This file routes documentation and validation. The component `AGENTS.md` files
hold implementation-specific rules; the wiki holds user-facing and shared
project documentation.

## Wiki-first documentation

Check the [ReRiff wiki](https://github.com/Aftonstjarna/ReRiff/wiki) before
adding or expanding a repository Markdown file. Do not create a new `.md` for a
feature, a joint project or a user-facing change when an existing wiki page can
hold it. Repository Markdown is reserved for agent instructions, validation
policy and durable internal contracts.

| Topic | Update |
| --- | --- |
| Install, update, uninstall, restore original files | [Installation](https://github.com/Aftonstjarna/ReRiff/wiki/Installation), [Updates](https://github.com/Aftonstjarna/ReRiff/wiki/Updates), [Restoring original files](https://github.com/Aftonstjarna/ReRiff/wiki/Restoring-original-files) |
| Launcher and in-game settings | [Launcher and in-game settings](https://github.com/Aftonstjarna/ReRiff/wiki/Launcher-and-in-game-settings) |
| Graphics, culling, free look, photo mode | [Graphics](https://github.com/Aftonstjarna/ReRiff/wiki/Graphics) |
| Mods, mod loader, asset overrides | [Mods](https://github.com/Aftonstjarna/ReRiff/wiki/Mods), [Mod loader](https://github.com/Aftonstjarna/ReRiff/wiki/Mod-loader) |
| Music search and beta features | [Music search and beta features](https://github.com/Aftonstjarna/ReRiff/wiki/Music-search-and-beta-features) |
| Last.fm, ListenBrainz, Libre.fm | [Last.fm](https://github.com/Aftonstjarna/ReRiff/wiki/Last.fm) |
| Servers, singleplayer, progression transport | [Servers and singleplayer](https://github.com/Aftonstjarna/ReRiff/wiki/Servers-and-singleplayer) |
| Server administration, Docker, LAN | [Server administration](https://github.com/Aftonstjarna/ReRiff/wiki/Server-administration) |
| Leaderboards and ghosts | [Leaderboards and ghosts](https://github.com/Aftonstjarna/ReRiff/wiki/Leaderboards-and-ghosts) |
| Progression and balance | [Progression and balance](https://github.com/Aftonstjarna/ReRiff/wiki/Progression-and-balance) |
| Build, release, repository history | [Building and releasing](https://github.com/Aftonstjarna/ReRiff/wiki/Building-and-releasing) |
| Original-game reconstruction, Linux port, research | [Original game research](https://github.com/Aftonstjarna/ReRiff/wiki/Original-game-research) |
| Proton and runtime tests | [Proton runtime tests](https://github.com/Aftonstjarna/ReRiff/wiki/Proton-runtime-tests) |
| Troubleshooting | [Troubleshooting](https://github.com/Aftonstjarna/ReRiff/wiki/Troubleshooting) |
| Audio analysis and M4A | [Audio analysis and M4A](https://github.com/Aftonstjarna/ReRiff/wiki/Audio-analysis-and-M4A) |

Link to authoritative code, scripts and configuration instead of duplicating
their details. A new page needs a durable purpose beyond recording the current
task. When a page does not exist, add it and update `_Sidebar.md`. Clone the wiki
outside the source tree:

```sh
git clone https://github.com/Aftonstjarna/ReRiff.wiki.git /tmp/reriff-wiki
```

When the task asks for user-facing or joint-project documentation, edit the
matching wiki page instead of adding a repository Markdown file. Push the wiki
change when the task authorizes publishing documentation; otherwise report the
proposed page change. A test run, a build result or a live gameplay caveat
belongs in the task report, not as a new wiki page.

## Joint projects

A joint project affects more than one component. Read both component guides,
keep the change coordinated, and update the matching wiki page instead of adding
a repository Markdown file.

| Area | Components | Wiki page |
| --- | --- | --- |
| `Shared/` | Client and server | The page for the feature being shared |
| `Progression/` | Server balance policy plus `ClientConfig/` and `ExperimentalMenu/` UI | [Progression and balance](https://github.com/Aftonstjarna/ReRiff/wiki/Progression-and-balance) |
| Wire protocol, discovery, checkpoints | Client and server | [Servers and singleplayer](https://github.com/Aftonstjarna/ReRiff/wiki/Servers-and-singleplayer), [Server administration](https://github.com/Aftonstjarna/ReRiff/wiki/Server-administration) |
| Installer ownership, archive and receipt formats | `NativeLauncher/`, `packaging/`, `ClientConfig/` | [Installation](https://github.com/Aftonstjarna/ReRiff/wiki/Installation), [Updates](https://github.com/Aftonstjarna/ReRiff/wiki/Updates) |
| Release and channel changes | `packaging/`, `ServerSync/` | [Building and releasing](https://github.com/Aftonstjarna/ReRiff/wiki/Building-and-releasing) |

Shared-format changes must update every consumer in the same coordinated change.
Do not leave a client-only or server-only half behind.

## Validation matrix

Run the checks that cover the change. CI is authoritative; use the matching
workflow for the full list. For local work, start with the subset below.

| Change | Required checks |
| --- | --- |
| `ExperimentalMenu/` native client | Build with `scripts/build-windows-launcher.py --component client`; compile the relevant `ExperimentalMenu/test_*.c`; run the affected native regression under Wine; run `NativeLauncher/test_lastfm.c` when the change touches shared launcher headers |
| `ClientConfig/`, `Managed/`, `NewsHeaders/`, `Patcher/` | Build with `scripts/build-client-config.py`; build the affected `ClientConfig/Tests/*.csproj` smoke project; run it |
| `NativeLauncher/` UI, installer, updater, runtime | Build with `--component launcher`; build and run the affected `NativeLauncher/test_*.c`; run `scripts/test_client_update.py`, `scripts/test-linux-installer.py` or `scripts/test_native_client.py` when their paths change |
| `packaging/`, release metadata | `python -m unittest discover -s packaging -p test_release_metadata.py -v`; `scripts/test_client_update.py`; the release workflow for publication |
| `ServerSync/`, `docker/`, `vendor/` | The container workflow and the protocol/checkpoint tests; `scripts/test_multiplayer_transport.py` or `scripts/test_progression_discovery.py` when the wire format moves |
| `Shared/` | The client and server checks for every consumer |
| `reports/` | `bash scripts/submit-decomp-research.sh reports/NOTE.md` |
| `.github/workflows/` | The affected workflow's manual dispatch or a pull request run; do not publish from a side branch |

If a required runtime or hardware is genuinely unavailable, say which check
could not run and why. Do not substitute a caveat for a check that could have
been run, and do not describe an inference as a measurement.

## Worktrees and artifacts

- Use one Git worktree per component when work can proceed in parallel. The
  workflow details are in `docs/COMPONENT-WORKFLOWS.md`.
- Keep `build/`, `dist/`, backups, game logs, downloaded tools, profiling dumps
  and local captures out of the tracked tree.
- Check free storage before large builds, captures or model/tool downloads.
- Preserve modified and unrelated files. Investigate unfamiliar files before
  deleting or overwriting them.
- Use a separate test game installation per worktree when testing in parallel;
  do not install two candidate DLL sets into the same game at once.
