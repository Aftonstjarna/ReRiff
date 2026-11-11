# Controller input and graphical remapping routes

Static research, 2026-11-02. No installed gameplay/menu changes. The requested
live probe was cancelled before its Windows sampler was launched; no live race
measurements were obtained. Linux denied the earlier read-only process-memory
attempts. A claim that forward stick accelerates faster than RT is unverified.

## Provenance

Original Windows reference: Steam depot 351991/build 1687655, Game.exe size
8,635,392, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the local RiffRacer recovery project and the existing hash-identified
IL survey in ReRiff's ignored analysis directory. Decompiled C# is an aid, not
source reconstruction. Original IL headers/bodies were separately extracted
from PE section data for the hashes below. No matching credit is claimed.

See also [the trigger-path evidence](NATIVE-MANUAL-SPEED-TRIGGERS.md).

## Existing input path

`gaInputManager.tick` (`0x06000C5D`) constructs each player's input record,
invokes `tickX360` (`0x06000C5E`) for the controller, then `tickPC`
(`0x06000C5F`) for keyboard input where available. It retains native pause,
focus and handbrake handling. The X360 mapping has literal button/axis choices;
the inspected path does not use a configurable controller binding table.

The X360 path assigns RT to acceleration and LT to braking, through the
original trigger getter and dead-zone function. Steering reads the stick
separately. Keyboard acceleration/braking can override their analog values
when those keys are held. Manual speed consumes these inputs as a speed
adjustment relative to the pace marker, followed by the original physical
throttle/brake calculation. An apparent sync difference is not enough to infer
a different maximum input value or a faster physical throttle path.

## Original graphical remapper

| Original method | Token | RVA | Relevant behavior |
| --- | --- | --- | --- |
| gaControlsOptions.initScreens | `0x06001159` | `0x2E9868` | Native text, sprites, selectable rows and navigation IDs. |
| gaControlsOptions.handleScreenInput | `0x0600115C` | `0x2EAE5C` | IDs 1–17 dispatch to updateSelection. |
| gaControlsOptions.updateFocus | `0x0600115D` | `0x2EAEE0` | Updates row presentation and calls base focus handling. |
| gaControlsOptions.tick | `0x0600115F` | `0x2EAF04` | Polls a PC keyboard input while capturing a binding. |
| gaControlsOptions.updateSelection | `0x06001160` | `0x2E94DC` | Focus/active colours and binding capture presentation. |
| gaControlsOptions.updateKey | `0x06001162` | `0x2E8AE8` | Updates text, registry storage and the active input manager. |
| gaControlsOptions.remapKey | `0x06001163` | `0x2E8C00` | Swaps an existing conflicting key binding. |

Specific observations:

- The two toggles occupy control IDs 1–10; keyboard binding IDs are 11–17.
  Active capture is an integer at menu offset 248 (`-1` means inactive).
- Capturing skips the normal base menu tick. It gets player-zero's PC
  interface through virtual slot +84, then polls that interface's slot +68.
  This is not a general controller-button capture routine.
- Seven key-binding records begin at menu offset 344, with a 20-byte stride;
  their stored key codes begin at offset 484.
- `updateKey` uses MapVirtualKeyW/GetKeyNameTextW for the displayed label and
  persists Windows virtual-key codes under the existing keyboard-controls
  registry key. Reusing it unchanged for controller IDs would mislabel and
  misroute those inputs.
- `remapKey` checks the other seven bindings and swaps the previous binding
  into a conflicting action. Escape is ignored by `updateKey`; its precise
  interaction with conflict-swapping deserves a regression before reuse.
- The existing menu uses normal native menu-item navigation fields, priority
  and selectable flags. These should be retained rather than adding a second
  navigation implementation.
- An asset catalogue entry exists for
  `graphics/misc/controllerbuttons/batchTextures.brres`. Its usable Windows UI
  sprite IDs and complete glyph coverage have not yet been established.
  Existing keyboard sprite IDs must not be relabelled as controller glyphs.

## Recommended extension, not yet implemented

Extend the existing Controls menu with a controller bindings page or mode;
retain the keyboard page and its bindings. Reuse native rows, focus colours,
selection audio, toggle/selector art and capture presentation. Use the native
menu reel/scrollbar if rows do not fit at supported resolutions/UI scales.

The genuinely new pieces would be:

1. Typed controller bindings: button, trigger, or signed stick axis. Preserve
   analog magnitudes instead of translating triggers into keyboard presses.
2. Controller capture using the game's current controller interface. Require
   neutral/release before capture so the button used to enter capture is not
   immediately rebound; distinguish held buttons from new presses and axis
   movement. Preserve a reliable cancel/reset route.
3. Controller-specific persistence, separate from keyboard virtual-key codes.
   Keep originals as defaults; make conflicts and any permitted multi-bindings
   explicit. Reuse the native swap behavior only where its semantics fit.
4. A gameplay-only adapter that feeds the existing gaInputData/vehicle path.
   Do not globally remap low-level controller getters: that would also affect
   menus, confirmation and cancel/navigation controls. Verify the selected
   hook's actual caller ABI and frame order before changing any function.
5. Feedback showing the bound input, current analog value and dead zone, using
   native control styling where available. This would make partial trigger
   travel and translation-layer mappings diagnosable without changing physics.

A full controller-diagram UI is a possible later presentation layer, but it
needs verified glyph/diagram assets and anchors. Native action rows with input
glyphs are the smaller, better-supported first implementation route.

## Original body fingerprints

SHA-256 covers the original fat method header and IL code bytes, excluding EH
sections. Exact bytes remain in the original executable; these fingerprints do
not describe patched code.

| Method | Bytes | SHA-256 |
| --- | ---: | --- |
| initScreens | 4958 | `57b2e68749820af59f67a74568dc6fbd4ee4682924301664b283f99388494d98` |
| tick | 135 | `6d216078fe2b8b074b220d93b689c276d55688df99605c2381536b1433579f90` |
| updateSelection | 853 | `108dfb7fa9b38a96d88872b426372b40d02a4c5aec620793898b580ace928135` |
| updateKey | 278 | `f13963e8a4df8f805793aa653c4419ecf48c2d74c02fda73804ea807e31797d2` |
| remapKey | 75 | `f1216237f99af80f84924770c623b651926844f3b8f7ee400145ce200e5be83d` |
