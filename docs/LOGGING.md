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
