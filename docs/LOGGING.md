# Diagnostic logging

Launcher → Diagnostics has a master switch and nine log selections.
The master switch disables all output without changing those selections.
Restart a running game or local server after changing its selections.

| Selection | File in `logs` | Configuration key |
| --- | --- | --- |
| Launcher | `ReRiff-launcher.log` | `log_launcher` |
| Connections, network and news | `ReRiff-connections.log` | `log_connections` |
| Menus, progression and saves | `ReRiff-game.log` | `log_game` |
| Unicode and song metadata | `ReRiff-text.log` | `log_text` |
| Font installation | `ReRiff-languages.log` | `log_languages` |
| Local server and helper | `ReRiff-localserver*.log` | `log_localserver` |
| Updates | `update.log` | `log_update` |
| Installer | `installer.log` | `log_installer` |
| Python setup | `ReRiff-python-setup.log` | `log_python` |

Preferences are saved under `[diagnostics]` in `client.ini`. The master key is
`logging_enabled`. Grouped records include component labels, such as `[network]`
or `[singleplayer]`, so their origin remains visible. Native and managed writers
serialize appends through the same mutex and use UTF-8 without a BOM.

Older `log_client`, `log_network`, `log_news`, `log_engine_menu`,
`log_singleplayer`, `log_unicode`, and `log_metadata` choices remain effective
until their group is explicitly set. A group checkbox is checked if any of its
legacy components is enabled. Changing that checkbox applies to all components
in the group; the explicit group key takes precedence over legacy choices.
Missing keys retain enabled behavior. Stable/beta master defaults are unchanged.
Unicode and metadata traces still require their corresponding diagnostic probe.

Disabled writers do not create log files. Existing logs are retained. Installation moves the superseded component files
into `logs/archive/before-grouped-*` without converting or deleting their contents. Review paths and server addresses before sharing logs.

## Traces for open diagnostics

These lines are intentionally low-volume and exist to attribute unresolved
issues from a normal session:

| Trace | Source | Purpose |
| --- | --- | --- |
| `Frame gap: N ms` | D3D9 Present (requires antialiasing enabled) | Records any stall of 250 ms or more, capped at 40 per session. Loading screens are recorded too; the cause is not attributed. |
| `FLAC decode: … declared=… decoded=… delta=… elapsed_ms=…` | Native FLAC bridge | Compares STREAMINFO's declared length with the decoded frame count, so the high-sample-rate analysis mismatch is measurable per track. |
| `FLAC analysis: … frames …; declared=… resampled=… delta=… elapsed_ms=…` | Native FLAC resampler | Same comparison after resampling to 44.1 kHz, with resample cost. |
| `Picker index reused: … reason=…` | Managed picker | `partial`, `clock`, `age`, `catalogue-missing`, `catalogue-size` or `catalogue-changed` explains why a full rescan follows. |
| `Picker unreadable: N e.g. …` | Managed picker | Names up to five folders that could not be read during a scan. |
| `Prepared N native mod volumes … in N ms` | Managed mod cache | Separates mod-volume preparation from audio and asset loading. |
| `Ghost car test refused: … (car=…, pack=…)` | Managed ghost test | Identifies the exact car and paint that the test fixture refuses. |
| `Car fallback: … base=… used=… slot=…` | Native fallback | Reports the stock ID that actually resolved, not a comparison of indexes. |
