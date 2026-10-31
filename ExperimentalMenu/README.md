# Experimental in-process server menu

**Superseded Windows-panel prototype.** The requested engine-rendered menu is
implemented separately in [ENGINE-MENU.md](ENGINE-MENU.md), built with
`build-engine.sh`. The panel below does not satisfy the in-game menu requirement.

Branch: `codex/experimental-ingame-menu` in ReRiff. Matching EXE recovery stays
in the separate RiffRacer repository on `decomp`.

This experiment replaces only the notification DLL in a test installation. The
existing `ClientConfig.GetServer` P/Invoke starts the menu thread when it shows
the server notification. No Game.exe offsets, native detours, modified managed
assemblies, remote-process injection or launcher changes are required.

Press **F8** with the game focused to open or close Servers. The panel edits
external/fallback URLs and external-only mode. Tab navigates controls; Escape
or Close dismisses the panel. Save stages an INI copy and replaces the original
only after successful writes. Unrelated INI keys are retained. Blank addresses
remain blank. Endpoint selection changes only on the next game start.

This is a Win32 panel owned by the game window and running in the game process,
not a recovered `gaMenu` screen or Direct3D-rendered overlay. It does not pause
simulation, intercept the game's raw/controller input, implement controller
navigation, probe servers, start/stop a backend, or change an active session.
Use from the front end in windowed/borderless mode. Exclusive-fullscreen
visibility and actual Game.exe/Steam-Proton integration require a live test.

## Build

From the project workspace:

```sh
distrobox enter development -- bash /var/home/afton/.dev/ReRiff/ExperimentalMenu/build.sh
```

Pass a Zig binary path as the script's first argument if the local cached tool
is unavailable. Outputs are isolated in `build/experimental-menu`:

- `ReRiff.Notify.dll`: x86 Windows DLL, compatible notification export retained.
- `test-menu.exe`: settings/validation test in a fake game process.
- `test-load.exe`: dynamically loads the actual DLL and exercises F8.

Run both tests with Proton Wine under a virtual display and a disposable Wine
prefix, through the development container's `distrobox-host-exec` bridge.
Never run `test-menu.exe` from a directory containing real server settings:
it creates and deletes `server.ini` beside its executable.

Verified with installed Proton Hotfix under Xvfb in a disposable prefix:

- Both x86 builds pass `-Wall -Wextra -Werror` inside development.
- Settings test passes URL validation, duplicate startup, control creation,
  saving, unrelated-key preservation, invalid-input rejection, blank addresses
  and close behavior.
- Dynamic-load test passes loading the actual DLL through
  `ReRiffShowServerToastW` and simulated F8 open/close.
- PE inspection confirms an Intel i386 DLL and the expected undecorated export.

Proton 9.0 first-run setup stalled before the test programs started; Hotfix
completed both tests. These fake-window tests do not establish actual game
focus/input behavior, exclusive-fullscreen compatibility or successful backend
authentication. The installed game was not changed or launched by this work.

## Manual game trial and rollback

The build script does not install anything. Close the game first. Back up the
currently installed `ReRiff.Notify.dll` and `server.ini` outside the game folder,
then copy the experimental DLL beside Game.exe. Keep the existing patched
Parse DLLs and ClientConfig DLL. `show_server_popup` must be enabled because
that is the existing load trigger. Start normally, reach the front end, and
press F8. Failure to initialize Parse or load the toast prevents this entry
point from running; the experiment has no independent startup hook.

Check opening/closing, typing, Tab/Escape, saving, returning focus to the game,
and settings persistence after restart. Do not infer successful authentication
from saved settings. To roll back, close the game and restore the backed-up
notification DLL and, if settings were edited, the backed-up INI.

## Next recovery target

Trace original `gaMainMenu.initScreens` and `handleScreenInput` into `gaMenu`
screen construction, focus and navigation. This panel establishes a separate
in-process loading experiment; it does not establish a compatible ABI for
constructing original engine menu objects. Such a hook needs original-byte and
calling-convention evidence before using recovered pointers at runtime.

No reconstruction byte credit is claimed. Recorded baseline at experiment
creation: 64,760 / 8,635,392 bytes (0.749937%), source-built fragments only.
