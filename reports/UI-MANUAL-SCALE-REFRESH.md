# Manual UI scale and native menu fitting

Investigated 2026-11-13 after manual scale changes left the main menu at its
resolution-dependent size and Graphics rows entered the header area.

## Original evidence

Reference: Steam depot 351991/1687655 `Game.exe`, 8,635,392 bytes, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Read alongside the recovery project's `native/UI-MATRIX.md`: matrix scaling
does not scale translation, so it is not a whole-menu layout solution.

The original CIL disassembly in `build/unicode-game-module.il` establishes:

- `gaMainMenu.initScreens`: body RVA `0x33f244`, 12-byte header, 2,272 bytes
  of IL. At IL `0064..0095`, it reads menu uniform Y scale at byte offset 60
  and reserves `170 * scaleY` above and `50 * scaleY` below the fitted list.
  It obtains its own font and derives list sizing from the available height.
  Therefore changing only shared font setup does not determine main-list size.
- `gaStateManager.rescaleMenus`: body RVA `0x2ff6a8`, 12-byte header, 58
  bytes of IL; iterates states and dispatches their menu-scale callback.
- `gaGame.onResolutionAdjusted`: body RVA `0x2d76c8`, 12-byte header, 299
  bytes of IL; calls `gaUI.setupFontScale` and updates viewport geometry.
  This notification itself does not perform a graphics-device reset.

SHA-256 of each original 12-byte header plus stated IL span, excluding any
following exception sections or padding:

| Body | Header + IL SHA-256 |
| --- | --- |
| Main init | `19818863966ba952a1c4babc189323017966cc79e351c73f5be7c3582d63564e` |
| Rescale menus | `05e435f779fe7d9faa70444c25dfd612e6870add09bdadf9b79653a61ec9e429` |
| Resolution adjusted | `f5d5114a928fdd47790f1ea97f56fc258e80b8e5c54b99e0bc3736ae35628a7f` |

These are provenance fingerprints, not a matching reconstruction claim.

## Intentional ReRiff integration

Manual scale uses a fixed 1080p reference for native font setup and menu design
reference. Auto restores the original reference and font factor. Saving a scale
change schedules the original menu/viewport notifications after the menu input
callback returns, without changing resolution or resetting the renderer.

The main list compensates for its separate native height fitting by scaling
original row parents, retaining native children, hit transforms and input.
Overflow uses the existing `gaMenuReel`, native direction callbacks and repeat
handling. Graphics and server rows retain spacing and start below the header;
their existing reels expose entries that no longer fit. No matrix or global
pointer-coordinate replacement is involved.

## Validation and limits

Native regression fixtures cover 720p, 1080p, 1440p and 2160p at manual values
25, 50, 55, 75 and 100, plus Auto restoration, deferred refresh, parent scaling,
original direction delegation, reel cleanup, wheel focus and icon hit bounds.
The engine integration test runs against the depot original's guarded bytes.
These checks pass; actual in-game appearance after changing only scale still
requires a user visual retest. The adaptation is a ReRiff patch, not vanilla
behavior or a byte-matched recovery contribution.

## Follow-up: Settings separator sizing

`gaOptionsMenu.initScreens`, CIL body RVA `0x2ef354`, 12-byte header and
2,739-byte IL stream, creates separator sprite 51 with origin 4. At
IL `03fb..0427`, its scale vector is `(requestedWidth / spriteWidth,
menuScaleY)`. It does not divide the Y component by the sprite height.
The second separator construction repeats this at IL `068e..06bf`.
SHA-256 of the original header plus IL, excluding trailing sections/padding:
`1651efbe340b1c8cea68f7721c692fe8906512a8f32101749413124174822926`.

ReRiff's added separators had instead used `(width / spriteWidth,
menuScaleY / spriteHeight)`, compressing the full artwork into the requested
pixel height. They now use the original vertical scaling rule. A fixture with
a 16-pixel sprite height verifies Y scales at manual 25/50/75/100 without
dividing away the artwork's height. Actual visibility still needs an in-game
visual retest; this is an intentional patch, with no matching-byte claim.
