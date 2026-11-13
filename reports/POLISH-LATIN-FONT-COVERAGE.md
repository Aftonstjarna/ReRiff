# Polish character coverage through the existing language fallback

Original reference: Steam depot 351991/build 1687655 Game.exe, 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The original-file hash was compared with the depot reference for this work.

## Observed boundary

The existing Unicode observer recorded the directory text `Dawid Podsiadło`
with U+0142 intact both at `find-next-W` and shared `font-layout` (caller RVA
0x5063A). At shared native glyph lookup (RVA 0x56E60), the observed request
U+0142 selected U+003F. This establishes missing glyph coverage at this boundary;
it does not establish a filename-encoding loss or a backend defect.

The pinned Noto CJK metrics lacked U+0142 and several other Polish letters.
A file-picker display failure therefore remains possible even with the earlier
UTF-16, UTF-8 metadata and fallback-layout corrections working as intended.
The source strings need no replacement with approximations or question marks.

## Intentional ReRiff extension

The independent Latin/European download uses Noto Sans from google/fonts commit
`29b4ec6df2522a8aee279414b78a07910b065874`, source SHA-256
`bfb7bb691513f12e734dc346c03a03f784912432d7e3fa8e56efcf906fe86b3d`.
The font and license pins are in `packaging/fonts/noto-latin.json`.
It supplies supported Latin characters from U+00A0 through U+024F, including
all uppercase/lowercase Polish letters. This is glyph coverage, not translation
or general complex-script shaping.

Existing native primary glyphs and existing CJK fallback glyphs retain priority.
Missing Latin records join the existing owned fallback, with a shared canonical
baseline and 32px layout metrics across 32/64/96px textures. No new native
instruction detour is introduced. The CJK source and its glyph designs remain
unchanged. Separate install/enable settings feed one shared owned VFS pair.
Original and unrelated mod VFS archives are retained. Per-pack ownership hashes
are updated together in the installer's rollback-capable transaction.

This is an intentional patch and asset integration, not original source
reconstruction or a matching-byte contribution. It builds on the addresses,
ABI guards and layout observations in `CJK-FONT-INTEGRATION.md`; no new original
function body or matching-byte claim is submitted here.

## Validation and limits

- Polish glyph alpha and canonical layout agree at 32, 64 and 96 pixels.
- Independent installer regressions cover both install orders, both packs in
  one selection, one shared VFS pair, disabling one pack, original preservation,
  changed-owned-file refusal and existing CJK migration/rollback behavior.
- Native GTK controls and child-process arguments pass isolated UI checks.
- Native original archive validation, shared fallback layout/highlight and
  UTF-8 metadata checks are recorded in the local build results.
- A live visual check of the Polish directory is still required after install.
