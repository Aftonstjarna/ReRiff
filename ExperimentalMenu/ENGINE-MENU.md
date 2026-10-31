# Engine-rendered Servers experiment

This is the corrected experiment on `codex/experimental-ingame-menu`. It hooks
the original `gaMainMenu` callbacks and edits existing `elUIItemText` objects
through the game's native text functions. It creates no windows, polls no keys,
and uses no overlay renderer. Original menu focus, mouse/keyboard/controller
selection dispatch and rendering remain responsible for interaction.

The prototype temporarily replaces **Friends** with **Servers**. News retains
its original label and action. Selecting Servers
turns the existing main-menu rows into a server-settings page. This is an engine
menu page within the existing screen, not a new registered `gaState` subclass.
It reuses the existing layout and icons. Back/Escape restores the original
localized main-menu labels, with Friends continuing to read Servers.

Commands select localhost, select the external URL already saved in server.ini,
toggle external-only mode, disable networking, save, reload, or return. Changes
are pending until Save; Back discards them on the next opening. They take effect
after restarting the game. Both disabled/offline addresses remain blank.
Arbitrary URL text entry, backend start/stop, live endpoint switching and new
menu artwork are not implemented. The replacement Friends row is re-enabled
after the original tick's Parse-connectivity gate and receives the original
focused/normal menu colours. New Track still uses its original connection gate.
If the native online interface is absent, the original constructor omits Friends
entirely; this experiment logs that absence instead of constructing a new row.

## Build, install, restore

Close Game.exe before installing/restoring. From this workspace:

```sh
distrobox enter development -- bash /var/home/afton/.dev/ReRiff/ExperimentalMenu/build-engine.sh
distrobox enter development -- python3 /var/home/afton/.dev/ReRiff/ExperimentalMenu/install_engine.py --game-dir '/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer'
```

The installer changes only `ReRiff.Notify.dll` and backs up the prior DLL with
hash verification under `backups/experimental-engine-menu`. It does not change
Game.exe, Parse patches, ClientConfig, server.ini or launcher files.

```sh
distrobox enter development -- python3 /var/home/afton/.dev/ReRiff/ExperimentalMenu/install_engine.py --game-dir '/var/home/afton/.local/share/Steam/steamapps/common/Riff Racer' --restore
```

The existing patched ClientConfig must call its notification export to load the
DLL, so `show_server_popup=true` remains the startup trigger. Despite that setting's
name, this build displays no popup. Hook installation occurs at that call and
the new label appears when the original main menu initializes. If initialization
already happened, return to/recreate the main menu. The DLL stays loaded for the
process lifetime. Installation and refusal reasons go to
`ReRiff-engine-menu.log` beside Game.exe; no URLs or account data are logged.

## Evidence and guards

Supported original SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Installation and runtime check the entire original file hash. Runtime also
checks all 27 menu vtable entries, the four IJW jump thunks and the native
text/input-function prologues before writing four aligned vtable pointers. Unexpected
bytes cause refusal. Game.exe on disk is unchanged. Original callbacks are
retained for initialization, tick, cleanup and all ordinary main-menu selections.

| Original entry | VA | Metadata token / evidence |
| --- | --- | --- |
| gaMainMenu vtable | 0x00BA7470 | Original data and ILSpy field |
| tick, slot 3 | 0x00740A54 | thunk through 0x00BA74F8 to 0x0600149A |
| initScreens, slot 15 | 0x0073FD14 | thunk through 0x00BA74F0 to 0x06001492 |
| onCleanupScreens, slot 16 | 0x0073F23C | thunk through 0x00BA74EC to 0x06001494 |
| handleScreenInput, slot 24 | 0x0073E88C | thunk through 0x00BA74E0 to 0x06001498 |
| Original label table | 0x00BA7434 | News ID 4, Friends ID 8, Settings ID 9, Exit ID 10 |
| elUIItemText.replaceTextRaw | 0x00450930 | 0x060016A2, UTF-16 raw display text |
| elUIItemText.replaceText | 0x004508A0 | 0x06001769, localized string keys |
| elUIItem.enableInput | 0x0044B4A0 | 0x060016D7, thiscall with byte boolean argument |

ILSpy `analysis/native-port/decompiled/Module.cs` establishes the menu's item
array at +248, title at +144, and child-vector begin/end at item +376/+380.
`gaMainMenu.tick` gets child zero as the text item. The hook borrows those
references only during callbacks and never constructs/releases game objects.
Native text calls and the hooks use x86 thiscall (ECX object, stack arguments).
Original disassembly confirms text replacement's RET 4. Ghidra exports at the
four native/thunk addresses are retained locally in RiffRacer's
`analysis/ghidra/recovery`. Ghidra's inferred text prototypes and six-byte body
extents are incomplete; original instructions and managed signatures supplied
the calling-convention evidence. Additional Ghidra exports for the tick thunk
and input-enable routine verify the Friends availability path. Focus is the
original menu's field at +352; slot 26 is the original updateItemColour callback.
Its focus/normal RGBA values come from gaMenu's original static colour data.

## Validation and remaining trial

Both x86 builds pass `-Wall -Wextra -Werror` inside development. Proton Hotfix
tests pass the original SHA-256 check, refusal in a foreign host executable,
original vtable/thunk/text-prologue checks, altered-vtable rejection, thiscall
dispatch, menu text routing, original-selection forwarding, preset selection,
save/reload, unrelated-key preservation, Back and cleanup. Regression checks
also verify News forwarding, Friends replacement/restoration, re-enabling after
an offline tick, and harmless handling of an absent Friends row. The test uses the
unrelocated original image as a read-only byte reference and stand-in menu/text
objects for behavior. It does **not** execute original rendering methods.

Actual Game.exe rendering, CLR-to-native callback transitions, all controller
paths and focus behavior still require a live game trial. Hook initialization
and page-open log entries distinguish loading from successful menu use. A live
failure should be investigated or rolled back; passing the harness is not a
claim that the actual game screen works.

No matching-reconstruction credit: baseline remains 64,760 / 8,635,392 bytes
(0.749937%). This DLL is an intentional mod, separate from `decomp` recovery.
