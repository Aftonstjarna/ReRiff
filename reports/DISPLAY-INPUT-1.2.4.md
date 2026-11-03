# 1.2.4-beta.1 display/input investigation

Original Game.exe SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Original VAs below use image base 0x00400000. Ghidra exports were read-only
from a private project; instruction checks used the unmodified depot copy.
These are behavioral findings, not newly byte-matching reconstructed source.

- Original pointer update at VA 0x0055B940 calls GetCursorPos and ScreenToClient,
  subtracts window-object offsets +0x14/+0x18, supports relative mode (+0x246)
  and optional normalized coordinates (+0x247). Getters 0x0055B470 and
  0x004F8780 return the stored coordinates, not fresh screen coordinates.
  ReRiff must not apply client/render scaling to relative or normalized input.
- The original window callback 0x0048C740 compares the complete WM_ACTIVATE
  WPARAM to zero (instruction test at 0x0048C82B). WA_INACTIVE plus a nonzero
  minimized high word therefore takes its activation branch. Normalizing the
  activation code preserves the native 0x0048D240 deactivation routine, which
  clears the active byte +0x140, keyboard bytes and mouse states.
- Existing recovery in Decomp/native/WORKER.md covers the synchronized reset
  requests and original window sizing at 0x0048CB30. Preserve that reset path;
  restore frame/position with the currently selected client size, rather than
  restoring dimensions captured before a resolution change.
- Quit confirmation token 0x0600158D handles ID 2 as confirm and other valid
  choices as cancel. Tokens 0x0600158E/158F preserve base focus and button colors;
  0x06001590 handles Back. ReRiff keeps this native overlay. Added rows must not
  re-enable input while any overlay owns the state manager's overlay slot.
- Fullscreen selector arrows own Left/Right (256/512). Also binding both keys
  to the label creates overlapping activation owners; the label now has zero
  activation shortcut while retaining native directional focus links.

Regression coverage includes windowed client/render pointer mismatch, relative
and normalized pass-through, minimized deactivation/capture release, selected
window-frame size requests, overlay input masks, selector shortcut ownership,
and native Keep/Revert timeout/rollback. Headless Wine can constrain actual HWND
dimensions: the sizing test verifies the requested adjusted frame, and the
pointer test verifies scaling against the actual client rectangle.

In-game Linux and Windows checks are still required for click alignment,
Alt-Tab/focus, selector repeat and save/display confirmation. The build retains
the separately tested FLAC resampling patch; no server data or caches are reset.

## Selector follow-up

Original sprite creation at VA 0x004222C0 consumes five stack arguments and
sets native menu-item/selectable bits 0x06, then stores its shortcut at +0x40.
Original item setScale at VA 0x00448A10 writes X/Y to +0xAC/+0xB0, not the
origin/priority fields at +0x38/+0x3C. The regression fixture previously wrote
its mock scale into those origin/priority fields; it now models the verified
scale offsets. No matching-byte reconstruction credit is claimed.

The fullscreen centre rectangle now ends before the arrow hitboxes, its click
only focuses the row, and each arrow uses priority 100 and its own native
focus colour. Activation logs record the selected direction and resulting mode.
The native regression suite passes; another desktop interaction test is needed.
