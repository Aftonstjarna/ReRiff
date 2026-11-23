# Native VFS override precedence: corrected evidence

Date: 2026-11-16 (America/New_York). Context: ReRiff experimental checkout
52048be plus the working changes described here. Consulted the supplied
`reports/LIVERY-MAKER-REQUIREMENTS.md` in the primary checkout, prior
`reports/MODS-VFS-DIRECTORY.md`, and the recovery project's native notes.
This is original-byte analysis and native behavioral testing, not matching
source reconstruction credit. No live recovery checkout was modified.

## Original provenance

Windows depot 351991/build 1687655, immutable reference:
`$VANILLA` (see `packaging/vanilla-reference.json`).
Game.exe SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The fixture verifies this hash before mapping original code.

Original `graphics/vehicles/decals/alf_01/batchTextures.brres` SHA-256:
`7805bdcc802a85f5d22e47053a08a50ccc37b158a76312a82d83cded684d26f0`.
The runner verifies this hash against the extracted original ZIP member.
All VAs below use preferred image base 0x400000.

## Correction to prior append claim

The lookup at `0x560230` does return the first matching mounted record. The
incorrect premise in prior notes was that secondary loading appends records.
The original secondary loader at `0x55ED80` instead searches for an insertion
position according to the incoming volume ID:

- `0x55F0F4`: `8B 95 68 FF FF FF` loads the incoming volume ID into EDX.
- `0x55F0FA`: `83 C0 0C` addresses an existing record's volume ID.
- `0x55F100`: `39 10 76 0A` compares existing ID with incoming ID and branches
  when existing <= incoming (unsigned comparison).
- `0x55F104`: `46 83 C0 14` advances the position and 20-byte record.
- `0x55F10E`: `8B FE` selects this insertion position.
- `0x55F110..0x55F124` computes `begin + position*20` and passes it to insertion.
- `0x55F12F`: `E8 2C 28 00 00` calls native vector insertion `0x561960`.

Thus higher IDs precede lower IDs; an equal ID is inserted before its existing
peer. Named mods receive increasing free IDs in sorted folder order. Their
later folder therefore wins duplicate paths, including over stock ID 0. Legacy
numbered archives participate by native ID, not a separate named-mod priority.
No new lookup detour or mount-order replacement is necessary.

The static initial loader at `0x55E740` stores discovered records via
`0x55EAA1 -> 0x52C610`. It calls the same secondary loader at `0x55ECBE`.
The fixture exercises secondary loading against a prepopulated baseline vector;
it does not execute the whole game's primary initialization/thread lifecycle.

## Native read validation

`ExperimentalMenu/test_mods_vfs.c` resolves the original imports and applies
HIGHLOW relocations only to its private mapping. Original functions used:

- scanner `0x55E130`, secondary loader `0x55ED80`;
- resource lookup `0x560230`, index-tree lookup `0x598D50`;
- archive stream constructor `0x597940`, read `0x597C00`, destruction `0x597B60`.

The fixture supplies a minimal manager and a false cancellation callback for
stream reads. It does not replace native lookup, ZIP decoding or stream reads.
A temporary test-only scanner routes the baseline mount to the fixture directory
or immutable original archive directory. The production scanner hook handles
named mods. Both before-manager hook installation and late-hook installation
are exercised in separate processes.

Four modes passed: synthetic baseline with hook installed early/late, and actual
original stock archive with hook installed early/late. Assertions cover:

- byte-for-byte native read of all 1,398,315 bytes of original ALF livery 1;
- two valid generated mods sharing that exact normalized resource path;
- native read returns the later folder's distinguishable payload;
- separate added paths, baseline fallback, missing lookup returning null/index 0;
- strictly descending unique volume IDs, original mount record preserved;
- original ALF bytes readable again when the fixture's visible vector contains
  only the original record. This models absence, not runtime unloading support.

Native low-level lookup uses backslash-separated, lowercase paths in these
checks. A mixed-case `batchTextures.brres` lookup against the generated lowercase
index failed; using lowercase `batchtextures.brres` succeeded, including against
the original index (whose ZIP member spelling has an uppercase T). This does
not establish every upstream resource-name normalization call site.

Reproduce on Linux with an isolated Wine prefix containing the game's VC2013
runtime; pass absolute paths for the chosen compiler, Wine binary and reference:

```sh
python3 scripts/test_mod_overrides.py \
  --zig /path/to/zig --wine /path/to/wine \
  --prefix /path/to/disposable-prefix/pfx \
  --reference /path/to/original-depot
```

The runner builds the fixture, generates temporary pairs, checks original
hashes, runs all four read cases plus the pre-existing mount-only case, and
removes generated artifacts. Original assets are read-only inputs; no installed
game, save, account configuration or original archive is changed.

## Intentional production fix and limits

Previously `mods_manager` was captured only when installing the hook. If the
manager did not yet exist, later scanning omitted its mounted IDs when choosing
free IDs. The new change refreshes the pointer from original global `0xBFCD2C`
at discovery time. The synthetic startup fixture reserves an existing ID 1 and
asserts the new mods get distinct higher IDs. This is a ReRiff timing fix, not
original reconstructed code. No lookup hook remains in the final change.

The mod payloads are byte strings, not valid edited textures. These tests prove
asset resolution and native byte reads, not shader binding, decal semantics,
UV orientation, garage/race rendering or texture-cache invalidation. Those
livery-maker gates remain open. Require a full restart for mod file changes;
this work adds no live mount/unmount or hot-reload contract.
