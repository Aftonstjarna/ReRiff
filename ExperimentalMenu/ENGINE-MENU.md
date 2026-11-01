# Native Settings > Servers integration

The experimental client hooks the original `gaOptionsMenu` callbacks, adding
Servers before its existing options. It does not patch the main-menu vtable,
change Friends, replace settings entries, or change audio sliders. Main Settings
rows, their arrows and dividers scroll together at the original row interval. New rows use the original `elUIItemText.createRaw`,
font 3, centered alignment, menu-item flags, native colours and engine input.

The entry is a child of the existing Settings group. The server page's ten rows
are owned by the existing screen and use distinct IDs 1501–1510 (entry 1500).
Opening the page saves the original Settings/audio group visibility, hides those
groups, and focuses the first server control through `elUIScreen.setFocus(int)`.
Back or Save and return restores the saved visibility and focuses Servers.
The base `gaMenu.tick` handles Servers and the scrolling root Settings list.
Original root actions still dispatch through `gaOptionsMenu.handleScreenInput`.
The original `gaOptionsMenu.tick` runs on Audio, where its slider hit tests and
drag latches belong; stale slider latches are cleared before root activation. Cleanup removes the added screen rows
before original cleanup so repeated visits cannot accumulate controls or IDs.

Skip launcher, External only and Force Offline use the original fullscreen switch
artwork: sprite 73 is the bar, 74/75 the on/off thumb. The thumb uses the same
right/left origins, bar-width displacement and 25×scale inset established by
ILSpy `gaGraphicsOptions.setTogglebarVars` (0x0600117D). The guarded original
sprite creation, sprite-definition, origin and position routines create/update
them. Their six screen-owned sprites have no input masks or menu IDs; the whole
labelled row remains the only mouse/controller activation target. The original
Settings colour callback is forwarded and its focus/pressed colour also tints
both graphics. Force Offline displays the inverse of the saved
`networking_enabled` value: Off by default, with networking enabled. Existing
preferences retain their meaning. Cleanup removes all six graphics as well as the ten text rows.

Server settings, save/reload, skip-launcher, and Save and restart retain the
existing behavior. No endpoint, DLL, or online Friends behavior changes here.
The module creates no menu windows; navigation reads the same game input device
and held-button masks as the original file picker.

## Scrolling and vanilla spacing

`settings_scroll.h` lays out both lists using the original Settings +264 row
interval and first-row Y. Text uses the same vertical anchor for switches and
ordinary actions. Divider sprite 51 follows the vanilla +half-gap +50×scale
placement and 1650×scale width. Servers gets the original sprite 52 arrow. The scrollbar keeps its viewport
height and right-side position, centered vertically on the screen.
Only complete rows fit in the viewport; all remaining text, switches and lines
are hidden and have zero input masks. Save/reload preserve the viewport.

The original `gaMenuReel` supplies the scrollbar artwork, arrows, dragging and
range limits. Its seven managed methods have no callable native entry points;
`ClientConfig/NativeReelBridge.cs` supplies rooted cdecl delegates that call the
original IL methods. Both sides verify the supported Game.exe hash, and native
hooks verify original vtable/thunk bytes. The addOption adapter supplies the
owning SSP indirection expected by the original method. No game assembly is
shipped. Missing bridge support leaves the original Settings menu untouched.

A reel holds empty native spacer options to establish the original range while
Settings rows retain their existing owners. It uses the game's allocator and
original deleting destructor. Switching between root, Servers and Audio releases
the reel, unregistering the original globally registered scrollbar visuals.
Ten added screen-owned divider lines and group-owned root arrow/dividers retain
original cleanup ownership.

Only wheel and Page/Home/End events are queued by the window procedure. Pointer
button and motion events pass untouched to the game for native dragging. Up=64
and Down=128 use the original input device masks from
`gaFileSelectInterface.tickInput`. One logical selection owns navigation; while
directional input is held, base menu navigation is gated as in that file picker.
Repeat begins at 250ms and accelerates to 20ms, advancing at most once per tick.
Focus is deferred until input dispatch returns, then both screen focus and the
original menu focus bookkeeping are updated. Hidden controls have no input masks. The game recursively assigns the screen
mask after initScreens, so masks are reconciled before dispatch and afterwards,
using the active screen mask rather than a saved initialization mask. Existing
press/release latches are preserved whenever the mask is already correct.

The native harness covers original guards, first-entry ordering, reel lifetime,
held repeat, boundaries, deferred focus, pointer forwarding, wheel, original
screen mask resets, hover/click and controller activation of all four vanilla
entries with stale Audio latches, Audio tick delegation, save/restart
and cleanup. The signed managed smoke test covers pointer/float arguments and
owning SSP indirection through actual cdecl callbacks on Mono and Windows CLR.
Live controller and drag testing is still required.

## Build, install, restore

Close Game.exe before installing/restoring. From this workspace:

```sh
distrobox enter development -- bash /var/home/afton/.dev/ReRiff/ExperimentalMenu/build-engine.sh
distrobox enter development -- python3 /var/home/afton/.dev/ReRiff/ExperimentalMenu/setup_client.py --game-dir '/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer'
```

Setup accepts the supported vanilla DLLs or an existing ReRiff client. It backs
up the exact prior files under the game's **bak** folder, applies the endpoint/
offline patches when missing, adds client-version reporting, and installs the
engine menu and updated helper. An existing endpoint patch is preserved byte for
byte. Unknown unsupported input DLLs are refused. Game.exe, server.ini, launcher
files and saved data are preserved. An interrupted installation rolls back.

Original DLL backups are in `bak/originals`; trial records and exact pre-trial
snapshots are in `bak/experimental-engine-menu`. Older trial records and the
active installer backup history are migrated there with verification. Explicit
backup-directory overrides on the standard installer remain available.

```sh
distrobox enter development -- python3 /var/home/afton/.dev/ReRiff/ExperimentalMenu/setup_client.py --game-dir '/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer' --restore
```

The updated ClientConfig loads the menu through its dedicated startup export,
independently of `show_server_popup`. It still supports ordinary notification
DLLs by tolerating a missing menu export. The optional selected-server toast is
preserved and follows `show_server_popup`; it is separate from the engine menu.
Hook installation occurs during client initialization and
the new label appears when the original Settings menu initializes. If initialization
already happened, return to/recreate Settings. The DLL stays loaded for the
process lifetime. Installation and refusal reasons go to
`ReRiff-engine-menu.log` beside Game.exe; no URLs or account data are logged.

## Evidence and guards

Supported original SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Installation and runtime check the entire original executable hash. Runtime
also validates every Settings vtable entry, the five IJW jump thunks, the base
menu tick, and native text/input/create/add/remove/focus entry bytes before
patching the five aligned Settings callbacks. Game.exe on disk is unchanged.

| Original entry | VA | Metadata token / evidence |
| --- | --- | --- |
| gaOptionsMenu vtable | 0x00BA5A7C | Original table bytes and ILSpy field |
| tick, slot 3 | 0x006F10A4 | fixup 0x00BA5AFC; 0x0600119A |
| initScreens, slot 15 | 0x006EFF50 | fixup 0x00BA5AEC; 0x06001195 |
| cleanup, slot 16 | 0x006F0408 | fixup 0x00BA5AF0; 0x06001196 |
| input, slot 24 | 0x006F08E3 | fixup 0x00BA5AF8; 0x06001199 |
| elUIItemText.createRaw | 0x00450AF0 | 0x0600169C; cdecl, RET |
| elUIItem.add | 0x00443800 | 0x06001699; thiscall, consuming SSP value |
| elUIScreen.add / remove | 0x0048E8D0 / 0x0048E9D0 | 0x0600169F / 0x060016A0 |
| elUIScreen.setFocus(int) | 0x0048F580 | 0x060016FE; thiscall, RET 4 |
| updateItemColour, slot 26 | 0x006EF1E0 | fixup 0x00BA5AE8; 0x06001198 |
| elUIItem.setAsMenuItem | 0x0044B530 | 0x060016AA |
| text raw / localized | 0x00450930 / 0x004508A0 | 0x060016A2 / 0x06001769 |
| elUIItem.enableInput | 0x0044B4A0 | 0x060016D7 |

ILSpy `analysis/native-port/decompiled/Module.cs` establishes Settings state at
+248, main/audio groups +268/+332, original setting text pointers +272 (five),
title +144 and screen +184. Text-item position is +32/+36, extents +48/+52;
visibility bit 0 at +372, enable mask +76, and interaction latch +356. The new
entry is positioned from the original first row and spacing. The submenu
uses the original first-row position, +264 row interval, +292 arrow references
and +312 divider references. A bounded viewport hides overflow rows and disables
their input masks instead of shrinking the gap. No invented settings object
layout is used.

Ghidra read-only exports are retained at `build/settings-evidence`; native
instructions and original metadata signatures resolve Ghidra's incomplete
prototypes. In particular, add/remove consume an SSP value on the stack, not a
pointer to a local SSP as the managed IL signature might suggest. Incrementing
the row reference before these calls matches the original `toItem` convention.
All objects are allocated and released by the original engine.

`enableInput(TRUE)` clears the interaction latch. The hook checks for the
existing all-input mask before calling it, preserving press/release across ticks.

## Validation and remaining trial

The x86 regression harness validates the original executable and Settings
vtable/thunk/native prologues, altered-vtable rejection, and original selections
(including all original setting IDs) being forwarded. It covers added row
ownership, audio-page isolation, focus/back, masks and native enable-input latch
behavior, unrelated INI preservation, save/reload, save failures, restart handoff
failure/cancellation, and removal of all added screen rows at cleanup. The
harness uses original bytes as a read-only image; only the pure native TRUE
branch of enableInput executes from that image. Rendering is not simulated.

Actual Game.exe rendering, CLR-to-native callback transitions, all controller
paths and focus behavior still require a live game trial. Hook initialization
and page-open log entries distinguish loading from successful menu use. A live
failure should be investigated or rolled back; passing the harness is not a
claim that the actual game screen works.

Fresh vanilla and existing modified-client installation/restore tests use the
actual supported DLLs in disposable game directories. They verify exact rollback,
the game-local bak folder, preservation of the existing endpoint DLL, interrupted
write rollback, and a separate backend extension which leaves the song library
and unrelated server code intact.

No matching-reconstruction credit: baseline remains 64,760 / 8,635,392 bytes
(0.749937%). This DLL is an intentional mod, separate from `decomp` recovery.

## Save and Play

The launcher now has separate Save settings and Play actions. Play uses the
settings already saved on disk; it never saves the launcher's unsaved fields.
`--skip` and `--skip-launcher` are aliases for that direct Play path. After a game
closes, launcher controls reload the saved settings so an explicit later Save
cannot inadvertently restore stale values from before the game session.
Local backend startup occurs only for a configured localhost:5144 fallback,
so offline and external-only Play do not depend on a local server. The installed
Linux start script's empty interpreter command was repaired; installer generation
also resolves a native Python fallback when its own executable path is unavailable.

Proton regressions verify atomically saved in-game settings survive this reload,
Play leaves unsaved fields out of the INI, both skip aliases launch a disposable
fake game without changing the INI, and the combined menu DLL's toast appears
without taking focus and dismisses itself. Exact pre-trial backups remain in bak.

## Main-menu branding and update notices

`main_branding.h` guards the original gaMainMenu table at RVA 0x7A7470 and hooks
init/tick/cleanup. The watermark is a screen-owned, right-aligned native text
item with no menu ID, shortcuts or input mask. Its compiled version includes
the numbered experimental revision. Position follows elUI.getResolution
(RVA 0x1FA50), so it stays in the bottom-right corner. Its rendered height is
half the original main-menu font height, derived from the original row interval
at menu +292 (half of 1.5 font heights), rather than scaling a fixed font by the
unrelated menu design scale. Late transport startup
can attach it during the first observed main-menu tick. Cleanup releases its
borrowed reference through the original screen API.

After a completed unattended installation, the temporary native updater writes
`localserver/update-notification.ini` using the installed client-version.txt.
Manual updates and failed installations create no notice. The next main menu
queues a matching-version toast and consumes the marker only if queuing works.
Server and update toasts have independent lifetime gates and stack vertically;
both are non-activating, click-through, control-free and dismiss automatically.
The installer also recognizes the older updater's prepared auto-resume context
and stages the notice in its transaction, so the first upgrade can notify too.
A failure rolls the marker back with the other files. The new native updater
writes the installed version after success as well. Same-version unattended
reinstall exercises notification delivery without a duplicate test release.

## Experimental graphics antialiasing

`graphics_aa_menu.h` adds ID 1601 to the original gaGraphicsOptions screen,
retaining resolution, fullscreen and Apply actions. Three rows are centered
using the measured vanilla row interval. Native fullscreen sprites render the
new switch. Left/right shortcuts belong to the focused row; Apply persists
`[graphics] antialiasing=true/false`, while Back discards pending changes.

The original renderer was cross-checked with the separate RiffRacer recovery
sources and original bytes: wsGraphicsDevice primary vtable RVA 0x5FBF44,
renderer singleton RVA 0x80D408, embedded PC vtable RVA 0x5FC178. The native
getPC adjustment at VA 0x4910D0 returns +0x18. Device creation's +0x78 and
optional Ex +0x7C offsets are relative to that PC subobject, so the complete
wrapper offsets are +0x90 and +0x94. Revision 8 incorrectly read wrapper
+0x78, a vector, and rejected attachment. Tests verify the original adjustment
and constructor bytes, then attach through this layout to a real D3D9 fixture.
Device creation and
presentation evidence is in that recovery's FRAME.md/device_create.S and
application_frame.S; local Ghidra resolution evidence is under build/graphics-evidence.
No new matching reconstruction credit is claimed for these intentional changes.

`antialiasing.h` attaches a private copy of the actual device's COM method table,
chaining Present/Reset and their Ex variants. It creates no competing device and
does not change display modes or multisample settings. With AA enabled, an
edge-directed pixel shader filters a copy of the completed back buffer. GPU
resources, drawing and cleanup execute on the original render thread. A state
block plus explicit render-target/depth/viewport restoration preserve the game's
state; Reset releases default-pool resources and the next frame recreates them.
Disabling AA releases its resources. Shader model 3 and successful allocation
are required; failures retain original presentation, disable AA and log a reason.
This is post-process AA, not a multisample-count setting.

The real software-rendered D3D9 fixture verifies changed/smoothed edge pixels,
state restoration, enabled-AA reset/recreation and Off cleanup. Live gameplay,
physical Windows drivers, Ex reset paths and arbitrary third-party renderer
wrappers remain experimental validation targets.
