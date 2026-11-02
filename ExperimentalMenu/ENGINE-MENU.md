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

Skip launcher, Primary server only and Singleplayer use the original fullscreen switch
artwork: sprite 73 is the bar, 74/75 the on/off thumb. The thumb uses the same
right/left origins, bar-width displacement and 25×scale inset established by
ILSpy `gaGraphicsOptions.setTogglebarVars` (0x0600117D). The guarded original
sprite creation, sprite-definition, origin and position routines create/update
them. Their six screen-owned sprites have no input masks or menu IDs; the whole
labelled row remains the only mouse/controller activation target. The original
Settings colour callback is forwarded and its focus/pressed colour also tints
both graphics. Singleplayer displays the inverse of the saved
`networking_enabled` value: Off by default, with networking enabled. Existing
disabled-networking preferences now select working singleplayer. Cleanup removes all six graphics as well as the ten text rows.

Server settings, save/reload, skip-launcher, and Save and restart retain the
existing behavior. Friends retains its original behavior; endpoint selection is handled by ClientConfig.
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
`gaFileSelectInterface.tickInput` for the Settings root. One logical selection
owns root navigation; while directional input is held, base menu navigation
is gated as in that file picker.

The Servers page uses the original native direction handler (elUIItem slot 28,
VA 0x448E70, dispatching mode-1 ID links through 0x44AA60). All ten rows form a
closed Up/Down loop. A private copy of the 48-method elUIItemText table retains
its RTTI locator and chains the original direction method after revealing its
target with the existing native reel/row-layout adapter. This is necessary
because the native handler rejects hidden or input-disabled targets. It does
not select a row itself or implement repeat: the original screen owns focus,
keyboard/controller input and held-key repeat. Custom direction polling is
bypassed on this page. Cleanup restores the original method tables before
removing rows. Tests cover three loops each way, wrap visibility, native dispatch,
no competing polling and cleanup restoration.
Root repeat begins at 250ms and accelerates to 20ms, advancing at most once per tick.
Root focus is deferred until input dispatch returns, then both screen focus and the
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
`logs/ReRiff-engine-menu.log` under the game folder; no URLs or account data are logged.

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
retaining resolution, fullscreen and Apply actions. Four rows are centered
using the measured vanilla row interval. Native fullscreen sprites render the
new switch. Left/right shortcuts belong to the focused row; Apply persists
`[graphics] antialiasing=true/false`, while Back discards pending changes.
The original native direction handler (VA 0x44AA60) uses mode 1 at item +0x98
and Up/Down target IDs at +0x9C/+0xA0. Extend vanilla's two-row loop to
Resolution → Fullscreen (ID 4) → Borderless (ID 1602) → AA → Resolution. Both resolution arrows wrap
Up to AA; AA wraps Down to the original left resolution arrow (ID 1). Apply
(ID 3) retains its activation and has Up to AA and Down to Resolution when
mouse-focused. Restore all four modified controls' navigation fields on cleanup.
Keyboard/controller directions and
held-key repeat continue through the original screen handler; there is no
competing custom direction poller. Ghidra direction/focus exports are in
build/graphics-evidence, checked against original instruction bytes.

The original resolution text is passive. Extend its hover area with the original
`elUI.createBlankButton` (RVA 0x222F0, ID 1604), centered between the arrow controls.
It participates in native hit testing and focus, highlights the resolution label
with the native palette, and restores arrow shortcuts. Clicking the text focuses
the row without changing the selected mode. Borderless uses the original
fullscreen switch sprites, and enabling it clears exclusive Fullscreen.

`display_modes.h` extends the original PC interface slots 0/1/2/5 for mode count,
lookup, current mode and mode changes. Additional render presets, up to 7680×4320,
are bounded by device texture limits and exposed only while Graphics builds its
native mode list. Original 24-byte mode records have separate content and render
dimensions. Borderless keeps both pairs at the selected render resolution and
uses the original windowed renderer reset. Physical monitor dimensions apply
only to the HWND, rather than splitting the game's internal coordinate spaces.
The existing window is made borderless and sized to its monitor; no additional
device, window or presentation loop is created.

Window style/position changes must run on the HWND's owner thread. The menu runs
on a worker that the original renderer can wait for; calling SetWindowLong or
SetWindowPos synchronously there deadlocks after an otherwise successful reset.
Extend the existing window procedure with a registered private message and queue
style/restoration work through its original message pump. Preserve and forward
all other messages to the original procedure. A cross-thread regression holds the
owner without pumping messages while verifying the worker returns, then drains
the original queue and checks the resulting style. Native catalog entries with
zero render dimensions are normalized to their selected content dimensions.
Style work is queued only when the window differs from the desired state;
minimized windows are left alone.

The original PC pointer getter (vtable RVA 0x5FDC00, slot 11, function RVA
0x0F8780) copies the coordinates produced by the game's own input update
(RVA 0x15B940). That update uses window pixels. Borderless presentation can
scale a larger back buffer into that window, so the getter extension multiplies
the original result by back-buffer/client size. It preserves the native polling,
button states, focus handling and optional coordinate normalization. Inactive
sentinels and relative pointer mode pass through. No separate mouse polling or
menu hit-test implementation is introduced. The original table entries and
getter bytes are checked before installation.

Main-menu centering reuses the eleven original row containers at menu +248 and
their child artwork bounds. Moving the parents through native `elUIItem.setPos`
moves the labels, icons and hit transforms together, retaining the original
row spacing, input IDs and navigation. Local testing confirmed visible centering;
the user confirmed matching hover after this pointer correction.
The row hit rectangle also extends left through its icon with padding of one
tenth of the native row interval. Compensating child positions preserves the
artwork's visible placement; the original row continues to own hover/click input.
The user subsequently reported that resolution changes are functioning. This
does not by itself verify every Alt-Tab, revert or restart path.

Apply delegates to the original `gaGraphicsOptions.handleScreenInput(3)` for the
renderer handoff, menu rescaling and overlay setup. Borderless-only changes force
the native dirty check without discarding the actual previous mode/fullscreen
snapshot. This reuses `gaGraphicsConfirm` (table RVA 0x7A5918, state 26), its original
localized message, Keep/Revert buttons and revert implementation. Its init,
tick and input hooks use the original countdown fields, with a monotonic
10-second deadline started by the original screen-entry callback (slot 2,
thunk RVA 0x2EB814, fixup RVA 0x7A598C). Resource initialization can happen long
before the overlay opens and must not start the deadline. Whole remaining
seconds are fed to the native timer so its
truncation cannot revert at nine seconds. The previous mode and fullscreen flag
use the original globals at RVAs 0x7EC1F8 and 0x7EBF04. Revert also restores the
previous borderless state and window style/position, and runs only once. Keep
persists render dimensions and display flags in `[graphics]`; startup restores
that confirmed mode through the original renderer.

Menu rescaling and resolution callbacks use the signed managed bridge to invoke
original methods `gaStateManager.rescaleMenus` (token 0x060012A5) and
`gaGame.onResolutionAdjusted` (0x060010DC). The native and managed helpers must be
updated together. Original PC entries and confirmation thunks are validated
against the supported EXE before patching. The display fixture checks extended
catalog bounds, coherent content/render sizes, native pointer mapping at client
center/edges, inactive/relative/windowed pass-through, Win32 message forwarding,
idle/minimized window behavior, actual Win32 borderless styling,
Keep/Reject, failure rollback, the 9999/10000ms boundary and startup restoration;
its renderer and original-confirmation callbacks are fixtures, so physical driver
and live-game behavior need separate verification.

Added option labels use uppercase text, including ANTIALIASING, BORDERLESS
FULLSCREEN and the Servers page actions; the root entry stays titled Servers; original labels are retained.

The original renderer was cross-checked with Ghidra, the separate RiffRacer
recovery sources and original bytes. wsGraphicsDevice (primary vtable RVA
0x5FBF44, public singleton RVA 0x80D408) is a command wrapper. Its PC interface
at +0x18 (vtable RVA 0x5FC178) forwards to a separate render-thread backend:
VA 0x497020 reads global 0xBFCC44. The original lazy initializer at 0x5CFB60
allocates 0xBBC bytes, calls constructor 0x49FD30 and stores that backend in
0xBFCC44 (RVA 0x7FCC44). This backend is non-polymorphic. Its device pointers
are +0x78 and optional Ex +0x7C. Revision 8 used wrapper +0x78 and revision 9
incorrectly treated the forwarding interface as an embedded backend.

Tests validate both original backend-global references and constructor offsets,
then attach a real D3D9 fixture through that non-polymorphic backend layout.
Attachment failures log the exact stage. Live Steam/Proton testing confirmed
device attachment and AA image-resource creation/recreation. Device creation
and presentation evidence is in that recovery's FRAME.md/device_create.S and
application_frame.S; local Ghidra exports for the initializer, forwarding method
and constructor are under build/graphics-evidence.
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
state restoration, enabled-AA reset/recreation and Off cleanup. Full-race visual
and performance checks, physical Windows drivers, Ex reset paths and third-party renderer
wrappers remain experimental validation targets.

## Display experiments held from stable 1.2.2

The display-mode, borderless and confirmation hooks are retained only on the
experimental branch. Stable is based on tested revision 1.2.2-12 and does not
include these hooks or the associated resolution-hover prototype. Live testing
found that keyboard/mouse input can stop after borderless applies even when
controller confirmation works. Reversion can freeze after a successful device
reset. Servers Save and restart after borderless may leave audio playing with
no visible window; it is not yet established whether the old process remains
or a restarted window is invisible. Preserve this evidence for further native
input, window-thread and shutdown investigation; fixture success is insufficient.

Latest restart evidence (2026-11-01 local test): the native menu logs
`Save and restart requested.` followed by the 15-second fallback
`Save and restart: game did not close; request canceled.` The launcher log does
not show a corresponding new completed restart handoff for that latest attempt.
This identifies failure to exit the old game for that attempt; it does not
establish the window/process state of every earlier report. A later process
check found no remaining Game.exe, after the test instance had been closed.

Singleplayer integration retains these same ten rows and original scrolling.
Use Singleplayer and the Singleplayer switch set `networking_enabled=false`
without clearing either server address. Use configured servers re-enables primary,
optional secondary, then in-process singleplayer selection after restart.


### Independent UI scaling (unreleased)

The Graphics slider retains the original Audio slider artwork (sprites 51/54),
0/100 endpoint placement, button masks, repeat input and drag latch. 0 selects
Auto; 25 is Small, 50 matches the 1080p reference, 75 is Large and 100 is Very
Large. The saved value is `[graphics] ui_scale`; absent settings default to 0.
The idle menu state is 0 (as in native Settings), not transition state 1.
Graphics uses the native file-picker reel when rows exceed the available height.

The current manual-scale experiment extends original `gaUI.setupFontScale`
(token 0x060012C0) and `gaMenu.setupMenuScale`. Layout and pointer coordinates
remain on the physical canvas. The font setup alone receives the manual factor;
native menu design-reference constants size artwork and spacing. No per-item
transforms, render-group creation or pointer rescaling are added. Auto restores
the original reference constants and ratios.

The native and managed helpers must be updated together. Fixtures cover sizing
factors, physical-resolution and pointer preservation, reference restoration,
and slider repeat, drag, bounds and transition guards. Manual scale and display
confirmation still require live validation. Earlier render-group experiments
were unsuitable for PC screens; passing matrix tests did not establish correct
live rendering. Auto was confirmed working by the user on November 1.

### Servers menu simplification

The redundant USE SINGLEPLAYER / USE CONFIGURED SERVERS actions are removed.
SKIP LAUNCHER, PRIMARY SERVER ONLY and SINGLEPLAYER retain their native graphical
toggles. Save, reload, restart, back and return remain; SAVE AND EXIT first saves
successfully, then sends the original game window WM_CLOSE. A save failure leaves
the game running. Stable internal IDs are mapped to nine visible rows, including
native wraparound and the original scrolling reel; removed actions are not inputs.

Alt+Enter toggles borderless/windowed through the existing owner-thread style path
without a renderer reset, preserving the selected render resolution. Held-key
repeat is ignored. The original native handler retains exclusive-fullscreen
Alt+Enter. The window hook remains installed in windowed mode and throughout
gameplay; this does not make an already hung message pump responsive.

### Fullscreen selector and resolution visibility

Graphics now has four rows: resolution, fullscreen mode, antialiasing and UI
scale. The fullscreen mode cycles OFF (WINDOWED), BORDERLESS and EXCLUSIVE.
It reuses `elUI.createButtonSprite` (token 0x060016E5, native RVA 0x222C0),
original resolution sprites 59/60, origins 3/5, native Up/Down links, and
left/right input masks. Original fullscreen fields remain the Apply authority;
the separate original toggle artwork is hidden, not removed from its owner.

Original `gaGraphicsOptions.showResolutionText` (token 0x06001177, IL RVA
0x2EC76C) returns immediately if the selected index at menu +300 is unchanged.
Otherwise it hides all text-vector entries at +280/+284, then shows the chosen
entry. A scrolling adapter that hides that entry must explicitly restore only
that selected entry when it becomes visible again. Calling the original function
with the unchanged selection cannot restore it. These semantics were checked
against the original IL; this integration is an intentional mod, not recovery credit.

### Native Apply focus

Original `gaGraphicsOptions.initScreens` explicitly clears menu-item flag bit 1
at Apply item +372 (IL offsets 0x0ABD..0x0AD4, AND 253). This excludes the existing
button from the normal directional-focus lifecycle. ReRiff uses original
`elUIItem.setAsMenuItem` at RVA 0x4B530 to opt it in, retaining its original sprite,
highlight/activation handlers and Apply action. The native Up/Down ID loop now
runs Resolution → Fullscreen mode → AA → UI Scale → Apply → Resolution.
Cleanup restores the original flag and navigation fields.

### Back with unsaved changes

Servers and Graphics compare pending values against their loaded/saved baseline.
Back (selection 2000) opens the original `gaQuitConfirmMenu` yes/no overlay,
state 9, through original `gaStateManager.setOverlayState` / `popOverlayState`
(tokens 0x0600129B/0x0600129C). Its native artwork, focus, Escape/controller Back,
transitions and cleanup are retained. There is no general original unsaved-settings
handler; the integration supplies dirty tracking and save/discard callbacks only.
During this use, selection 2 applies and selection 1 discards. The original
quit action is never forwarded. Escape/Back inside the overlay cancels the prompt
and keeps editing. Callbacks run only after the native overlay is removed.
The actual timed display Keep/Revert overlay remains separate.

The final four Servers actions are Return without saving, Save and return,
Save and restart, Save and exit. Their original internal IDs remain 9, 10,
8 and 11; native link targets and scrolling use the visible-order map.

## Windows image relocation guards (1.2.3-4)

The original Game.exe SHA-256 is
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`
(8,635,392 bytes), preferred PE image base `0x00400000`. Windows logs showed
the same executable loaded at `0x00C30000`, where fixed absolute operands in
otherwise identical function prologues invalidated the old startup checks.

The checked prologue RVAs and 32-bit absolute operand offsets are:
`50AF0+6`, `43800+6`, `8E8D0+6`, `4B530+9`, `8F580+6`, `8E9D0+6`,
`22030+6`, `48830+4`, `482B0+6`. Each operand has an original PE HIGHLOW
relocation entry. The guard now requires that entry, compares its exact expected
relocated value, and compares every remaining instruction byte unchanged.
The `4B530` guard includes all 13 bytes through the four-byte operand. No opcode
or operand masks are used; the original executable hash check remains required.

`test_engine.c` applies the original relocation table to a mapped fixture at the
observed Windows base. The original and relocated images pass; opcode/operand
mutations are rejected. This is runtime-hook validation evidence, not additional
byte-for-byte recovered source or proof of live Windows gameplay compatibility.

### Discardable relocation section

The original `.reloc` section characteristics are `0x42000040`, including
`IMAGE_SCN_MEM_DISCARDABLE`. Windows may unmap these records after applying them.
Requiring the live relocation directory to be readable therefore rejected a
legitimate loaded image even after the 1.2.3-4 address correction. Keep the records
from the same SHA-256-verified file handle instead; continue checking every live
instruction byte and its exact adjusted operand. The regression fixture protects
the mapped relocation region with `PAGE_NOACCESS` and verifies that valid entry
checks pass while opcode/operand mutations still fail. The log now reports the
specific failed guard line and build version for remaining runtime differences.

### Live-header independence

1.2.3-5 Windows logs reach the first prologue guard (`RVA 0x50AF0`) after
vtable/thunk validation, with all 200,596 verified relocation bytes available.
The prior helper still read the live optional header and required its ImageBase
and relocation directory to match unmodified PE32 file metadata. That condition
is unnecessary: preferred-base metadata and bounds now come from the same
hash-verified file as the relocation records. Every live prologue byte and adjusted
absolute operand is still checked, with an original HIGHLOW entry required.

The added fixture changes live ImageBase/relocation-directory fields while live
relocation pages are inaccessible; valid instructions pass and an altered operand
fails. This models a loader state, not proof that those specific header values
occurred on the affected PC. Failure diagnostics now record the live PE magic,
base field and prologue bytes so any remaining difference can be identified.
Microsoft documents that CLR validation may modify an in-memory managed image:
https://learn.microsoft.com/en-us/dotnet/framework/unmanaged-api/hosting/corvalidateimage-function

### Direct3D device table preservation

The Windows 1.2.3-6 crash report records `0xC0000005`, unknown module, fault
address `0x00000001`, and CLR `System.AccessViolationException` with only the
outer `_WinMainCRTStartup` frame. Native logs put it during/reset-after Borderless
Apply. This suggests an invalid function pointer but does not identify the caller.

The previous AA hook substituted a 119/134-entry copy for the device's native
method table. That preserves the public COM interface only, not necessarily the
runtime's full internal dispatch layout. Keep the native table pointer instead
and atomically replace only the original public Present/Reset and Ex slots.
Record the original callbacks before publishing hooks; callbacks for other devices
sharing a table forward directly without touching primary-device resources.

`test_antialiasing.c` verifies original table pointer identity, unmodified methods,
visible AA output, state restoration, Reset/Present and ResetEx/PresentEx, plus
another device's reset/present leaving primary AA textures/surfaces intact.
Run the fixture normally and with `--ex`. Both passed through Wine; actual Windows
crash resolution still requires a new run, and a dump is needed if it persists.
