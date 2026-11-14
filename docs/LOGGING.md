# Diagnostic logging

Launcher → Diagnostics has a master switch and individual log selections.
The master switch disables all output without changing those selections.
Turning it back on resumes only the selected types. Existing log files remain.
Restart a running game or local server after changing its selections.

Preferences are saved under `[diagnostics]` in `client.ini`. The master key is
`logging_enabled`. Individual keys use `log_` followed by: `launcher`, `client`,
`network`, `engine_menu`, `unicode`, `metadata`, `news`, `languages`,
`localserver`, `singleplayer`, `update`, `installer`, or `python`.
The local-server selection includes its helper log. Missing individual keys
retain the previous enabled behavior; the existing stable/beta master defaults
are unchanged. Traces still require their corresponding diagnostic probe.

Logs are written under the game folder's `logs` directory. Disabled writers do
not create their log files. Disabling logging never removes an existing folder
or its contents. Review paths and server addresses before sharing logs.
