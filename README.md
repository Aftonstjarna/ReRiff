# ReRiff

Riff Racer server restoration and client compatibility tools for Windows and
Linux/Proton. This is an early 0.1.1 release; it does not make the game Linux-native
or remove the game's existing .NET Framework requirement.

## Install

**Version 0.1.1 is an update-only release.** It requires an existing supported
ReRiff installation. It updates ReRiff's launcher/helper/server scripts and leaves
the patched game DLLs unchanged. This public package contains no game-derived
binary patches. A clean, unmodified game installation is not sufficient.

For step-by-step Linux instructions, see [README-LINUX.md](README-LINUX.md).

Close the game and launcher before installing. No administrator/root access is
normally needed when you own the Steam game directory.

**Windows:** extract `ReRiff-0.1.1-windows.zip`, run `ReRiff-Setup.exe`, and select
the game's `Game.exe`. Portable Python and server dependencies are bundled; no
separate Python installation is needed. Keep the ZIP contents together until
installation finishes. This package targets x64 Windows; the launcher itself is
x86 to match the game.

**Linux/Proton:** run the self-extracting installer with Python 3.10+ installed:

```sh
sh ReRiff-0.1.1-linux.run --game-dir '/path/to/steamapps/common/Riff Racer'
```

Omit `--game-dir` to search common Steam libraries. The installer works offline and
bundles the Python server libraries. It installs the backend into `localserver/`
beside Game.exe. It does not change your selected Proton, prefix, or Steam settings.

Both packages verify the supported original DLL hashes and apply binary deltas
locally. Original game DLLs and assets are not included. Existing INI settings,
scores, ghosts, and other server data are preserved. Unknown DLL modifications are
rejected rather than overwritten. Backups go to the ReRiff project's `backups/`
directory when available, otherwise `~/ReRiff/backups/`. Use `--backup-dir` for an
explicit project backup location. `--dry-run` validates without writing anything.

## Launch

To run only the backend in Docker, see [the container instructions](docker/README.md).
The image includes persistent storage and the same detailed tracing as the local
server. Start it with `docker compose up -d --build` from the project root.

On Windows, run `ReRiff-Launcher.exe` beside Game.exe. Add `--skip-launcher` to
start the server and game without the settings window.

On Linux, set Steam launch options to the installed bridge, substituting your path:

```text
"/path/to/Riff Racer/localserver/steam-launcher.sh" -- %command%
```

Add `--skip-launcher` before `--` to skip the settings GUI. The Windows launcher
runs in Steam's existing Proton prefix and bridges to the native Linux Python
server. [Launcher details](LAUNCHER.md).

## Settings and notifications

`server.ini` beside Game.exe controls the preferred external server, local
fallback, timeout, and startup notification. New installs default to the local
server. Existing blank addresses remain blank and disable Parse networking.
The notification is click-through, keeps focus on the game, and fades after six
seconds. Borderless/windowed mode gives the most reliable overlay visibility.
[Configuration details](SERVER-CONFIG.md).

The admin panel is at http://127.0.0.1:5144/admin. The backend binds to localhost.
Detailed redacted traces are in `localserver/server/data/traces/`. Game/player
names remain in logs. [Logging details](LOGGING.md).

## Restore

The 0.1.1 update does not modify original game DLLs and cannot restore them. Use
the original full installer and its backups for the commands below.

Run the Linux installer with `--restore --game-dir '...'`, or on Windows run:

```text
ReRiff-Setup.exe --restore --game-dir "C:\path\to\Riff Racer"
```

This restores original DLLs while retaining the server, INI, and saved data.
Installation state records the backup location. Keep that project backup folder.
Original runtime dependencies and any pre-existing Proton issues still apply.

## Build and release

Source builds use a .NET 8 SDK and Zig 0.14.1 for Windows cross-compilation.
`dotnet build ClientConfig/ClientConfig.csproj -c Release -o build` downloads the
.NET Framework reference assemblies automatically. Original game DLLs are needed
only when regenerating the bridge/platform patches, not for building this
update-only release. See scripts/build-client-config.py and
scripts/build-windows-launcher.py. The helper targets .NET Framework; the
launcher, notification DLL, and Windows setup executable are native Win32.

Build installer packages after building the binaries:

```sh
python3 packaging/build_release.py --update-only --wheels-dir /path/to/pinned-wheels --python-zip /path/to/python-3.13.17-embed-amd64.zip
```

Download the versions pinned in packaging/requirements.txt. Portable Python is
verified against the Python.org SHA-256 embedded in the builder. Releases include
checksums, source archive, commit provenance, and third-party license files.

Updating VERSION on the `launcher` branch triggers the client release workflow.
GitHub builds and tests both installers, then publishes the release and download
assets. Existing release versions are not overwritten.

The vendored extended backend is from rumbadebu/riffapi-local-extended at revision
d6b98d2845bfbc8ee2888f805f8f0dc36bb73ede, based on rebane2001/riffapi-local. Its
Unlicense is retained. Python and bundled libraries retain their own licenses.
ReRiff source is MIT licensed. See RELEASE-NOTES.md for known limitations.
