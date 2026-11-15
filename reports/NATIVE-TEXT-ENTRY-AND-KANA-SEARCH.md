# Text-entry prototype and kana search groundwork

## Original reference and scope

Original Windows Game.exe: Steam depot 351991/build1687655, 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted recovery evidence, original strings, CLI picker/confirmation methods
and the existing native overlay adapter. No live decomp checkout was modified.
This is an intentional extension, not original byte-matching reconstruction.

No complete editable game text widget was identified in the examined UI
constructors/types and input methods. That is a bounded negative finding, not
proof that no unused editor exists anywhere in the executable. Text rendering,
focus, confirmation overlays and the picker reel are present and reused.

## Reused overlay

The existing gaQuitConfirmMenu vtable is image+0x7A7900. In addition to the
previously guarded enter/init/input slots, the prototype wraps slot3 (tick):
original callable thunk RVA0x360BC1, exact six bytes `ff257479ba00`, whose
absolute target location relocates to image+0x7A7974. Existing slots2/15/24
remain at RVAs0x361068/0x361B88/0x360B60 with original target locations
image+0x7A7978/0x7A7980/0x7A796C. The installer checks all four before writing.

The native state9 overlay retains original visual construction, confirmation
buttons, lifecycle and callbacks. Editor rendering uses its existing title and
message items; no separate renderer or substitute confirmation menu is added.
The window hook transports UTF-16 character/key/clipboard operations into a
small shared editor state protected by a critical section. Only the menu thread
calls native UI or pushes/pops an overlay. Accepted addresses update pending
Servers settings; the existing explicit save commits them. Cancel discards the
edit. No addresses or typed text are written to diagnostic logs.

The new rows use the original Settings spacing, focus and native reel scroll.
Tests exercise the expanded rows and preserve original Audio/input isolation.
Native tests passed for UTF-16 surrogate-safe editing, clipboard, deferred
accept/cancel, URL scheme/host validation, duplicate-editor refusal and native
confirmation ownership. Real typing, controller focus, IME composition and
appearance still require live validation; automated character insertion does
not establish those behaviors.

## Search groundwork

KanaSearch creates derived search keys only: NFKC normalization, kana script
folding and a kana-to-romaji map including digraphs, voiced forms, small-tsu
consonant doubling and prolonged vowels. The original name/path is preserved.
It supports literal kanji matching but does not infer kanji readings.
Fourteen managed checks passed on the Windows CLR for mixed names, tsu/つ/ツ,
halfwidth and combining forms, kana script equivalence and preserved filenames.
This matcher is not yet wired into the live picker.

Original gaFileSelectInterface.setupDirectoryContents token0x0600104A at
RVA0x2D5814 destroys and rebuilds the native 28-byte dirEntry vector. Filtering
must preserve its wstring ownership and the one-to-one row/reel mapping; hiding
arbitrary rows or rewriting just one vector is not a validated search method.
Nested native overlay ownership and space for a search control must be checked
before adding a field to the picker. The prior unsafe wraparound is not reused.

## First live editor test and undo extension

The user fully restarted and confirmed typing, caret movement, paste and cancel
in the server-address overlay. No changed address was saved in this test.
The picker layout trace showed native overlay ownership already belongs to the
picker (manager+44 == picker). A second confirmation overlay therefore cannot
be assumed safe for search. The observed first directory row was y286, height40,
with11 source entries; this is one tested layout, not a universal space budget.

The shared editor now has bounded per-field undo/redo history. Consecutive typing
within750ms can form one step; paste and cursor/selection changes break groups.
Tests passed for Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z, Unicode paste reversal, selection
replacement, history branching and reset when opening another field. These
are new ReRiff behaviors, not recovered original editing functionality.
