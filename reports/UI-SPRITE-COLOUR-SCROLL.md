# Standalone toggle artwork and scroll focus resets

Original reference: Steam depot 351991/build 1687655 Game.exe, 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Evidence: the unmodified depot executable, its prior CLI dump, existing recovery
notes, and isolated execution of the original native instructions under Wine.
No files were written into the live recovery checkout.

## Verified original behavior

`elUIItem.setColour` is the native vtable slot 4 at VA 0x448500, RVA 0x48500.
The reviewed span ends at VA 0x4486A1 inclusive (`ret 4` at 0x44869F): 418 bytes,
SHA-256 `d9496c98fa2e35e537c426b8e3b69b002de3dac3b7a7268566e72dafade98cef`.
This hash identifies the original fragment; no source-built matching claim is made.

It copies RGBA into item offsets 0x54..0x60. For a mesh visual (nonzero visual
+0x12C), it traverses the vertex vector at +0x120/+0x124 in 36-byte records,
replacing each packed vertex colour at +8 with the supplied colour. It does not
multiply the existing vertex colour. For example, the instruction at 0x4485E6
is `89 54 03 E4`, storing the replacement packed value for the current vertex.

A regression executes the original function on a bounded fixture with four
0xFF888888 vertices. A white RGBA call replaces all four with 0xFFFFFFFF and
stores that RGBA on the item. The test mapping is relocated privately; original
files are unchanged. This confirms the destructive effect of an explicit white
reset on mesh shading, without needing to infer it from a screenshot.

The original Graphics `updateItemColour` CLI body at RVA 0x2EC6D4 is nine bytes
and delegates to `gaMenu.updateItemColour`. The standalone toggle thumb is made
with item ID -1. The original `setTogglebarVars` CLI body at RVA 0x2ECA18 uses
sprite definitions 74/75, origin and position; it adds no explicit thumb-colour
reset. These CLI identities retain their original RVAs; metadata tokens have
not been newly resolved in this note.

## Intentional ReRiff correction

The custom Servers focus hook and deferred scroll-focus reset explicitly coloured
both the bar and thumb white. This could replace the thumb's shading when scrolling
back to the top. Initial creation and the added AA row had the same unnecessary
explicit colouring path.

Keep native standalone thumb artwork untouched: apply row highlighting/reset to
the label and bar, while continuing to use original sprite-def/origin/position
calls for on/off state. No new native hook or hardcoded replacement shade is added.
Tests exercise scrolling/focus cycles, verify bar highlighting still propagates,
and ensure none of these paths dispatch colour resets to standalone thumbs.
Live appearance remains subject to a user retest after restarting the game.
