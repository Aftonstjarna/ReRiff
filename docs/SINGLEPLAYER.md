# Singleplayer integration

Experimental client branch: `codex/experimental-client`.

The reference project is [moylien/RiffRacer-Offline](https://github.com/moylien/RiffRacer-Offline).
Its relevant changes are local Parse storage, seeded configuration/version,
local login, persistent records and a loopback file listener. It replaces
`Parse.dll`, `Parse.NetFx45.dll`, and the wrapper's assembly references.
ReRiff does not copy those SDK replacements, binaries or signing keys.

ReRiff retains the original Parse SDK and its existing endpoint hook. There is
no suitable original local Parse database: the game expects backend responses
even for track creation. `ClientConfig/Singleplayer.cs` supplies only that missing
storage endpoint, inside the game's process. It implements the requests consumed
by the original client, rather than reimplementing its object model or tasks.
An ephemeral, IPv4 loopback-only listener avoids Windows URL reservations and
fixed-port conflicts. A random per-session URL prefix prevents accidental access
by unrelated local clients. The listener is a background thread, not a service
or child process. No HTTP endpoint is exposed on the LAN.

## Selection and saved settings

`ClientConfig.Select`: primary health probe, optional secondary health probe,
then singleplayer. Primary-only skips the secondary probe. A false legacy
`networking_enabled` setting now means singleplayer without remote probes.
Selections remain pinned for the game session: a failed write never causes
cross-database retry. URLs remain saved when singleplayer is selected.

The launcher no longer starts Python. Its existing local button selects
singleplayer; the server fields become Primary and Secondary. The old stop
button remains solely for shutting down a previously installed backend.
The in-game page keeps the original graphical switches and native scrolling.
Redundant plaintext mode buttons are removed. Singleplayer replaces Force
Offline; Save and exit saves preferences before requesting a normal game close.
Save/Play and restart behavior are retained.

## Data

Records use `singleplayer/classes/<class>/<id>.json`; uploads use
`singleplayer/files/`. Configuration, Version, Profile_Steam and News are seeded
only when absent. Scores, ghosts and other records use original SDK operations.
Writes replace a temporary file atomically. Malformed stored JSON fails visibly
through SDK errors rather than being silently reset. Persisted ParseFile URLs
are rebound when returned to the new session's port and prefix.

First initialization copies legacy `localserver/server/data/classes` and
`files` without moving or changing the originals. Stop a legacy backend before
the initial import for a consistent snapshot. Existing game saves remain under
the original game's control. Shared-server data is not downloaded or merged.
Selecting Singleplayer is necessary if an existing configuration still points
to a running legacy localhost backend.

## Packaging and validation

New client manifests set `singleplayer=true`. Client packages omit the Python
HTTP backend, Flask wheels and server catalog libraries. Windows Python remains
for the existing installer/updater; Linux installation and updating still use
system Python. The server and its dependencies remain in Docker/Podman builds.
Older package manifests retain their installer behavior.

`RoutingSmoke.cs` covers primary precedence, secondary fallback, explicit
singleplayer, preserved URLs, primary-only, missing and invalid configuration.
`SingleplayerSmoke.cs` exercises the actual original Parse SDK: handshake,
login, queries, score updates, batches, files, errors and restart persistence,
plus non-destructive legacy import. The latter needs the user's original
`Parse.dll` and patched `Parse.NetFx45.dll`, which are not distributed.
Native tests cover the existing menu lifecycle and launcher persistence.

This is not a complete general-purpose Parse server. Unsupported operations
return errors. Storage tests on Mono and native menu tests on Wine do not replace
an end-to-end Windows game session covering track generation, racing and ghosts.
