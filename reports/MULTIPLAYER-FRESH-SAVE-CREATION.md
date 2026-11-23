# Fresh multiplayer save allocation before native serialization

Research date: 2026-11-15. Original reference:
the hash-verified `Game.exe` reference.
Direct SHA-256: `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted RiffRacerDecomp recovery files `Decomp/native/save_device_open.S`
and `save_device_create.S`, plus ReRiff's original CIL survey and decompiled
`gaSaveGame` methods. No original bytes or reconstruction files were changed.

## Original evidence

`gaSaveGame.createSaveFiles2`, token `0x06001215`, RVA `0x2f6020`, reads file
count at save object +172, names at +64 with stride 32, and computed file sizes
at +160 with stride 4. It calls the active device's exists/delete/create slots.
The first size is therefore supplied by the game's original file/chunk setup,
not guessed from managed structure sizes. Its original 188-byte fat-header-plus-
CIL body has SHA-256
`2cc1d2fbe07dbf9889157a9bbf78e38fc99caee70d3dc5e4c868830bae6657ca`.
Direct bytes from the baseline PE were compared equal with the preserved
`analysis/matching-decomp/original-il/0x6001215.cil` body.

The original local create entry is RVA `0x1358f0`, already checked by ReRiff's
save-device vtable guards. It creates directories and allocates/zero-fills the
requested file length. Its native open entry, RVA `0x135b10`, first requires
existing file attributes; mode 3 is update, not create. The original save2
method (`0x060011EE`, RVA `0x2f5ccc`) opens dirty chunks in mode 3. Its limited
per-chunk create fallback does not create the default `save.bin` in this case.
The higher createSaveFiles wrapper also has cloud reload and reset behavior;
it is not a safe general replacement for initializing an isolated target.

## Observed failure and intentional fix

A live new multiplayer account reached the isolated native load with zero XP,
zero coins, no achievements and dirty/enabled settings and progression chunks.
At 2026-11-15T07:21:45Z, mode-1 open returned false, followed by mode-3 open
returning false; no create call occurred. The managed SaveCurrent adapter then
refused the checkpoint and closed the game. Server authentication succeeded,
and the server account still had no checkpoint. The missing allocation step
was therefore outside the earlier protocol tests, whose native saves were
synthetic buffers.

ReRiff now calls the original device create function with the original computed
file size before the existing fresh-profile save callback. It does so only for
an active, initializing multiplayer domain, no prior observed load, exactly one
native file named save.bin, a 1-byte-to-1-MiB native size, ready save flags, and
an absent isolated target. It never deletes/recreates an existing target and
never creates or overwrites the source save. Original serialization still owns
all file bytes after allocation. This is an intentional ReRiff integration
change, not reconstructed original behavior or matching-byte credit.

Synthetic native regressions exercise allocation size, refusal to overwrite,
wrong file tables, save-disabled flags and already-loaded domains. A live
fresh-profile retry is still required. The separately requested completed-save
import reuses the existing native source-copy path and targets only an empty
server account; it does not bypass these fresh-profile guards.

## Live import and prestige validation

After the client fix and import API were deployed, a user-requested import at
2026-11-15T07:38:20Z copied a 704-byte original cloud save through the existing
native seed path. Server generation 1 had 61,338,574 permanent/cycle XP, zero
prestiges, 38 displayed achievements and 2,330 coins. Its native-save SHA-256
matched the backed-up source bytes exactly. The per-account import marker cleared.
At 07:39:34Z the user confirmed the native prestige prompt; the live log and
authenticated server read agreed on generation 2, prestige 1, unchanged permanent
XP, zero cycle XP/coins, First Prestige only, and the equipped car 5/pack 6 retained
as the only ownership. The source save hash remained unchanged after this reset.
This validates the import and manual prestige integration for that run. It does
not validate fresh-empty-profile allocation in a live game or reconnect recovery;
those boundaries remain distinct from the automated native/protocol regressions.
