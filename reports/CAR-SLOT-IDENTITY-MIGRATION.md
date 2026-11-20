# Car slot identity and native save lifecycle

Status: local experimental integration, not installed or live-validated.
This note separates original behavior from intentional ReRiff changes. It does
not claim matching reconstruction or verified compatibility with arbitrary saves.

## Reference and original behavior

Reference: Steam depot 351991/build 1687655, `Game.exe` SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Existing provenance and exact original guards are in
`CAR-CAPACITY-AND-SAVE-TRACE.md` and `PRESTIGE-NATIVE-BOUNDARIES.md`.
The local original CIL inspection is retained in `build/car-save-inspect/save.il`.

- `gaSaveGame.load2`, token `060011EC`, RVA `002F5A84`, reads the sizes
  reported by the native chunks, closes the file, and checks its corruption
  state. It does not require EOF immediately after those ranges. This supports
  investigating a trailing metadata envelope; static inspection is not a live
  round-trip test.
- `gaSaveGame.save2`, token `060011EE`, RVA `002F5CCC`, writes enabled dirty
  chunks through the native device. Save byte +13 inhibits saving; +14 marks
  corruption. ReRiff retains the original serializer and checkpoint mechanism.
- Settings `setChosenPackIndexForCar`, token `0600127B`, RVA `002FD37C`, takes
  `(settings, car, pack)` and writes +236+4*car. `setChosenCar`, token
  `0600127C`, RVA `002FD39C`, writes +324. Both mark settings dirty when changed.
- Progression stores 22 usage counters at +188+4*car, car ownership bits at +53,
  and nine paint ownership bits per car in two-byte fields at +56+2*car.
  The existing native car/pack setters are used; usage has an increment API,
  so restoring its counter requires a separate bounded field write.
- Original startup creates the paint and unlock managers before loading saves.
  `gaLoaderFE.load`, token `0600147B`, RVA `0033BE38`, later invokes
  `gaUnlockManager.checkPackUnlocks`, token `060012DA`, RVA `00309024`.
  The unlock singleton is field `04001342`,
  `?ms_pInstance@gaUnlockManager@@0PAV1@A`.
- `isCarUnlocked`, token `060012D1`, RVA `003089FC`, combines configured
  enabled/unlocked-at-start values with saved ownership. `unlockCar`, token
  `060012D2`, RVA `00308F4C`, applies free/sole-pack defaults. `unlockPack`
  (`060012D8`) also evaluates achievement thresholds. A second explicit unlock
  reconciliation is not established as side-effect-free and is not added.

## Intentional ReRiff changes

`origin.txt` opts packages into donor replacement or automatic allocation. The
fixed 22-slot layout remains unchanged. Slots 0–14 remain stock; 15–21 are
allocated to stable package/source identities and may be reused. The generated
`mods/car-slots.ini` records assignments, donors and historical inactive entries.

A per-save SHA-256 checked trailer (`RRCAR001`, schema 1) carries that save's
layout and retired car state through opaque local/server checkpoints. A global
registry alone cannot identify the writer of a remotely downloaded save.

After native file-close/load observation, `CarSaveRuntime.Apply` migrates car
state in memory before the existing frontend unlock check. It does not save
recursively from `load2`. A later main-menu tick saves pending migration using
the original native save function. Native save-close observation seals metadata
before the existing checkpoint commit. An unchanged layout does not force a save.

The migration preserves stock fields, moves custom ownership, paint selection,
pack masks, usage and selection by identity, and retains removed state for
reinstallation. A client without installed car mods can still read an incoming
save's identities and fall back to the stock donor. New occupants start empty
and use the existing frontend unlock lifecycle. Ambiguous legacy custom progress
without identity metadata is refused rather than guessed.

Shared ghost headers are normalized to the stock donor and default paint.
Custom-model sharing is still deferred; replay payload bytes remain unchanged.

## Automated evidence and remaining tests

Local smoke tests cover allocation, registry validation, removal/reinstall,
trailer checksums and malformed envelopes. Cache integration runs against the
original VFS pair, including original deflate-option flags and mixed-case ZIP
names, then validates composed rosters, model resources, reuse, removed packages,
overwrite behavior and preservation of modified caches. These are intentional
patch tests, not byte-match recovery results.

Still required: live startup/garage/race/restart tests, removal and reinstall,
cross-client/server checkpoint round trips, and original-reader compatibility
with the appended envelope. Legacy custom-save import requires a verified
identity mapping. Paint-count/order changes are not automatically migrated.
