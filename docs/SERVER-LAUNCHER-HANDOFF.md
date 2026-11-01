# Handoff for the Server and Launcher chat

User requested notification of the singleplayer changes and local test install.
Direct chat messaging tools were unavailable in this session; this is the handoff,
not confirmation that the other chat has received a notification.

## Experimental changes

All work is in the working tree on `codex/experimental-client`; main is untouched.
There are also pre-existing uncommitted native graphics/UI changes. Preserve them;
do not commit or reset the whole working tree indiscriminately.

- `ClientConfig/Singleplayer.cs` is an in-process storage endpoint that retains the
  original Parse SDK. No replacement Parse SDK or third-party binaries were copied.
- Selection is primary (`external_url`) → secondary (`fallback_url`) → singleplayer.
  `external_only=true` skips the secondary. Selection is pinned for the session.
- `networking_enabled=false` now means working singleplayer, without server probes.
  The launcher and native menu retain saved addresses when switching modes.
- Launcher Python-server startup is removed. The old stop control is retained for
  legacy installations. Docker/Podman backend source is unchanged.
- New client packages set `singleplayer=true`, omit the Python backend, Flask and
  server libraries. Windows Python remains for installer/updater compatibility.
- Singleplayer data is under the game's `singleplayer/` directory. The initial
  import copies legacy `localserver/server/data/classes` and `files` unchanged.
- See `docs/SINGLEPLAYER.md` for implementation and validation details.

## Installed for local testing on November 1, 2026

Game directory: `/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer`.

Installed and SHA256-verified:

- `ReRiff.ClientConfig.dll` from `build/singleplayer-tests/`.
- `ReRiff-Launcher.exe` from `build/`.
- `ReRiff.Notify.dll` from `build/experimental-engine-menu/` (latest native menu build).

Backups and hash manifest:
`bak/singleplayer-test-20261101T185352393612Z/` inside the game directory.

Local test configuration sets `networking_enabled=false` and
`auto_update_on_skip=false`, preventing published releases from replacing the
experimental test files. Both server URLs, Skip launcher and graphics settings
were retained. Game.exe and the installed Parse DLLs were not modified.
Game.exe remains SHA256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.

Routing and original-SDK storage tests passed under Mono. Native menu and launcher
persistence tests passed under Wine. Fifteen installer tests passed with synthetic
fixtures. Full Windows gameplay/track-generation testing remains outstanding.
No release was published and no installed game session was launched by this work.
