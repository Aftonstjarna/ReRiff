# Native music file-picker boundary navigation

Original reference: Windows Steam depot 351991/1687655 Game.exe, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The local RiffRacerDecomp native/recovered notes were consulted; no recovered
`gaFileSelectInterface` implementation was found in that search. This note
records original managed CIL and native instructions, not matching recovery.

- `gaFileSelectInterface.changeSelection`: token 0x06001054, CIL method
  RVA 0x2CF4A0, 1606 code bytes after its 12-byte header. Its first instructions
  increment field token 0x04000F39, RVA 0x7E96C4, once per call.
- `tickInput`: CIL RVA 0x2CFC00, 763 code bytes. It owns direction +376,
  repeat countdown +380 and repeat period +384, calls changeSelection for
  accepted Up/Down attempts and re-arms its own countdown even at a boundary.
- `updateVisibility`: token 0x06001056, CIL RVA 0x2CC7FC, 974 code bytes.
- `tick`: CIL RVA 0x2CFF08, 1579 code bytes. The original vtable is at
  RVA 0x7A54BC. Its existing ReRiff UI-scale tick adapter remains the single
  vtable integration point.
- Native file rows are the SSP vector at +324/+328. Fixed rows +340 and +336
  precede the vector in navigation. Vector rows outside the current viewport
  can be hidden. The picker reel is at +248 and screen at +184.
- Original `elUIScreen.setFocus(int)`, VA 0x48F580, delegates to its group and
  tracks the focused SSP at screen +20. Its absent-ID path permits clearing
  focus. changeSelection's original absent-focus branch chooses the last
  visible list/fixed row for Up, or the first fixed/list row for Down. It owns
  the normal highlight, audio and SSP lifetime handling.

The intentional ReRiff extension observes the original attempt counter around
one unchanged native tick. Only one accepted attempt with unchanged focus,
unchanged screen/reel/vector/phase, and the focus at the matching outer boundary
can wrap. It uses the original reel's zero-duration move and updateVisibility,
clears screen focus through the original API, and calls unchanged
changeSelection in the same direction. No independent input polling, timing,
Game.exe modification, fake input, or custom selection rendering is added.
Empty and single-entry lists remain supported. A rejected focus clear leaves
the original selection untouched; an unsuccessful native selection restores it.

Regression harnesses cover both directions, multiple loops, native repeat
acceptance versus waiting frames, scrolling reveal, native movement taking
precedence, fixed-only/empty/single-row layouts, invalid vectors, and the
original method-header guard. Callback tokens are resolved with exact method
name checks behind the existing original executable SHA-256 guard.
A live keyboard/controller test remains necessary to confirm the runtime
counter and the original visibility/focus behavior on the user's current game.

## Live-test correction

The first live boundary attempt exited with 0xC0000005. The earlier synthetic
harness did not reproduce the real focus/scroll handoff; its passing result is
not evidence of live safety. The two previously working client files were
restored before further testing. No exact failing instruction was captured.

The assumption that setFocus(-1) clears focus is withdrawn. The original
integer overload calls elUIItem::setFocus(id) on the tree; an ID of -1 is not
proved absent from that tree and cannot be treated as a clear-focus API.
The scroll and visibility calls also remain unverified in this handoff.

The runtime adapter now observes accepted native boundary attempts only.
It does not call selection, visibility, focus or scroll functions. A bounded
log records live row IDs, vector length and reel type for the next investigation.
The regression harness explicitly aborts if any of those action callbacks run.
Wraparound remains disabled until the crash and native handoff are established.

## Abandoned at user request — 2026-11-13

The observation-only live test reached both edges without crashing. Boundary
records confirmed native attempt detection and the expected reel vtable,
including fixed-row and list-only layouts. This does not identify the failing
instruction in the action prototype.

At the user’s request, all runtime hooks, bridge additions, probes and related
prototype tests were removed. The verified pre-experiment client files were
restored locally. The separate race-pause Servers removal remains intact.
Research is retained for provenance only; no further wraparound work is active.
