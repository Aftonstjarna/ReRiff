# In-game menu consistency audit

Date: 2026-11-11. ReRiff source context:
`7e7c441184d3cf80d0873b7a6bf9669e820b2a3c`, with local menu changes.
Recovery reference: RiffRacer checkout
`f375e465e075446f2a269921a43bef261c40eeff`.

## Original reference and scope

Steam depot 351991, build 1687655, manifest 8951408038967536554.
The unmodified reference executable is 8,635,392 bytes, SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Its hash was checked against the DepotDownloader reference for this audit.

Reviewed `Decomp/RERIFF-EVIDENCE.md`, `Decomp/native/UI-MATRIX.md`,
`Decomp/native/PE-MENU-RELOCATIONS.md`, the local original Graphics tick IL
export, and ReRiff's Settings, Graphics, slider, prompt, scaling and menu-hit
adapters. Recent recovery commits inspected concern collision geometry and
record initialization; they do not establish new menu behavior.

## Consistency checks

- UI scale uses native font/menu sizing, with physical coordinates retained.
  It does not use `elMatrix.scaleBy` to reposition a menu. The recovered matrix
  helper, token `0x06001651`, VA `0x458810`, leaves translation unchanged.
- Graphics mode arrows use the original button-sprite factory, token
  `0x060016E5`, VA `0x4222C0`, and sprites 59/60 with origins 3/5.
- UI Scale uses the Audio pressed/repeat masks 256/512 and has no competing
  activation shortcut. The native mode-1 direction links use -1 for Left/Right;
  the original handler at VA `0x44AA60` ignores that target.
- Apply is the existing ID-3 sprite, deliberately made selectable by ReRiff.
  Native `setAsMenuItem`, token `0x060016AA`, VA `0x44B530`, also assigns
  materials and flags. ReRiff calls the original function rather than treating
  it as a single-bit write. The harness's simplified menu-item mock is not
  independent validation of those material side effects.
- Save/discard uses the original quit-confirm overlay, state 9, with the quit
  action intercepted. Explicit choices are deferred until the overlay is gone;
  Back keeps editing. Manager methods are tokens `0x0600129B/0x0600129C`.
- Added scrolling uses the original gaMenuReel bridge. Hidden rows have zero
  input masks; unchanged masks retain press/release latches. Selected resolution
  text is restored by layout because original `showResolutionText`, token
  `0x06001177`, returns early for an unchanged index.

## Reproduced ReRiff integration defect

During an active overlay, `aa_graphics_tick` called input reconciliation, which
made the added Graphics rows visible, then returned without restoring the reel
viewport. On a short viewport this revealed the hidden UI Scale row behind the
confirmation. Input was disabled, so this observation does not establish an
overlay activation or focus failure.

The regression fixture reproduced the defect before the correction: the new
hidden-slider assertion failed. The correction runs the existing
`graphics_layout` after the original Graphics tick in the overlay branch.
It restores visibility and disables underlying row input without ticking or
moving the reel while the overlay owns input. No original executable bytes,
metadata tokens, allocator conventions or native input routines were changed.

The fixture checks hidden slider artwork, disabled AA/Apply masks, preserved
scroll position and absence of reel ticks during the overlay. The native menu,
UI scale, save-prompt and display harnesses provide focused integration checks.
Actual Windows/Proton mouse, controller, animated layout and display transitions
still need an in-game retest; fixtures do not prove their visual behavior.

This is a ReRiff audit and intentional integration correction. It introduces no
new original-function reconstruction or matching-byte credit.
