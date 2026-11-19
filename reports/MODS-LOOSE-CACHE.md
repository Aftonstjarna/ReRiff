# Loose mod cache paths through original native VFS

Verified 2026-11-19. This is new ReRiff integration research, not an original
reconstruction or matching-byte claim.

## Provenance

Reference: Steam depot 351991/build 1687655, unmodified Windows `Game.exe`,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Addresses below are RVAs (preferred image base `0x400000`). Observed original
byte ranges, not whole-function claims unless the range covers that function:

| Observed range | RVA | Bytes | SHA-256 |
| --- | --- | --- | --- |
| vector push move | `0x1614e0` | 150 | `fbe2ff8b37eaa23a5059c7b64d5497d7c06e591175edd1c56b4aa70e1a27557b` |
| wide string move | `0x4f6f0` | 106 | `ddddfd168f6985e222b5e14483be7861e8cf4aacd11466a3137f4cfeba853060` |
| wide string assign guard | `0x4f9b0` | 10 | `eac69ef9b884a24e8e9007d1252396fc04dd14e1da1c59e68cca33565eecc1fa` |

Vector push guard: `55 8b ec 56 8b f1 57 8b 7d 08`.
Wide-string assign guard: `55 8b ec 53 8b 5d 08 56 8b f1`.
The existing secondary scanner call at RVA `0x15ede7` remains the only patched
call site (`e8 44 f3 ff ff`). ReRiff checks the reference executable hash and
these original instruction guards before binding native helpers.

## Verified behavior

The original scanner calls RVA `0x1614e0` with ECX pointing to its three-pointer
vector and one stack argument pointing to a 28-byte entry (volume ID followed
by a 24-byte wide string). The function returns with `ret 4`.

**This insertion MOVES the string; it does not copy a borrowed buffer.** Its
external-entry path calls RVA `0x4f6f0`, which transfers a long-string pointer,
then clears the source pointer, length and capacity (reset to 7). Passing a
borrowed path produced a reproducible failed mount after the buffer was freed.
The corrected adapter initializes an empty string with capacity 7, then uses
original assign RVA `0x4f9b0` (thiscall, path and character count on stack) before
moving the entry. Original vector destruction owns and releases the allocation.
No foreign allocator is used for native strings.

The original mount pass accepts an arbitrary archive stem; it appends `.dat`
and `.zip`. No `vfs` prefix or numbered filename is required when entries are
supplied directly. This permits `.reriff-merged-<hash>` directly in `mods/`.
Ordinary `vfs*.dat` discovery does not rediscover that generated cache.

## Intentional ReRiff changes

`ClientConfig/ModArchiveCache.cs` generates indexed stored-ZIP pairs from loose
`assets/` trees and merges named archives when available IDs are exhausted.
The writer follows the already verified `packaging/native_archive.py` format;
there is no original loose-directory-to-VFS builder to reuse. Original native
index parsing, mounting, file lookup and reading remain in use.

Individual caches stay in their mod folders; overflow caches stay directly in
`mods/`. Earlier mods retain slots, later mods merge in ordinal priority order.
Already mounted/numbered archives remain reserved; zero free slots cannot be
recovered by replacing archives already in use. Native per-volume bounds remain.

## Validation

`ClientConfig/Tests/ModArchiveCacheSmoke.cs` checks cache reuse, content changes
including unchanged size/mtime, source additions/removals, disabled folders,
root-level overflow cache placement, mixed archive/loose mods, >31 named mods,
reserved slots, invalid paths and corruption recovery. Package tests verify a
single ZIP of loose assets and reject links, nested ZIPs and output overwrite.

`python scripts/test_mod_overrides.py --cache-smoke <built-test.exe>` retains
legacy native tests and adds generated merged-cache mounts. Private relocated
mappings of the hash-pinned original executable test both early and late hook
installation, synthetic stock and real original archives, exact override bytes,
last-folder priority, missing resources, stock fallback and preserved stock
mount records. The game entry point is never called. These native cases passed.
No normal installation or active car test was changed; live rendering remains
untested for this cache integration.

## 2026-11-19: live loose-load and authoring follow-up

This supersedes the initial no-install/live-test-pending statement above.
The development cache loader was installed into the normal game and the owner
confirmed a loose Pulse Test load. Initial extraction into a directory named
`vfs/` was not recognized: this intentional ReRiff format requires `assets/`.
All 33 extracted files matched the tested mod ZIP member hashes before the
folder was renamed. The standalone obsolete `vfs.dat` was backed up. Native
mod count had fallen from five to four with the wrong layout, and the existing
missing-car fallback selected stock Pulse; after the correction the owner
confirmed successful loose loading. No asset bytes were changed by the rename.
Audit: `build/ghost-fallback-only/loose-layout-fix.json`.

The later Goliath-name issue also occurred with packed mods. It was an omitted
live-test localization-refresh setting, corrected and owner-confirmed as recorded
in `CAR-CAPACITY-AND-SAVE-TRACE.md`. It is not evidence of a loose-cache lookup
or resource-precedence difference. Automatic overflow merging still has native
fixture coverage rather than an owner-confirmed over-limit live rendering test.

### RiffRacer Customs loose-export integration

The maintained standalone authoring checkout is
`/var/mnt/512GB-SSD1/reriff-livery-standalone`. The older ReRiff-worktree
`LiveryMaker/` was not substituted for this newer Windows/Linux tool.
Existing unrelated GUI/preview edits were preserved.

Intentional tooling changes, not recovered original code:

- Export native resources under `assets/<original-lowercase-resource-path>` plus
  `livery-maker.json` ownership metadata (`reriff-livery-maker-v2`, `loose-v1`).
  New exports contain neither `vfs.dat` nor `vfs.zip`; ReRiff builds the cache.
- Retain texture/paint encoding and native resource contents. Validate ownership
  by per-resource hashes; migrate hash-verified v1 archive exports on re-export.
- Accept only complete, hash-verified loader cache pairs as disposable outputs.
  Re-export discards those stale caches; refuse changed assets, unknown files,
  symlinks/reparse points and unsafe resource paths.
- Detect conflicts against loose and legacy archive mods; ignore `.disabled`
  and hidden generated caches. The editor does not create an overflow merge.
- Updated independent package and native-paint expectation helpers to accept
  loose resources. Existing archive validation remains available for old exports.

Validation: warnings-as-errors C++ build and unit tests passed, including
re-export, verified cache disposal, changed-asset refusal, v1 migration and
loose conflict detection. Original-asset tests passed for 110 paints, 30 decal
layouts/model UVs and 82 eligible added-decal paints. The headless diagnostic
export and actual GTK paint-edit/export test produced loose packages accepted
by the independent Python layout/hash/CFG/Lua/TABT/DDS/mip validator. The paint
expectation helper produced 3373 native-value assertions; generating these
assertions is not a new execution of all assertions in the original Lua runtime.

The loader's original-code native cache-read tests are recorded above. The new
Customs exports were not separately rendered in a live race in this pass.
Windows-specific execution of this updated exporter was not performed. A local
Linux executable was built; no release was published. Older archive-only ReRiff
releases do not support the new export layout.

Standalone source snapshot provenance (uncommitted changes included):

| File | SHA-256 |
| --- | --- |
| `LiveryMaker/core.hpp` | `e1ff0e06b140fcb0c0f5f0529cbe3d2f43d5da75ce85357d170c0cdf5e91a177` |
| `LiveryMaker/test.cpp` | `bf55735540e41410d6085da0743ec37c4cec50acf426ff6b436f59d08d6f25fd` |
| `LiveryMaker/main.cpp` | `1253b94cbb5ab93e2c70633def7034985d7f29456adc7e36148b9f73534af5b3` |
| `LiveryMaker/validate_package.py` | `7283a2ce17e4036e67299b9ea35d4435c3d733bd608ef7892be0751e2ae8fe33` |
| `LiveryMaker/native_paint_check.py` | `6e44c5b11071338d367ec3627d86c6962d3c9519aebe1c7e57d2cb27b2b2956a` |

Standalone HEAD: `81f66dc5aaa871a22b0bc9e5e2203ae6e862b2bc`. Source hashes identify the actual tested working files; HEAD alone does not.
