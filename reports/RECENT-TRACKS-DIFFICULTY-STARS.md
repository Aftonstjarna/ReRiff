# Recent Tracks difficulty stars (2026-11-18)

## Verified original evidence

Pinned depot 351991/build 1687655 Game.exe SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Read-only dnlib IL inspection, with the RiffRacer recovery project's symbol/string
reference consulted. No files were written into that recovery checkout.

- `gaSaveGame.updateSongDifficulty` (06001204, RVA 2F73E8) stores its integer
  rating at localSong +192 (IL 00A9–00B0) and calls writeSongCache.
- `sortByDifficulty` (06001596, RVA 362780) compares that same +192 field.
- `gaTrackSummary.initScreens` (0600088F, RVA 24741C), IL 15BD–1679,
  creates five sprites with definitions 89 (filled) and 90 (empty). It compares
  the loop index against rating local 36, uses origin 4, scales through virtual
  slot 9, and adds through native SSP ownership. Thus five native sprite assets
  already exist; ReRiff need not draw replacement star glyphs.
- `gaRecentTracksMenu.setupRecentTracksList` (060015A0, RVA 36394C) fills
  menu+1260 with 24-byte song-id strings (localSong+24) at IL 042A and menu+1248
  with four-byte row pointers at IL 0949. It skips `__REMOTESONG` rows before
  both pushes. Each original row has two text children and two sprite children.
- Its text clipping budget uses `ldc.r8 1400` at IL 03C6, exact bytes
  `23 00 00 00 00 00 E0 95 40`; the row's separate 1400-wide hitbox is retained.
- Recent Tracks vtable RVA 7A79C8 slot 3 points to RVA 364E74, whose original
  x86 thunk is `FF 25 3C 7A BA 00` at image base 400000 (fixup RVA 7A7A3C).

## Intentional ReRiff extension

After the original tick, match the current rows' song IDs to the saved catalogue
and append native 89/90 sprites through existing createSprite and add-item calls.
Use row-relative dimensions and reserve a right-side column by changing only the
native text-clipping constant from 1400 to 1100. Leave row height, hitboxes,
scrolling, selection and sort behavior native. Children disable their own input.
Rows own their sprites; cleanup remains native. A decorated first row avoids
repeating the catalogue scan each frame; native sorting rebuilds rows.
Ratings outside 1–5 display five empty stars, not an invented difficulty value.

## Validation scope

Original constant and vtable/thunk bytes compared to the pinned executable.
Native fixture checks sprite IDs for known/unknown ratings, non-overlap at six
scales (0.5–3), disabled input, consuming SSP references and duplicate prevention.
Development DLL build passed. The native fixture passed in the isolated Proton
Experimental Wine prefix, including all six scales and ratings -1 through 6.
Live appearance, original sprite scaling and native sorting/cleanup integration
still require in-game testing. No claim of byte-matching reconstruction is made.
