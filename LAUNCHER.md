# Native Windows launcher

ReRiff-Launcher.exe lives beside Game.exe and server.ini. It is a native x86
Windows executable, using Win32 controls and system DLLs. It has no CLR header,
.NET dependency, PySide dependency, or Visual C++ redistributable requirement.

Run it under the game's Wine/Proton environment to edit server settings. The local
server starts automatically; Save and play starts Game.exe in the same prefix.
The launcher stays alive while Game.exe runs. Use `--skip-launcher` to start or
reuse the server and run Game.exe without showing the launcher GUI.

## Steam launch options

Use Steam's selected Proton and game prefix by setting launch options to:

```text
"/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/localserver/steam-launcher.sh" -- %command%
```

To skip the settings window:

```text
"/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer/localserver/steam-launcher.sh" --skip-launcher -- %command%
```

The shell bridge replaces only the Game.exe argument with ReRiff-Launcher.exe.
It does not choose another Proton version or create another game prefix. This is
not a Linux GUI launcher. It is glue for Steam's existing launch command.

## Native Linux backend

Under Wine the EXE uses Wine's `start /unix` bridge and path conversion to run
localserver/start-server-host.sh with the native Linux shell. That script starts
the installed Linux Python environment and records console output in
localserver/server-console.log. Windows Python is not needed under Proton.
The launcher reuses a recognized backend on port 5144 and checks readiness before
starting Game.exe. A Windows mutex serializes starts within a Wine prefix.
Keep one game prefix running against the local backend at a time.

On native Windows, a prepared localserver/windows-venv/Scripts/python.exe is used
if available; otherwise Python's `py -3` launcher is tried. Native Windows requires
Python 3 and Flask to be installed; the EXE does not bundle Python.

The server stays running after the game exits. Both INI addresses blank remain
blank and disable the game's Parse networking. Server startup does not alter
those settings. Use local server fills http://127.0.0.1:5144 into fallback;
Save settings or Save and play commits changes. Existing comments and unknown
INI keys are retained by Windows profile APIs.

## Builds and backups

Build with a Zig Windows cross-compiler:

```sh
python3 scripts/build-windows-launcher.py --zig /path/to/zig
```

DLL originals are stored in the ReRiff project's backups/client folder, not beside
the game. Legacy backup files and the retired Linux GUI are retained under
backups/legacy. These folders are excluded from Git. Client DLL rollback uses
scripts/install-client-config.py --restore; optional --backup-dir overrides the
project backup location.

Verified: native executable structure and imports, startup under a disposable Wine
prefix, full EXE-to-native-Python autostart and fake-game launch on an isolated
test port, Steam argument forwarding with spaced paths, and project-backup rollback.
The full graphical flow and Game.exe launch still need live testing in the game's
Steam Proton session. Steam's runtime container can affect access to host Python;
backend startup failures are reported rather than silently launching the game.
