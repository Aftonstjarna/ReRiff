# ReRiff

Riff Racer server restoration and client compatibility tools for Windows and
Linux/Proton. ReRiff provides a local or shared server, a native Windows launcher,
and client configuration tools. The game still runs through Proton on Linux and
retains its existing .NET Framework requirements.

This fork is the primary ReRiff repository and contains the server, launcher,
client tools, and installers together. The previous
[Aftonstjarna/ReRiff-test repository](https://github.com/Aftonstjarna/ReRiff-test) is
retained for legacy/test use and history. Current releases, instructions and
Docker publishing are maintained here.

[Download the latest release](https://github.com/Aftonstjarna/ReRiff/releases/latest)
· [Install and update](#install-or-update)
· [Steam launch options](#launch-through-steam)
· [Docker server](#docker-server)
· [Troubleshooting](#troubleshooting)

## Install or update

**Public client releases are update-only.** An existing supported ReRiff
installation is required; these downloads do not patch a clean game installation.
They verify the supported patched DLL hashes and update ReRiff's own components.
Original game executables, assets, and game-derived DLL patches are not included.

Close the game and launcher before a manual installation. Existing settings,
scores, ghosts, and server data are preserved. Backups stay outside the game
folder, using the installation's recorded backup location. Unknown game DLL
modifications are rejected rather than overwritten.

**Windows:** download and extract `ReRiff-<version>-windows.zip`, run
`ReRiff-Setup.exe`, and select the game's `Game.exe`. Keep the ZIP contents
together until setup finishes. Portable Python and server dependencies are
included. The package targets x64 Windows; the launcher is native x86 Win32.

**Linux/Proton (x86_64):** install Python 3.10 or newer, then run:

```sh
sh ReRiff-<version>-linux.run --game-dir '/path/to/steamapps/common/Riff Racer'
```

Replace `<version>` with the downloaded release number. Omit `--game-dir` to
search common Steam libraries. Do not use `sudo`. The installer bundles the
server libraries and places the backend in `localserver/` beside `Game.exe`.
It does not change Steam's selected Proton version or launch options.

### Automatic client updates

Click **Update from GitHub** while the game is closed. The launcher downloads
the latest stable release for your platform, verifies its SHA256 and package
contents, applies the update with backups and rollback, then reopens itself.
The title bar shows the running version. Settings and saved data are retained.

Install v0.1.5 or newer manually if an older launcher only opens the release
page or fails with "Could not stop this installation's local server." Subsequent
updates use the corrected updater. Client updates do not update the Docker server.

## Launch through Steam

On Windows, run `ReRiff-Launcher.exe` beside `Game.exe`. Add `--skip-launcher`
to start the game without the settings window.

On Linux, set Steam launch options to the installed bridge, using your game path:

```text
"/path/to/Riff Racer/localserver/steam-launcher.sh" -- %command%
```

To skip the launcher window:

```text
"/path/to/Riff Racer/localserver/steam-launcher.sh" --skip-launcher -- %command%
```

The Windows launcher runs in Steam's Proton prefix and starts or reuses the
native Linux Python server when local operation is enabled. Keep your existing
working Proton and .NET setup. In Steam, find the game path with **Properties →
Installed Files → Browse**, then edit **Properties → General → Launch Options**.
Do not put the Windows EXE directly before `%command%` on Linux; use the bridge
above. The launcher waits for the game to exit so Steam keeps the session active.

## Server settings

The launcher edits `server.ini` beside `Game.exe`:

- **External server:** the preferred shared server address, including its port.
- **Fallback server:** normally `http://127.0.0.1:5144` for local play.
- **External server only:** disables local autostart and fallback. An unavailable
  external server leaves game networking disabled.
- **Use local server:** selects local operation.
- **Stop local server:** stops only this installation's Python backend. An
  already stopped server succeeds; Docker and unrelated services are untouched.

Both addresses blank disables Parse networking. Server selection is fixed for
the game session, so restart the game after changing settings. The optional
server notification fades automatically and does not take controller focus.
Borderless/windowed mode gives the most reliable overlay visibility.

Example client configuration:

```ini
[server]
external_url = http://your-server:5144
fallback_url = http://127.0.0.1:5144
timeout_seconds = 5
show_server_popup = true
external_only = false
```

The client probes the external `/classes/Version` endpoint at initialization and
uses the fallback if that probe fails, unless external-only mode is enabled.
A failed upload does not switch databases or retry on another server. Local and
remote databases are separate and are not automatically synchronized. Missing,
unreadable, or invalid configuration disables Parse networking; there is no
hidden localhost destination. URL credentials, query strings and fragments are
not accepted. Unrelated Steam networking remains enabled.

On native Windows the bundled portable Python runs the local backend. On Proton
the launcher bridges to native Linux Python. The backend stays running after the
game closes; use **Stop local server** when needed. Keep one local backend per
installation/port. Automatic local startup expects port 5144.

## Shared leaderboards and ghosts

Use the same server on each client and update both the clients and server.
The launcher registers cached song IDs with the source audio file's SHA256
when the game starts and after it closes. The server links IDs for identical
files, so existing scores, friend leaderboards, ranking counts, and ghosts can
be shared even when clients originally cached different song IDs.

Source audio files must remain accessible. Matching title/artist metadata alone
does not establish identity; different encodings or edited file tags can produce
different hashes. New imports register after the game closes. The server
preserves existing records rather than deleting or rewriting history.

The registry is stored in the server's persistent `song-catalog.json`. Registration
sends song IDs, file hashes, title and artist; it does not send audio bytes or
local file paths. A cached ID already registered to a different hash is rejected.
Duplicate player records rank the best score once per song/mode/version while
keeping the underlying history. Missing historical track assets are not recreated
from a score or cache entry.

For verified legacy repairs, the optional `song-aliases.json` contains equivalence
groups such as `[["first-cached-id", "second-cached-id"]]`. Automatic registration
removes the need to maintain those groups. Never join IDs solely by song title.

## Docker server

The published image is `ghcr.io/aftonstjarna/reriff:latest`.
If an existing deployment uses `ghcr.io/aftonstjarna/riffapi-local-extended`,
change its image to `ghcr.io/aftonstjarna/reriff:latest` while retaining the
existing environment and data volume or bind mount. Existing `reriff` deployments
can continue using the same image address. From this repository,
use the supplied Compose configuration:

```sh
export RERIFF_IMAGE=ghcr.io/aftonstjarna/reriff:latest
docker compose pull server
docker compose up -d --no-build server
```

The default port mapping is local-only. For LAN access, configure
`RERIFF_BIND_ADDRESS=0.0.0.0`, set `RERIFF_PUBLIC_URL` to the server's address
including port 5144, and allow TCP 5144 through the server firewall. Configure
each client's external URL to that address. The admin API has no hardened access
control, so use a trusted network.

The persistent `/data` volume contains records, uploaded files, the song catalog,
and request traces. Keep that volume when updating or recreating the container.
A restart alone does not install a newly pulled image; recreate the container
with its existing settings and data mount. Avoid `docker compose down -v` unless
you intend to delete saved data.

### LAN configuration with Compose

Put persistent settings in a `.env` file beside `compose.yaml`:

```ini
RERIFF_IMAGE=ghcr.io/aftonstjarna/reriff:latest
RERIFF_BIND_ADDRESS=0.0.0.0
RERIFF_PORT=5144
RERIFF_PUBLIC_URL=http://your-server:5144
```

Replace `your-server` with the server's reachable IP or hostname. Then run
`docker compose pull server` and `docker compose up -d --no-build server`.
`RERIFF_PUBLIC_URL` controls generated download URLs; it does not open the
firewall, configure TLS, or change the host bind address.

### Host networking and a host data folder

On a Linux Docker host:

```sh
mkdir -p "$HOME/Applications/docker/ReRiff-Server/data"
touch "$HOME/Applications/docker/ReRiff-Server/server.env"
```

Set `RERIFF_PUBLIC_URL=http://your-server:5144` in `server.env`, then run:

```sh
docker run -d --name reriff-server --restart unless-stopped \
  --network host --user "$(id -u):$(id -g)" \
  --env-file "$HOME/Applications/docker/ReRiff-Server/server.env" \
  --mount "type=bind,source=$HOME/Applications/docker/ReRiff-Server/data,target=/data" \
  ghcr.io/aftonstjarna/reriff:latest
```

Do not add `-p` with host networking. The user override makes the container write
as the owner of the host data folder. Without an override, the image runs as
UID/GID 10001; bind-mounted data must be writable by that identity.

To update a `docker run` deployment, pull the new image and recreate the container
using the same network, environment, and data mount. Keep the host data directory.

### Build locally, migrate data, and verify

```sh
docker build -t reriff-server:local .
python3 docker/smoke_test.py --image reriff-server:local
```

The smoke test uses a disposable volume and port. It checks health, admin access,
persistent scores/ghosts, automatic identity mapping, tracing and file URLs.
For Podman, use `--engine podman --image localhost/reriff-server:local` after
building with Podman. Published builds currently target Linux amd64.

To migrate the game's local database into a Compose volume, stop its backend
first and back up the source data, then run:

```sh
docker compose create server
docker compose cp "/path/to/Riff Racer/localserver/server/data/." server:/data/
docker compose run --rm --no-deps --user 0 --entrypoint sh server -c 'chown -R 10001:10001 /data'
docker compose up -d --no-build server
```

Keep the previous client-visible server address where possible; stored file URLs
can still refer to it. The image contains server code and dependencies, not game
binaries or assets. Run one backend process against each data directory.

GitHub's container workflow builds and tests pushes and pull requests. The default
branch publishes `latest` and a commit-specific tag to GHCR; version-tag builds
can publish version tags. GitHub does not deploy or restart your server.

## Admin panel and logs

For a local server, open `http://127.0.0.1:5144/admin`. For a shared server, use
its configured address with `/admin`.

- Client selection: `ReRiff-client.log` beside `Game.exe`.
- Launcher and updates: `ReRiff-launcher.log`; update errors also identify a
  temporary folder containing `update.log`.
- Local backend: `localserver/server-console.log` and
  `localserver/server/data/traces/`.
- Docker: `docker logs reriff-server` and `/data/traces/` inside the container.

Credentials are redacted from structured traces; player and song names remain.

To export Docker diagnostics on the server host:

```sh
docker logs --timestamps reriff-server > reriff-console.log 2>&1
docker cp reriff-server:/data/traces ./reriff-traces
tar -czf reriff-logs.tar.gz reriff-console.log reriff-traces
```

Transfer that archive from the server host if investigation happens on another
machine. No extra logging port is needed. Watch live output with
`docker logs --follow --tail 100 reriff-server` (or `docker compose logs -f server`).

Each server session gets a JSONL trace with request IDs, timestamps, routes,
queries, headers, JSON bodies, response status and duration. Binary content is
represented by size and SHA256. Investigation flags are clues, not proof of a
missing feature; an empty leaderboard can be normal. Local logs cannot show
requests sent directly to another server. Archive old traces as needed.

For a local session summary:

```sh
python3 "/path/to/Riff Racer/localserver/trace-server.py" --report /path/to/session.jsonl
```

## Restore original DLLs

Update-only releases do not modify or restore the game's patched DLLs. Use the
original full installer and its backups with `--restore --game-dir '...'` to
restore original DLLs while retaining the server, settings, and saved data.
Keep the recorded backup folder. Original game runtime dependencies still apply.

With a full installer capable of restoring original DLLs:

```sh
sh ReRiff-<version>-linux.run --restore --game-dir '/path/to/Riff Racer'
```

On Windows:

```text
ReRiff-Setup.exe --restore --game-dir "C:\path\to\Riff Racer"
```

Current update-only packages reject restore mode. `--dry-run` validates a manual
installation without writing files; `--backup-dir` selects an explicit backup
location. Existing installations continue using their recorded backup directory.

## Troubleshooting

- **Game folder not found:** pass `--game-dir` with the folder containing
  `Game.exe`; use Steam's **Browse** button to locate it.
- **Launcher does not open under Proton:** check the quoted bridge path and
  retain `-- %command%`. Use the game's existing working Proton configuration.
- **Old updater reports a stop failure:** install a current release manually
  once. Updated launchers distinguish actual host-helper completion from Wine's
  wrapper exit status, including when no local backend is running.
- **Cannot connect over LAN:** check the saved external URL and port, container
  port mapping/host networking, TCP firewall rule, and `ReRiff-client.log`. A
  public-URL setting alone does not expose a port. Restart the game to reselect.
- **Same song has separate leaderboards:** update the server and both launchers,
  ensure the exact source files are accessible, and relaunch each client. Metadata
  alone cannot recover unknown cached identities.
- **Server will not start:** check `localserver/server-console.log` and
  `ReRiff-launcher.log`; another service may already occupy port 5144.
- **Update fails:** inspect the temporary `update.log` reported by the launcher.
  Download or preflight failure keeps the old launcher open; installation errors
  retain or roll back files. Keep the game's original runtime dependencies.
- **Notification hidden in fullscreen:** use borderless/windowed mode. The
  notification fades automatically and requires no input.

## Standalone source server

The compatibility command remains available:

```sh
python3 -m pip install Flask
python3 riffapi-local.py
```

It runs the same server implementation used by Docker and installed clients,
with tracing enabled, binds to localhost, and uses `data/` beside this root script.
`RERIFF_DATA_DIR` (or the legacy `RIFFAPI_DATA_DIR`) can select another data folder.
Source-based song registration also needs the built native catalog library; set
`RERIFF_CATALOG_LIBRARY` to its path in `build/server-sync/`. Docker and the
installers include that library automatically.

## Build and release

Client source builds use .NET SDK 8 and Zig 0.14.1:

```sh
dotnet build ClientConfig/ClientConfig.csproj -c Release -o build
python3 scripts/build-windows-launcher.py --zig /path/to/zig
python3 scripts/build-song-catalog.py --zig /path/to/zig
```

The launcher, updater, notification DLL, and Windows setup are native Win32.
The server's identity engine is C++; the existing server and installer tools
use Python. The managed client helper retains its .NET Framework target.

For installer builds, obtain the pinned wheels in `packaging/requirements.txt`
and the verified Windows portable Python archive, then run:

```sh
python3 packaging/build_release.py --update-only --wheels-dir /path/to/wheels --python-zip /path/to/python.zip
```

Updating `VERSION` on `main` or `launcher` triggers GitHub's client release
workflow. It builds and checks both installers, then publishes assets and
checksums. Each release contains only its own notes; the full changelog remains
in [RELEASE-NOTES.md](RELEASE-NOTES.md). Existing release versions are not replaced.

The vendored extended backend originates from
[rumbadebu/riffapi-local-extended](https://github.com/rumbadebu/riffapi-local-extended)
at revision `d6b98d2845bfbc8ee2888f805f8f0dc36bb73ede`, based on
rebane2001/riffapi-local. Its Unlicense is retained. ReRiff source is MIT licensed;
bundled dependencies retain their licenses.
