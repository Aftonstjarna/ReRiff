# Native cached ghost missing from Singleplayer selection

Original reference: Windows depot 351991/1687655 Game.exe, 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
This is original control-flow/format evidence and an intentional ReRiff repair;
it is not a source reconstruction or matching-byte credit claim.

## Original behavior

`gaReplayManager.saveGhost`, MethodDef 0x060006af, RVA 0x221e44,
360 CIL bytes, SHA-256
`8d2fe32e18c6347416580c73a0d8568185fbca2bbce55149196ef861243689b4`.
CIL +0x10..0x2c reads the old local replay header. At +0x47..0x51 the old
score is converted through R8/I4; +0x52..0x68 rejects a current replay whose
score is below that cached score. A positive replay duration is also required
at +0x69..0x7f. The original file writer is called at +0x94..0x9b. Later,
+0xef..0x107 checks native local-song fields +152 and +128 before queuing
`gaSaveGame.addPendingGhostUpload` at +0x112..0x118.

A new backend can consequently have a race score but no ghost row when the
shared native replay cache contains a better old personal best. The ghost
selector's successful `ghost_v18` song query then returns zero rows. Observed
locally: score present, zero matching ghost records, valid older cached replay
with a higher score. Five other stored ghost files were preserved and matched
the corresponding native cached bytes. This is evidence of the tested case,
not proof that every absent ghost has this cause.

## Original replay header

`gaReplay.readFromFile`, MethodDef 0x06000690, RVA 0x220c40,
403 CIL bytes, SHA-256
`96157b9677cf9b0750271dbbd6e7c96b36292211a9ea4729088f26ecc0ee977e`.
The supported header reads, in order: int32 version (18), int32 stream count,
float duration (object +116), length-prefixed byte string user (+64), float score
(+8), length-prefixed song (+12), length-prefixed car (+36), int32 pack (+60),
then payload length and stream headers. Strings use a 32-bit byte length.
The repair only reads the bounded identity header; original game code still
owns full replay validation, upload serialization and playback.

`gaReplayManager.calcLocalGhostFilename`, token 0x060006ad, RVA 0x221ad8,
gets the original `gaOnline.getUniqueOnlineID` and delegates to
`calcGhostFilename` (0x060006ae, RVA 0x22193c), which uses `%s-%s.ghost`.

## Original queue and ownership bindings

- `gaSaveGame.addPendingGhostUpload`: 0x06001209, RVA 0x2f9154, 59 CIL bytes,
  SHA-256 `1279ddce53ef92829f1276fe49c1ac6d43b73c080f6208ac02af4e3f6d9540bc`.
  The original routine compares filenames, skips duplicates, copies a native
  string into its vector, and calls `savePendingGhostUploads`.
- `gaSaveGame.savePendingGhostUploads`: 0x06001207, RVA 0x2f6354. The native
  pending list is serialized through `elSaveDevice`; the observed filename is
  `pendingGhostUploads.bin`. It is separate from progression `save.bin`.
- `gaOnline.getUniqueOnlineID`: 0x060012f3, RVA 0x30a774, 166 CIL bytes,
  SHA-256 `30e4470f44962df826d87e9750c46e8d9e48f6ab58e2258933aa1a42a7ddf233`.
- Native char-string constructor: 0x06000023, RVA 0x1d4acc, 53 CIL bytes,
  SHA-256 `a7ede8e3ffdc81e4bd9d00d341ecbe750c37f3ae6c1f7cffe527b6f337c5b1e2`.
- Native char-string `_Tidy`: 0x06000034, RVA 0x1d406c, 49 CIL bytes,
  SHA-256 `bb3de963003b275217969972fb79e4bb257a848834cdc25d97083807f2ec88b9`.
- Original pending vector uses 24-byte string objects. The constructor and
  original `_Tidy(true,0)` own allocation/freeing of the internal text buffer.
- Save singleton 0x0400126f and online singleton 0x040013b6 retain their exact
  original metadata names in the repair's checked bindings.

## Intentional integration and validation limits

ReRiff extends its already guarded native main-menu tick, after forwarding the
original tick. On the game thread, one cached replay per tick is considered.
Only actual Singleplayer mode, the signed-in native owner with a matching local
profile, a matching filename/header owner/song, and a missing own ghost record
qualify. Existing backend records, replay files and progression are unchanged.
The original queue handles deduplication and upload completion. Foreign-owner,
unsupported-version, malformed, non-finite and truncated headers are excluded.
No calls into game state are made from HTTP workers.

Parser fixtures and the affected native replay header pass. Binding provenance
is pinned to the full original executable hash. Native string contents are
checked before queue mutation, and the original destructor releases its storage.
Live native queue/upload completion and selector availability remain pending.
Recommended selection still follows the original rank/offset query; this repair
does not introduce a personal-best recommendation fallback or change IDs19/32.

## Live failure and revised repair

The initial native-queue candidate FAILED the live test: the game exited before
showing the main menu. The launcher recorded exit code 1. No new engine-menu
startup or cache-queue diagnostics were emitted, so the exact interop failure
is unresolved; there is no evidence the queue processed any replay. The two
candidate DLLs were rolled back to the validated split-config build. Do not
consider the native queue adapter safe or verified. Its bindings above remain
original-code observations, not evidence of a working ReRiff call context.

The revised implementation removes the new native callback, main-menu tick
addition and managed/native queue bindings. It extends the existing
Singleplayer storage migration instead. When the original selector queries a
missing `ghost_v18` song, a cached replay may supply the normal `ghost_v18` and
`FileData` records and an exact payload copy. A profile's observed SystemID is
preferred; inference is allowed only with a single non-seed local profile.
Foreign owners, ambiguous profiles, invalid headers and existing/conflicting
records are retained without import. Source replay files, race scores and
progression are untouched. This is an intentional backend migration, justified
by the failed native integration; it does not replace selection or playback.

The original Parse SDK regression confirms imported owner/score/car/pack fields,
byte-for-byte HTTP download, idempotence, preservation of existing ghost records,
and unchanged source bytes. The ordinary 16 SDK checks also pass. Live selector
and playback validation of this revised candidate remain pending.

## Storage-only live result

PASS for startup and Leader availability: the user confirmed the menu opens and
the Leader ghost appears. The trace shows one own cached replay imported, then
the same Leader song query changes from zero results to one. The copied payload
is byte-identical to the native source, all 32 observed source replays retain
their hashes, and the original Steam progression save retains its baseline
hash. The loaded isolated checkpoint retains its prior XP, coins, prestige,
equipped items and achievements. Download/playback and a winning Friend or Foe
award are not yet live-verified.

The same live session records Recommended's score-count query with zero higher
scores, followed by the descending-score query `skip=1&limit=1`. With only one
matching ghost row, that query returns zero. Thus the sole own record is visible
in Leader but not Recommended in this observed case. This does not prove a
universal self-exclusion rule; it confirms the original offset behavior here.

## Subsequent live download and achievement check

The original selector fallback downloaded a 205332-byte replay. The player
completed a Leader race at147420 against93128; original score comparison
reported a strict win. The Singleplayer extension awarded original achievement
32 once, and its later results callback preserved the already-earned flag.
The user confirmed Friend or Foe is earned. Native checksum-valid save and
checkpoint generation18 retain the flag, prestige1, cycleXP1034466,
permanentXP62909630 and2900 coins; the original Steam save remains unchanged.
This validates download/playback and the win award for the tested Leader.
A full restart after this win has not yet been reported. Imported FileData's
username-filtered lookup still needed the original name-only fallback; this
result does not claim that every query path or Recommended selection is fixed.
