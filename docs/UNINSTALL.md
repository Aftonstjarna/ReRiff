# Uninstalling ReRiff

Open the launcher, select **Diagnostics → Uninstall ReRiff**, and confirm.
Close Riff Racer before uninstalling. After completion, remove ReRiff's command
from the game's Steam launch options so Steam launches the original game.

The uninstaller restores the two modified game DLLs from hash-verified original
backups. It removes recorded ReRiff launcher/helper files and owned font/artwork
mods. It preserves user data in `localserver`, singleplayer saves, INI settings,
logs, backups, and unrelated mods. Installation ownership records for removed
font/artwork components are retired so they do not obstruct a future reinstall.

Removed files are kept under `.bak/uninstall-*` for recovery. Unrecorded files
are left alone. Modified owned files or missing/invalid original backups stop
uninstallation before removal; operational failures roll back moved files.

This requires an installation made with an installer that writes
`localserver/uninstall-state.json`. For older installations, reinstall the
updated package first. The button does not delete server accounts or remote
server data, remove Proton/.NET, or change Steam's launch options automatically.

Developer checks: `scripts/test_uninstall.py --host PATH --vanilla DEPOT_PATH`
uses disposable folders and verified original DLLs. Add `--wine PATH` to exercise
the Windows helper. `--running-launcher PATH` optionally tests removal while a
Windows launcher fixture remains running. No live game files are changed.
